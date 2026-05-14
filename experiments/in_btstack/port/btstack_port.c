/*
 * btstack_port.c — MAX32630FTHR platform bring-up for BTstack + CC2564B.
 *
 * Based on the official BTstack port at third_party/btstack/port/max32630-fthr/.
 * Implements the hal_uart_dma interface consumed by btstack_uart_block_embedded.c,
 * plus hal_cpu and hal_time_ms.
 *
 * The UART HAL is polling-based (same as the official port): the main loop
 * calls hal_btstack_run_loop_execute_once() which drains RX/TX FIFOs and
 * then ticks the BTstack embedded run loop.
 */

#include <string.h>
#include "mxc_config.h"
#include "gpio.h"
#include "i2cm.h"
#include "ioman.h"
#include "ioman_regs.h"
#include "clkman.h"
#include "uart.h"
#include "tmr_utils.h"
#include "pwrseq_regs.h"
#include "rtc_regs.h"
#include "mxc_sys.h"

#include "btstack.h"
#include "btstack_run_loop_embedded.h"
#include "hci_transport.h"
#include "hci_transport_h4.h"
#include "btstack_chipset_cc256x.h"
#include "hal_uart_dma.h"

#include "btstack_port.h"

int btstack_main(int argc, const char *argv[]);

#define BLE_UART_ID         0
#define UART_RXFIFO_USABLE  (MXC_UART_FIFO_DEPTH - 3)

/* -------------------------------------------------------------------------
 * hal_uart_dma — polling implementation used by btstack_uart_block_embedded
 * (matches the official BTstack MAX32630FTHR port structure)
 * ------------------------------------------------------------------------- */
static void dummy_handler(void) {}
static void (*s_rx_done)(void) = dummy_handler;
static void (*s_tx_done)(void) = dummy_handler;
static uint8_t *s_rx_buf;
static int      s_rx_len;
static uint8_t *s_tx_buf;
static int      s_tx_len;

static uint32_t s_baud_rate;

void hal_uart_dma_init(void)
{
    s_rx_len = 0;
    s_tx_len = 0;
    hal_uart_dma_set_baud(115200);
}

int hal_uart_dma_set_baud(uint32_t baud)
{
    s_baud_rate = baud;
    const uart_cfg_t cfg = {
        .parity=UART_PARITY_DISABLE, .size=UART_DATA_SIZE_8_BITS,
        .extra_stop=0, .cts=1, .rts=1, .baud=baud,
    };
    const sys_cfg_uart_t sys = {
        .clk_scale=CLKMAN_SCALE_AUTO,
        .io_cfg=IOMAN_UART(BLE_UART_ID, IOMAN_MAP_B, IOMAN_MAP_B, IOMAN_MAP_B, 1, 1, 1),
    };
    mxc_uart_regs_t *u = MXC_UART_GET_UART(BLE_UART_ID);
    UART_Init(u, &cfg, &sys);
    /* Active-high CTS/RTS polarity to match CC2564B; RTS deasserts near FIFO full */
    u->ctrl |= MXC_F_UART_CTRL_CTS_POLARITY | MXC_F_UART_CTRL_RTS_POLARITY;
    u->ctrl &= ~MXC_F_UART_CTRL_RTS_LEVEL;
    u->ctrl |= (UART_RXFIFO_USABLE << MXC_F_UART_CTRL_RTS_LEVEL_POS);
    console_write("[uart] baud=");
    console_write_u16dec((uint16_t)(baud / 100));
    console_write("00\r\n");
    return (int)baud;
}

void hal_uart_dma_set_block_received(void (*h)(void)) { s_rx_done = h; }
void hal_uart_dma_set_block_sent(void (*h)(void))     { s_tx_done = h; }
void hal_uart_dma_set_csr_irq_handler(void (*h)(void)){ (void)h; }
void hal_uart_dma_set_sleep(uint8_t s)                { (void)s; }

void hal_uart_dma_receive_block(uint8_t *buf, uint16_t len)
{
    s_rx_buf = buf;
    s_rx_len = (int)len;
}

void hal_uart_dma_send_block(const uint8_t *buf, uint16_t len)
{
    s_tx_buf = (uint8_t *)buf;
    s_tx_len = (int)len;
}

/* -------------------------------------------------------------------------
 * hal_cpu — required by btstack_run_loop_embedded
 * ------------------------------------------------------------------------- */
void hal_cpu_disable_irqs(void)          { __disable_irq(); }
void hal_cpu_enable_irqs(void)           { __enable_irq(); }
void hal_cpu_enable_irqs_and_sleep(void) { __enable_irq(); }

/* -------------------------------------------------------------------------
 * hal_time_ms — SysTick at 1 kHz
 * ------------------------------------------------------------------------- */
static volatile uint32_t s_ms;
void SysTick_Handler(void) { s_ms++; }
uint32_t hal_time_ms(void) { return s_ms; }

/* -------------------------------------------------------------------------
 * Debug console — UART1 MAP_A (P2.1 TX / DAPLink CDC) at 115200
 * Public: main.c uses console_write() for app-level diagnostics.
 * ------------------------------------------------------------------------- */
void console_write(const char *s)
{
    UART_Write(MXC_UART1, (uint8_t *)s, strlen(s));
}

void console_write_u8hex(uint8_t v)
{
    static const char h[] = "0123456789abcdef";
    uint8_t b[2] = { h[v >> 4], h[v & 0xf] };
    UART_Write(MXC_UART1, b, 2);
}

void console_write_u16dec(uint16_t v)
{
    char b[6]; int i = 5; b[5] = 0;
    if (!v) { UART_Write(MXC_UART1, (uint8_t *)"0", 1); return; }
    while (v) { b[--i] = '0' + v % 10; v /= 10; }
    UART_Write(MXC_UART1, (uint8_t *)(b + i), 5 - i);
}

volatile uint8_t ble_io_log = 0;

static void debug_init(void)
{
    const uart_cfg_t cfg = {
        .parity=UART_PARITY_DISABLE, .size=UART_DATA_SIZE_8_BITS,
        .extra_stop=0, .cts=0, .rts=0, .baud=115200,
    };
    const sys_cfg_uart_t sys = {
        .clk_scale=CLKMAN_SCALE_DIV_1,
        .io_cfg=IOMAN_UART(1, IOMAN_MAP_A, IOMAN_MAP_A, IOMAN_MAP_A, 1, 0, 0),
    };
    UART_Init(MXC_UART1, &cfg, &sys);
}

/* -------------------------------------------------------------------------
 * PMIC — MAX14690 on I2CM2; set LDO2 + LDO3 to 3.3 V for VDDIOH (Port 2)
 * ------------------------------------------------------------------------- */
static void pmic_init(void)
{
    const sys_cfg_i2cm_t cfg = {
        .clk_scale=CLKMAN_SCALE_DIV_1,
        .io_cfg=IOMAN_I2CM2(IOMAN_MAP_A, 1),
    };
    if (I2CM_Init(MXC_I2CM2, &cfg, I2CM_SPEED_100KHZ) != E_NO_ERROR) return;
    uint8_t cmds[][2] = { {0x15,25},{0x14,2},{0x17,25},{0x16,2} };
    for (int i = 0; i < 4; i++)
        I2CM_Write(MXC_I2CM2, 0x28, NULL, 0, cmds[i], 2);
}

/* -------------------------------------------------------------------------
 * BLE hardware — 32 kHz slow clock (CC2564B reference) + nSHUTD toggle
 * UART0 is NOT initialised here; hal_uart_dma_set_baud() does it when
 * BTstack opens the HCI transport.
 * ------------------------------------------------------------------------- */
static const gpio_cfg_t ble_nshutd = {PORT_1, PIN_6, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL};
static const gpio_cfg_t ble_hcirts = {PORT_0, PIN_3, GPIO_FUNC_GPIO, GPIO_PAD_INPUT_PULLUP};

static void bt_comm_init(void)
{
    /* BLE UART pins (Port 0/1) use core VDDIO, not VDDIOH */
    MXC_IOMAN->use_vddioh_0 &= ~(PIN_0|PIN_1|PIN_2|PIN_3);
    MXC_IOMAN->use_vddioh_1 &= ~(PIN_6|PIN_7);

    /* Configure HCI RTS as input so we can poll for module ready */
    GPIO_Config(&ble_hcirts);

    /* Start 32.768 kHz nano-ring oscillator — feeds CC2564B slow clock via P1.7 */
    MXC_RTCCFG->clk_ctrl |= MXC_F_RTC_CLK_CTRL_NANO_EN;
    MXC_RTCCFG->osc_ctrl |= MXC_F_RTC_OSC_CTRL_OSC_WARMUP_ENABLE;
    MXC_PWRSEQ->reg4     |= MXC_F_PWRSEQ_REG4_PWR_PSEQ_32K_EN;
    TMR_Delay(MXC_TMR0, MSEC(50));

    /* Assert nSHUTD low to reset CC2564B, then release to start it */
    GPIO_Config(&ble_nshutd);
    GPIO_OutClr(&ble_nshutd);
    TMR_Delay(MXC_TMR0, MSEC(10));
    GPIO_OutSet(&ble_nshutd);

    /* Wait for the module to pull HCI RTS low (ready to receive) */
    console_write("[boot] waiting for CC2564B RTS...\r\n");
    while (GPIO_InGet(&ble_hcirts)) { /* spin */ }
    console_write("[boot] CC2564B ready\r\n");
}

/* -------------------------------------------------------------------------
 * hal_btstack_run_loop_execute_once — call from main() while(1)
 *
 * Matches the official BTstack MAX32630FTHR port structure:
 *   1. Drain RX FIFO → notify BTstack when a block is complete
 *   2. Drain TX FIFO → notify BTstack when a block is sent
 *   3. Tick the BTstack embedded run loop
 * ------------------------------------------------------------------------- */
void hal_btstack_run_loop_execute_once(void)
{
    mxc_uart_regs_t *uart = MXC_UART_GET_UART(BLE_UART_ID);

    /* Drain RX */
    while (s_rx_len > 0) {
        int rx_avail = UART_NumReadAvail(uart);
        if (!rx_avail) break;

        int n = (s_rx_len < rx_avail) ? s_rx_len : rx_avail;
        int got = 0;
        UART_Read(uart, s_rx_buf, n, &got);
        if (got <= 0) break;

        s_rx_buf += got;
        s_rx_len -= got;

        if (s_rx_len <= 0) {
            s_rx_len = 0;
            (*s_rx_done)();
        }
    }

    /* Drain TX */
    while (s_tx_len > 0) {
        int tx_avail = UART_NumWriteAvail(uart);
        if (!tx_avail) break;

        int n = (s_tx_len < tx_avail) ? s_tx_len : tx_avail;
        int ret = UART_Write(uart, s_tx_buf, n);
        if (ret < 0) break;

        s_tx_buf += n;
        s_tx_len -= n;

        if (s_tx_len <= 0) {
            s_tx_len = 0;
            (*s_tx_done)();
        }
    }

    btstack_run_loop_embedded_execute_once();
}

/* -------------------------------------------------------------------------
 * HCI transport config — 115200 init, renegotiate to 4 Mbps after init
 * Hardware CTS/RTS flow control enabled.
 * ------------------------------------------------------------------------- */
static const hci_transport_config_uart_t hci_cfg = {
    HCI_TRANSPORT_CONFIG_UART,
    115200,   /* baudrate_init */
    4000000,  /* baudrate_main — renegotiate after CC256X init */
    1,        /* flowcontrol: hardware CTS/RTS */
    NULL,
};

/* -------------------------------------------------------------------------
 * bluetooth_main — full platform + BTstack bring-up; calls btstack_main()
 * Called from main() before entering the run loop.
 * ------------------------------------------------------------------------- */
void bluetooth_main(void)
{
    debug_init();
    console_write("\r\n[boot] start\r\n");

    SysTick_Config(SystemCoreClock / 1000);   /* 1 ms SysTick for hal_time_ms */

    console_write("[boot] pmic\r\n");
    pmic_init();

    console_write("[boot] bt_comm\r\n");
    bt_comm_init();

    console_write("[boot] btstack init\r\n");
    btstack_memory_init();
    btstack_run_loop_init(btstack_run_loop_embedded_get_instance());

    const hci_transport_t *t =
        hci_transport_h4_instance(btstack_uart_block_embedded_instance());
    hci_init(t, &hci_cfg);
    hci_set_chipset(btstack_chipset_cc256x_instance());

    console_write("[boot] btstack_main\r\n");
    btstack_main(0, NULL);
    console_write("[boot] run loop\r\n");
}
