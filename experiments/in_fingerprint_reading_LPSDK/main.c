#include <stdint.h>
#include <stddef.h>
#include "mxc_config.h"
#include "gpio.h"
#include "tmr_utils.h"
#include "i2cm.h"
#include "ioman.h"
#include "clkman.h"
#include "uart.h"

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
 * MAX7219 8×8 LED matrix — bit-bang SPI.
 * DIN=P3.3, CLK=P3.4, CS=P3.5.
 * ------------------------------------------------------------------------- */
static const gpio_cfg_t m7_din = { PORT_3, PIN_3, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };
static const gpio_cfg_t m7_clk = { PORT_3, PIN_4, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };
static const gpio_cfg_t m7_cs  = { PORT_3, PIN_5, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };

static void max7219_send_byte(uint8_t b)
{
    for (int i = 7; i >= 0; i--) {
        if ((b >> i) & 1) GPIO_OutSet(&m7_din);
        else              GPIO_OutClr(&m7_din);
        GPIO_OutSet(&m7_clk);
        GPIO_OutClr(&m7_clk);
    }
}

static void max7219_write(uint8_t reg, uint8_t data)
{
    GPIO_OutClr(&m7_cs);
    max7219_send_byte(reg);
    max7219_send_byte(data);
    GPIO_OutSet(&m7_cs);
}

static void max7219_init(void)
{
    GPIO_Config(&m7_din); GPIO_OutClr(&m7_din);
    GPIO_Config(&m7_clk); GPIO_OutClr(&m7_clk);
    GPIO_Config(&m7_cs);  GPIO_OutSet(&m7_cs);

    max7219_write(0x0F, 0x00); /* display test off */
    max7219_write(0x0B, 0x07); /* scan all 8 rows */
    max7219_write(0x09, 0x00); /* no decode — raw pixels */
    max7219_write(0x0A, 0x03); /* intensity: medium-low */
    max7219_write(0x0C, 0x01); /* normal operation */
    for (uint8_t r = 1; r <= 8; r++) max7219_write(r, 0x00);
}

/* 8×8 bitmaps for hex digits 0–F.
 * Each row: bit 7 = leftmost column, rows stored top-to-bottom. */
static const uint8_t hex_font[16][8] = {
    { 0x00, 0x3C, 0x42, 0x42, 0x42, 0x42, 0x3C, 0x00 }, /* 0 */
    { 0x00, 0x08, 0x18, 0x08, 0x08, 0x08, 0x1C, 0x00 }, /* 1 */
    { 0x00, 0x3C, 0x42, 0x04, 0x18, 0x20, 0x7E, 0x00 }, /* 2 */
    { 0x00, 0x1E, 0x02, 0x0E, 0x02, 0x02, 0x1E, 0x00 }, /* 3 */
    { 0x00, 0x04, 0x0C, 0x14, 0x24, 0x7E, 0x04, 0x00 }, /* 4 */
    { 0x00, 0x7E, 0x40, 0x7C, 0x02, 0x42, 0x3C, 0x00 }, /* 5 */
    { 0x00, 0x3C, 0x40, 0x7C, 0x42, 0x42, 0x3C, 0x00 }, /* 6 */
    { 0x00, 0x7E, 0x02, 0x04, 0x08, 0x10, 0x10, 0x00 }, /* 7 */
    { 0x00, 0x3C, 0x42, 0x3C, 0x42, 0x42, 0x3C, 0x00 }, /* 8 */
    { 0x00, 0x3C, 0x42, 0x3E, 0x02, 0x42, 0x3C, 0x00 }, /* 9 */
    { 0x00, 0x18, 0x24, 0x42, 0x7E, 0x42, 0x42, 0x00 }, /* A */
    { 0x00, 0x7C, 0x42, 0x7C, 0x42, 0x42, 0x7C, 0x00 }, /* B */
    { 0x00, 0x3C, 0x42, 0x40, 0x40, 0x42, 0x3C, 0x00 }, /* C */
    { 0x00, 0x78, 0x44, 0x42, 0x42, 0x44, 0x78, 0x00 }, /* D */
    { 0x00, 0x7E, 0x40, 0x7C, 0x40, 0x40, 0x7E, 0x00 }, /* E */
    { 0x00, 0x7E, 0x40, 0x7C, 0x40, 0x40, 0x40, 0x00 }, /* F */
};

static void max7219_show_hex(uint8_t digit)
{
    const uint8_t *rows = hex_font[digit & 0xF];
    for (uint8_t r = 0; r < 8; r++)
        max7219_write(r + 1, rows[r]);
}

/* X pattern — shown on no-match, flashed 3 times. */
static const uint8_t x_pattern[8] = {
    0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81
};

static void show_x_flash(int n)
{
    for (int i = 0; i < n; i++) {
        for (uint8_t r = 0; r < 8; r++) max7219_write(r + 1, x_pattern[r]);
        led_set(1, 0, 0);
        TMR_Delay(MXC_TMR0, MSEC(300));
        for (uint8_t r = 0; r < 8; r++) max7219_write(r + 1, 0x00);
        led_set(0, 0, 0);
        TMR_Delay(MXC_TMR0, MSEC(200));
    }
}

/* -------------------------------------------------------------------------
 * Rotary encoder — CLK=P5.3, DT=P5.4, SW=P5.5, all input pull-up.
 *
 * Clockwise:         CLK falls while DT is HIGH → value++
 * Counter-clockwise: CLK falls while DT is LOW  → value--
 * Value wraps 0 ↔ F.  Swap CLK/DT wires if direction is reversed.
 * ------------------------------------------------------------------------- */
static const gpio_cfg_t enc_clk = { PORT_5, PIN_3, GPIO_FUNC_GPIO, GPIO_PAD_INPUT_PULLUP };
static const gpio_cfg_t enc_dt  = { PORT_5, PIN_4, GPIO_FUNC_GPIO, GPIO_PAD_INPUT_PULLUP };
static const gpio_cfg_t enc_sw  = { PORT_5, PIN_5, GPIO_FUNC_GPIO, GPIO_PAD_INPUT_PULLUP };

static int     enc_last_clk;
static uint8_t enc_value;

static void enc_init(void)
{
    GPIO_Config(&enc_clk);
    GPIO_Config(&enc_dt);
    GPIO_Config(&enc_sw);
    enc_last_clk = (GPIO_InGet(&enc_clk) != 0);
    enc_value = 0;
}

/* Returns 1 if enc_value changed. */
static int enc_poll(void)
{
    int clk = (GPIO_InGet(&enc_clk) != 0);
    int changed = 0;
    if (!clk && enc_last_clk) {
        if (GPIO_InGet(&enc_dt) != 0)
            enc_value = (enc_value + 1) & 0xF;
        else
            enc_value = (enc_value - 1) & 0xF;
        TMR_Delay(MXC_TMR0, MSEC(1));
        changed = 1;
    }
    enc_last_clk = clk;
    return changed;
}

/* Returns 1 while shaft button is held (active-low). */
static int enc_button(void)
{
    return (GPIO_InGet(&enc_sw) == 0);
}

/* -------------------------------------------------------------------------
 * Fingerprint sensor — AS608/R305/R307, UART2 Map A: TX=P3.1, RX=P3.0.
 * Packet: [EF 01][FF FF FF FF][pid][len_hi len_lo][data...][ck_hi ck_lo]
 * ------------------------------------------------------------------------- */
#define FP_BAUD 57600
#define FP_SLOTS 5   /* max stored fingerprints */

static void uart_init(void)
{
    const uart_cfg_t uart_cfg = {
        .parity     = UART_PARITY_DISABLE,
        .size       = UART_DATA_SIZE_8_BITS,
        .extra_stop = 0,
        .cts        = 0,
        .rts        = 0,
        .baud       = FP_BAUD,
    };
    const sys_cfg_uart_t uart_sys_cfg = {
        .clk_scale = CLKMAN_SCALE_DIV_1,
        .io_cfg    = IOMAN_UART(2, IOMAN_MAP_A, IOMAN_MAP_A, IOMAN_MAP_A, 1, 0, 0),
    };
    UART_Init(MXC_UART2, &uart_cfg, &uart_sys_cfg);
}

static int fp_read_byte(uint8_t *b)
{
    for (uint32_t i = 0; i < 6000000; i++) {
        if (UART_NumReadAvail(MXC_UART2)) {
            int n = 0;
            UART_Read(MXC_UART2, b, 1, &n);
            return (n == 1) ? 0 : -1;
        }
    }
    return -1;
}

static void fp_send(const uint8_t *data, int len)
{
    const uint8_t header[2] = { 0xEF, 0x01 };
    const uint8_t addr[4]   = { 0xFF, 0xFF, 0xFF, 0xFF };
    uint8_t  pid    = 0x01;
    uint16_t length = (uint16_t)(len + 2);
    uint8_t  lbuf[2] = { (uint8_t)(length >> 8), (uint8_t)(length & 0xFF) };
    uint16_t ck = pid + lbuf[0] + lbuf[1];
    for (int i = 0; i < len; i++) ck += data[i];
    uint8_t ckbuf[2] = { (uint8_t)(ck >> 8), (uint8_t)(ck & 0xFF) };

    UART_Write(MXC_UART2, (uint8_t *)header, 2);
    UART_Write(MXC_UART2, (uint8_t *)addr,   4);
    UART_Write(MXC_UART2, &pid,               1);
    UART_Write(MXC_UART2, lbuf,               2);
    UART_Write(MXC_UART2, (uint8_t *)data,   len);
    UART_Write(MXC_UART2, ckbuf,              2);
}

/* Read ACK. Returns confirm byte, or 0xFF on timeout/framing error.
 * If extra != NULL, fills it with the next 2 payload bytes (e.g. found_id). */
static uint8_t fp_recv_ack(uint16_t *extra)
{
    uint8_t b;
    for (;;) {
        if (fp_read_byte(&b) < 0) return 0xFF;
        if (b != 0xEF) continue;
        if (fp_read_byte(&b) < 0) return 0xFF;
        if (b == 0x01) break;
    }
    for (int i = 0; i < 5; i++) {
        if (fp_read_byte(&b) < 0) return 0xFF;
    }
    uint8_t lh, ll;
    if (fp_read_byte(&lh) < 0) return 0xFF;
    if (fp_read_byte(&ll) < 0) return 0xFF;
    int payload = ((int)lh << 8) | ll;
    if (payload < 2) return 0xFF;

    if (fp_read_byte(&b) < 0) return 0xFF;
    uint8_t confirm = b;
    int remaining = payload - 1;

    if (extra && remaining >= 2) {
        uint8_t hi = 0, lo = 0;
        fp_read_byte(&hi);
        fp_read_byte(&lo);
        *extra = ((uint16_t)hi << 8) | lo;
        remaining -= 2;
    } else if (extra) {
        *extra = 0;
    }
    for (int i = 0; i < remaining; i++) fp_read_byte(&b);
    return confirm;
}

static uint8_t fp_verify_password(void)
{
    const uint8_t c[] = { 0x13, 0x00, 0x00, 0x00, 0x00 };
    fp_send(c, sizeof(c));
    return fp_recv_ack(NULL);
}

static uint8_t fp_get_image(void)
{
    const uint8_t c[] = { 0x01 };
    fp_send(c, sizeof(c));
    return fp_recv_ack(NULL);
}

static uint8_t fp_img2tz(uint8_t slot)
{
    const uint8_t c[] = { 0x02, slot };
    fp_send(c, sizeof(c));
    return fp_recv_ack(NULL);
}

static uint8_t fp_reg_model(void)
{
    const uint8_t c[] = { 0x05 };
    fp_send(c, sizeof(c));
    return fp_recv_ack(NULL);
}

static uint8_t fp_store(uint16_t id)
{
    const uint8_t c[] = { 0x06, 0x01, (uint8_t)(id >> 8), (uint8_t)(id & 0xFF) };
    fp_send(c, sizeof(c));
    return fp_recv_ack(NULL);
}

/* Search all stored templates against char buffer 1.
 * Returns 0x00 on match; fills *found_id with the matched page (1–FP_SLOTS). */
static uint8_t fp_search(uint16_t *found_id)
{
    const uint8_t c[] = { 0x04, 0x01, 0x00, 0x00, 0x00, 0xA2 };
    fp_send(c, sizeof(c));
    return fp_recv_ack(found_id);
}

/* Wipe all stored templates from sensor flash. Returns 0x00 on success. */
static uint8_t fp_empty(void)
{
    const uint8_t c[] = { 0x0D };
    fp_send(c, sizeof(c));
    return fp_recv_ack(NULL);
}

/* -------------------------------------------------------------------------
 * Enroll one finger into flash slot (1–FP_SLOTS).
 * LED feedback: blue=place finger, green=sample OK, purple=merging, red=error.
 * Button press during wait cancels and returns 0.
 * Returns 1 on success, 0 on failure/cancel.
 * ------------------------------------------------------------------------- */
static int enroll_finger(uint16_t slot)
{
    /* --- Sample 1 --- */
    led_set(0, 0, 1);
    while (1) {
        if (enc_button()) return 0;
        uint8_t r = fp_get_image();
        if (r == 0x00) break;
        if (r == 0x02) continue;
    }
    if (fp_img2tz(1) != 0x00) {
        led_set(1, 0, 0);
        TMR_Delay(MXC_TMR0, MSEC(500));
        return 0;
    }
    led_set(0, 1, 0);
    TMR_Delay(MXC_TMR0, MSEC(400));

    /* Wait for lift */
    led_set(0, 0, 0);
    TMR_Delay(MXC_TMR0, MSEC(400));
    while (fp_get_image() != 0x02) {
        if (enc_button()) return 0;
    }

    /* --- Sample 2 --- */
    TMR_Delay(MXC_TMR0, MSEC(200));
    led_set(0, 0, 1);
    while (1) {
        if (enc_button()) return 0;
        uint8_t r = fp_get_image();
        if (r == 0x00) break;
        if (r == 0x02) continue;
    }
    if (fp_img2tz(2) != 0x00) {
        led_set(1, 0, 0);
        TMR_Delay(MXC_TMR0, MSEC(500));
        return 0;
    }
    led_set(0, 1, 0);
    TMR_Delay(MXC_TMR0, MSEC(400));

    /* --- Merge and store --- */
    led_set(1, 0, 1);
    if (fp_reg_model() != 0x00 || fp_store(slot) != 0x00) {
        led_set(1, 0, 0);
        TMR_Delay(MXC_TMR0, MSEC(500));
        return 0;
    }

    /* Success — show slot number for 2 s */
    led_set(0, 1, 0);
    max7219_show_hex((uint8_t)slot);
    TMR_Delay(MXC_TMR0, MSEC(2000));
    return 1;
}

/* -------------------------------------------------------------------------
 * Application
 *
 * IDLE state:
 *   Encoder CW/CCW scrolls 0–F on display.
 *   Finger placed → search → show slot number (1–5) or flash X ×3.
 *   At value F, press encoder button → enter ENROLL state.
 *
 * ENROLL state:
 *   Display shows "E".  Blue LED = waiting for finger.
 *   Place finger twice → stored at next slot; display shows slot number.
 *   Can enroll up to FP_SLOTS fingers before auto-returning to IDLE.
 *   Press button at any time to cancel and return to IDLE.
 * ------------------------------------------------------------------------- */
typedef enum { STATE_IDLE, STATE_ENROLL } app_state_t;

int main(void)
{
    led_init();
    uart_init();
    max7219_init();
    enc_init();

    TMR_Delay(MXC_TMR0, MSEC(200)); /* sensor POR */

    if (fp_verify_password() == 0x00) {
        led_set(0, 0, 1);
    } else {
        while (1) { show_x_flash(1); TMR_Delay(MXC_TMR0, MSEC(400)); }
    }
    TMR_Delay(MXC_TMR0, MSEC(500));

    max7219_show_hex(0);
    led_set(0, 0, 1);

    app_state_t state    = STATE_IDLE;
    uint8_t     next_slot = 1;

    while (1) {

        /* ---- IDLE ---- */
        if (state == STATE_IDLE) {

            if (enc_poll())
                max7219_show_hex(enc_value);

            /* 0 + button press → wipe all slots */
            if (enc_value == 0x0 && enc_button()) {
                while (enc_button()) {}
                TMR_Delay(MXC_TMR0, MSEC(150));
                if (fp_empty() == 0x00) {
                    next_slot = 1;
                    led_set(0, 1, 0);
                    for (uint8_t r = 1; r <= 8; r++) max7219_write(r, 0xFF); /* all on */
                    TMR_Delay(MXC_TMR0, MSEC(500));
                    for (uint8_t r = 1; r <= 8; r++) max7219_write(r, 0x00); /* all off */
                    TMR_Delay(MXC_TMR0, MSEC(300));
                } else {
                    show_x_flash(3); /* wipe failed */
                }
                max7219_show_hex(enc_value);
                led_set(0, 0, 1);
                continue;
            }

            /* F + button press → enroll */
            if (enc_value == 0xF && enc_button()) {
                while (enc_button()) {}
                TMR_Delay(MXC_TMR0, MSEC(150));

                if (next_slot > FP_SLOTS) {
                    show_x_flash(3); /* all slots full */
                    max7219_show_hex(enc_value);
                    led_set(0, 0, 1);
                } else {
                    state = STATE_ENROLL;
                    max7219_show_hex(0xE);
                    led_set(0, 0, 1);
                }
                continue;
            }

            /* Check for finger */
            uint8_t r = fp_get_image();
            if (r != 0x00) continue; /* 0x02 = no finger, others = error */

            if (fp_img2tz(1) == 0x00) {
                uint16_t found_id = 0;
                if (fp_search(&found_id) == 0x00 && found_id >= 1 && found_id <= FP_SLOTS) {
                    max7219_show_hex((uint8_t)found_id);
                    led_set(0, 1, 0);
                    TMR_Delay(MXC_TMR0, MSEC(2000));
                } else {
                    show_x_flash(3);
                }
            } else {
                show_x_flash(3);
            }

            /* restore display and wait for finger to lift */
            max7219_show_hex(enc_value);
            led_set(0, 0, 1);
            while (fp_get_image() != 0x02) {}
            TMR_Delay(MXC_TMR0, MSEC(300));

        /* ---- ENROLL ---- */
        } else {

            /* Button press exits enroll */
            if (enc_button()) {
                while (enc_button()) {}
                TMR_Delay(MXC_TMR0, MSEC(150));
                state = STATE_IDLE;
                max7219_show_hex(enc_value);
                led_set(0, 0, 1);
                continue;
            }

            if (enroll_finger(next_slot)) {
                next_slot++;
                if (next_slot > FP_SLOTS) {
                    state = STATE_IDLE; /* all slots filled — return automatically */
                    max7219_show_hex(enc_value);
                } else {
                    max7219_show_hex(0xE); /* ready for next finger */
                }
            } else {
                max7219_show_hex(0xE); /* failed/cancelled — stay in enroll */
            }
            led_set(0, 0, 1);
        }
    }
}
