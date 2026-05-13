#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "mxc_config.h"
#include "gpio.h"
#include "tmr_utils.h"
#include "i2cm.h"
#include "ioman.h"
#include "ioman_regs.h"
#include "clkman.h"
#include "uart.h"
#include "pwrseq_regs.h"
#include "rtc_regs.h"

/* CC256XB service pack — BasePatch[] + LowEnergyPatch[].
 * BTTypes.h is a local shim that defines BTPSCONST = const so the header
 * compiles without the full Bluetopia SDK. */
#define __SUPPORT_LOW_ENERGY__
#include "BTTypes.h"   /* local shim: #define BTPSCONST const */
#include "CC256XB.h"   /* TI service pack — path added to IPATH via Makefile */

/* -------------------------------------------------------------------------
 * PAN1326B (TI CC2564B) connected to MAX32630FTHR via UART0.
 *
 * All P0/P1 BLE pins run at VDDIO (1.8V logic) — clear use_vddioh bits.
 *
 * Pin mapping:
 *   P0.0  UART0 RX (Mapping B)  ← PAN1326B TX   (Mapping B crossover needed: Map A would put TX here)
 *   P0.1  UART0 TX (Mapping B)  → PAN1326B RX
 *   P0.2  UART0 CTS input       ← PAN1326B RTS  (module signals MCU it may transmit)
 *   P0.3  UART0 RTS output      → PAN1326B CTS  (MCU signals module it may transmit)
 *   P1.6  GPIO out  nSHUTD       active-low reset; drive HIGH to enable module
 *   P1.7  32kHz RTC output       PAN1326B SLW_CLK reference
 *
 * Boot sequence:
 *   1. Start 32kHz nano oscillator → route to P1.7
 *   2. Init UART0 (Mapping B, 115200, CTS/RTS)
 *   3. Release nSHUTD reset, wait 500ms for module boot
 *   4. HCI Reset
 *   5. Upload CC256XB service pack (BasePatch + LowEnergyPatch)
 *   6. LE advertising as "SentinelBox"
 * ------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------
 * Debug UART — UART1 on P2.0(RX)/P2.1(TX), USB-to-serial bridge on FTHR.
 * Connect with: screen /dev/tty.usbmodem* 115200
 * ------------------------------------------------------------------------- */
static void debug_init(void)
{
    const uart_cfg_t cfg = {
        .parity = UART_PARITY_DISABLE, .size = UART_DATA_SIZE_8_BITS,
        .extra_stop = 0, .cts = 0, .rts = 0, .baud = 115200,
    };
    const sys_cfg_uart_t sys = {
        .clk_scale = CLKMAN_SCALE_DIV_1,
        .io_cfg    = IOMAN_UART(1, IOMAN_MAP_A, IOMAN_MAP_A, IOMAN_MAP_A, 1, 0, 0),
    };
    UART_Init(MXC_UART1, &cfg, &sys);
}

static void dbg(const char *s)
{
    UART_Write(MXC_UART1, (uint8_t *)s, strlen(s));
}

static void dbg_hex(const char *label, const uint8_t *buf, int len)
{
    char tmp[8];
    dbg(label);
    for (int i = 0; i < len; i++) {
        snprintf(tmp, sizeof(tmp), "%02X ", buf[i]);
        dbg(tmp);
    }
    dbg("\r\n");
}

/* -------------------------------------------------------------------------
 * Board init — PMIC (MAX14690) via I2CM2, LDO2+LDO3 → 3.3V.
 * ------------------------------------------------------------------------- */
int Board_Init(void)
{
    const sys_cfg_i2cm_t i2cm2_cfg = {
        .clk_scale = CLKMAN_SCALE_DIV_1,
        .io_cfg    = IOMAN_I2CM2(IOMAN_MAP_A, 1),
    };
    if (I2CM_Init(MXC_I2CM2, &i2cm2_cfg, I2CM_SPEED_100KHZ) != E_NO_ERROR)
        return E_NO_ERROR;

    uint8_t ldo2_vset[2] = { 0x15, 25 };
    uint8_t ldo2_cfg[2]  = { 0x14,  2 };
    uint8_t ldo3_vset[2] = { 0x17, 25 };
    uint8_t ldo3_cfg[2]  = { 0x16,  2 };

    I2CM_Write(MXC_I2CM2, 0x28, NULL, 0, ldo2_vset, 2);
    I2CM_Write(MXC_I2CM2, 0x28, NULL, 0, ldo2_cfg,  2);
    I2CM_Write(MXC_I2CM2, 0x28, NULL, 0, ldo3_vset, 2);
    I2CM_Write(MXC_I2CM2, 0x28, NULL, 0, ldo3_cfg,  2);

    return E_NO_ERROR;
}

/* -------------------------------------------------------------------------
 * RGB LED — P2.4=R, P2.5=G, P2.6=B, active-low open-drain.
 * ------------------------------------------------------------------------- */
static const gpio_cfg_t led_r = { PORT_2, PIN_4, GPIO_FUNC_GPIO, GPIO_PAD_OPEN_DRAIN };
static const gpio_cfg_t led_g = { PORT_2, PIN_5, GPIO_FUNC_GPIO, GPIO_PAD_OPEN_DRAIN };
static const gpio_cfg_t led_b = { PORT_2, PIN_6, GPIO_FUNC_GPIO, GPIO_PAD_OPEN_DRAIN };

static void led_init(void)
{
    GPIO_Config(&led_r); GPIO_OutSet(&led_r);
    GPIO_Config(&led_g); GPIO_OutSet(&led_g);
    GPIO_Config(&led_b); GPIO_OutSet(&led_b);
}

static void led_set(int r, int g, int b)
{
    r ? GPIO_OutClr(&led_r) : GPIO_OutSet(&led_r);
    g ? GPIO_OutClr(&led_g) : GPIO_OutSet(&led_g);
    b ? GPIO_OutClr(&led_b) : GPIO_OutSet(&led_b);
}

/* -------------------------------------------------------------------------
 * HCI UART — UART0 at 115200 baud, Mapping B, hardware flow control.
 * ------------------------------------------------------------------------- */
#define BLE_UART        MXC_UART0
#define BLE_UART_BAUD   115200

static const gpio_cfg_t ble_nshutd = { PORT_1, PIN_6, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };

static void ble_hw_init(void)
{
    /* BLE UART pins (P0.0-P0.3) and control pins (P1.6-P1.7) run at 1.8V.
     * Default is VDDIO (1.8V); clear any bits that may have been set VDDIOH. */
    MXC_IOMAN->use_vddioh_0 &= ~(PIN_0 | PIN_1 | PIN_2 | PIN_3);
    MXC_IOMAN->use_vddioh_1 &= ~(PIN_6 | PIN_7);

    dbg("[BLE] hw_init start\r\n");

    /* Start the 32.768 kHz nano oscillator, then route it to P1.7.
     * NANO_EN starts the crystal; OSC_WARMUP_ENABLE gates it to the RTC;
     * PWR_PSEQ_32K_EN routes the signal to P1.7 for PAN1326B SLW_CLK. */
    MXC_RTCCFG->clk_ctrl |= MXC_F_RTC_CLK_CTRL_NANO_EN;
    MXC_RTCCFG->osc_ctrl |= MXC_F_RTC_OSC_CTRL_OSC_WARMUP_ENABLE;
    MXC_PWRSEQ->reg4     |= MXC_F_PWRSEQ_REG4_PWR_PSEQ_32K_EN;
    TMR_Delay(MXC_TMR0, MSEC(50));  /* oscillator stabilisation */
    dbg("[BLE] 32kHz osc enabled\r\n");

    /* Hold PAN1326B in reset (nSHUTD LOW) while UART stabilises. */
    GPIO_Config(&ble_nshutd);
    GPIO_OutClr(&ble_nshutd);

    /* Init UART0: Mapping B crossover (P0.0=RX, P0.1=TX), TX/RX only.
     * Hardware CTS/RTS checking is deliberately disabled: when the module
     * boots its RTS output (= our CTS input P0.2) stays HIGH, and with
     * .cts=1 the UART hardware would block every transmission until CTS goes
     * low — the HCI Reset would never be sent.  Mapping the CTS/RTS pins in
     * IOMAN without enabling hardware flow control matches what the working
     * reference Arduino implementation does. */
    const uart_cfg_t ble_cfg = {
        .parity     = UART_PARITY_DISABLE,
        .size       = UART_DATA_SIZE_8_BITS,
        .extra_stop = 0,
        .cts        = 0,
        .rts        = 0,
        .baud       = BLE_UART_BAUD,
    };
    const sys_cfg_uart_t ble_sys = {
        .clk_scale = CLKMAN_SCALE_DIV_1,
        .io_cfg    = IOMAN_UART(0, IOMAN_MAP_B, IOMAN_MAP_A, IOMAN_MAP_A, 1, 0, 0),
    };
    UART_Init(BLE_UART, &ble_cfg, &ble_sys);

    dbg("[BLE] UART0 init done\r\n");
    /* Verify IOMAN acknowledged Mapping B for TX/RX. */
    {
        int ioman_wait = 0;
        while (MXC_IOMAN->uart0_ack != MXC_IOMAN->uart0_req) { ioman_wait++; }
        if (ioman_wait) dbg("[BLE] IOMAN ack waited\r\n");
        else            dbg("[BLE] IOMAN ack immediate\r\n");
    }

    /* Release PAN1326B from reset. Module asserts RTS HIGH while booting,
     * then drives it LOW once ready. Allow 500 ms for boot. */
    TMR_Delay(MXC_TMR0, MSEC(10));
    GPIO_OutSet(&ble_nshutd);
    dbg("[BLE] nSHUTD released, waiting 500ms\r\n");
    TMR_Delay(MXC_TMR0, MSEC(500));
    dbg("[BLE] hw_init done\r\n");
}

/* -------------------------------------------------------------------------
 * HCI helpers — blocking send + receive.
 * ------------------------------------------------------------------------- */
static void hci_send(const uint8_t *cmd, int len)
{
    UART_Write(BLE_UART, (uint8_t *)cmd, len);
}

/* Read up to max_len bytes; returns bytes received or -1 on timeout. */
static int hci_recv(uint8_t *buf, int max_len, uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    int n = 0;

    while (n < max_len) {
        if (UART_NumReadAvail(BLE_UART)) {
            int got = 0;
            UART_Read(BLE_UART, buf + n, 1, &got);
            if (got == 1)
                n++;
        } else {
            TMR_Delay(MXC_TMR0, MSEC(1));
            if (++elapsed >= timeout_ms)
                return (n > 0) ? n : -1;
        }
    }
    return n;
}

/* Drain one HCI event packet (0x04 hdr): reads header then parameters. */
static void hci_drain_event(uint32_t first_byte_timeout_ms)
{
    uint8_t hdr[3];
    if (hci_recv(hdr, 1, first_byte_timeout_ms) < 1 || hdr[0] != 0x04) return;
    if (hci_recv(hdr + 1, 2, 50) < 2) return;
    int param_len = hdr[2];
    if (param_len > 0) {
        uint8_t params[256];
        hci_recv(params, param_len < 256 ? param_len : 255, 50);
    }
}

/* -------------------------------------------------------------------------
 * HCI Reset — 0x01 0x03 0x0C 0x00
 * Expected Command Complete event: 0x04 0x0E 0x04 0x01 0x03 0x0C 0x00
 * Returns 1 on success, 0 on timeout or wrong response.
 * ------------------------------------------------------------------------- */
static int hci_reset(void)
{
    static const uint8_t cmd[] = { 0x01, 0x03, 0x0C, 0x00 };
    dbg_hex("[HCI TX] ", cmd, sizeof(cmd));
    hci_send(cmd, sizeof(cmd));

    /* The last patch command (0xFD5B LE-enable) may not have drained yet;
     * its Command Complete arrives before the Reset Complete.  Loop until
     * we see the Reset opcode (0x0C03) or run out of attempts. */
    for (int attempt = 0; attempt < 4; attempt++) {
        uint8_t rsp[16];
        int n = hci_recv(rsp, 7, 2000);
        if (n > 0) dbg_hex("[HCI RX] ", rsp, n);
        else { dbg("[HCI RX] timeout\r\n"); return 0; }

        if (n >= 7 && rsp[0] == 0x04 && rsp[1] == 0x0E
                   && rsp[4] == 0x03 && rsp[5] == 0x0C)
            return rsp[6] == 0x00;   /* status byte */

        dbg("[HCI] stale event before reset complete, retrying\r\n");
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * CC256XB service pack upload.
 *
 * CC256XB.h stores the patch as a flat stream of HCI command packets:
 *   0x01 <opcode-lo> <opcode-hi> <param-len> [param-len bytes]
 * Each command is sent in full then we drain the Command Complete response
 * before sending the next one. Hardware flow control handles pacing.
 *
 * Upload order: BasePatch first, then LowEnergyPatch (AvprPatch skipped).
 * Expected duration: ~5–15 seconds at 115200 baud.
 * ------------------------------------------------------------------------- */
static void cc256xb_send_patch(const uint8_t *patch, unsigned int len)
{
    const uint8_t *p   = patch;
    const uint8_t *end = patch + len;

    while (p + 4 <= end) {
        if (p[0] != 0x01) break;           /* sanity: must be HCI command */
        unsigned int param_len = p[3];
        unsigned int cmd_len   = 4 + param_len;
        if (p + cmd_len > end) break;

        hci_send(p, cmd_len);
        hci_drain_event(200);              /* 200ms — last patch cmd (LE enable) is slow */

        p += cmd_len;
    }
}

static void cc256xb_init(void)
{
    cc256xb_send_patch(BasePatch,      BasePatchLength);
    cc256xb_send_patch(LowEnergyPatch, LowEnergyPatchLength);
}

/* -------------------------------------------------------------------------
 * HCI LE advertising commands.
 * ------------------------------------------------------------------------- */
static int hci_le_set_adv_params(void)
{
    static const uint8_t cmd[] = {
        0x01, 0x06, 0x20, 0x0F,
        0xA0, 0x00,                          /* interval min = 100 ms */
        0xA0, 0x00,                          /* interval max = 100 ms */
        0x00,                                /* ADV_IND — connectable, undirected */
        0x00,                                /* own address type = public */
        0x00,                                /* peer address type */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x07,                                /* all three channels */
        0x00,
    };
    hci_send(cmd, sizeof(cmd));
    uint8_t rsp[16];
    int n = hci_recv(rsp, 7, 500);
    return (n >= 7 && rsp[0] == 0x04 && rsp[6] == 0x00);
}

static int hci_le_set_adv_data(void)
{
    static const uint8_t cmd[] = {
        0x01, 0x08, 0x20, 0x20,
        0x10,                               /* 16 bytes of AD data follow */
        0x02, 0x01, 0x06,                   /* Flags: LE General Discoverable */
        0x0C, 0x09, 'S','e','n','t','i','n','e','l','B','o','x',
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    hci_send(cmd, sizeof(cmd));
    uint8_t rsp[16];
    int n = hci_recv(rsp, 7, 500);
    return (n >= 7 && rsp[0] == 0x04 && rsp[6] == 0x00);
}

static int hci_le_set_adv_enable(int enable)
{
    uint8_t cmd[] = { 0x01, 0x0A, 0x20, 0x01, enable ? 0x01 : 0x00 };
    hci_send(cmd, sizeof(cmd));
    uint8_t rsp[16];
    int n = hci_recv(rsp, 7, 500);
    return (n >= 7 && rsp[0] == 0x04 && rsp[6] == 0x00);
}

/* -------------------------------------------------------------------------
 * Application:
 *   Blue LED    — hardware init + patch upload in progress
 *   Red LED     — HCI Reset failed (check wiring)
 *   Green blink — advertising as "SentinelBox"
 * ------------------------------------------------------------------------- */
int main(void)
{
    Board_Init();
    debug_init();
    led_init();

    dbg("\r\n[BOOT] sentinel-box bluetooth init\r\n");
    led_set(0, 0, 1);   /* blue: init */
    ble_hw_init();

    dbg("[HCI] sending reset\r\n");
    if (!hci_reset()) {
        dbg("[HCI] reset FAILED\r\n");
        led_set(1, 0, 0);
        while (1) {}
    }
    dbg("[HCI] reset OK\r\n");

    /* Upload CC256XB firmware service pack — required for LE commands. */
    dbg("[CC256X] uploading patch...\r\n");
    cc256xb_init();
    dbg("[CC256X] patch done\r\n");

    /* HCI Reset again after patch upload to put the controller in a clean state. */
    dbg("[HCI] post-patch reset\r\n");
    if (!hci_reset()) {
        dbg("[HCI] post-patch reset FAILED\r\n");
        led_set(1, 0, 0);
        while (1) {}
    }
    dbg("[HCI] post-patch reset OK\r\n");

    /* Start advertising. */
    dbg("[BLE] starting LE advertising\r\n");
    if (!hci_le_set_adv_params() || !hci_le_set_adv_data() || !hci_le_set_adv_enable(1)) {
        dbg("[BLE] LE advertising FAILED\r\n");
        led_set(1, 1, 0);   /* yellow: patch uploaded but LE still failing */
        while (1) {}
    }
    dbg("[BLE] advertising as SentinelBox\r\n");

    /* Green blink: advertising as "SentinelBox". */
    while (1) {
        led_set(0, 1, 0);
        TMR_Delay(MXC_TMR0, MSEC(500));
        led_set(0, 0, 0);
        TMR_Delay(MXC_TMR0, MSEC(500));
    }
}
