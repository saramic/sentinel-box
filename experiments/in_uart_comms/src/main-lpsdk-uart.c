// MAX32630FTHR - LPSDK UART2 communicator
// Pairs with main-esp32-uart.cpp (ESP32 side)
//
// Wiring (3.3V — no level shifting needed):
//   MAX32630 P3.1 (UART2 TX)  -->  ESP32 GPIO16 (RX2)
//   MAX32630 P3.0 (UART2 RX)  <--  ESP32 GPIO17 (TX2)
//   GND                        ---  GND
//
// Debug output goes to UART1 on P2.0(RX)/P2.1(TX) at 115200
// (USB-to-serial bridge on MAX32630FTHR)

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "mxc_config.h"
#include "gpio.h"
#include "tmr_utils.h"
#include "i2cm.h"
#include "ioman.h"
#include "clkman.h"
#include "uart.h"

#define UART_BAUD   57600
#define MSG_START   0x02    // ASCII STX
#define MSG_END     0x03    // ASCII ETX
#define BUF_SIZE    128
#define TICK_MS     10      // loop cadence
#define TICKS_PER_PING  100 // 10ms * 100 = ~1s

static uint8_t  rx_buf[BUF_SIZE];
static int      rx_pos     = 0;
static int      in_message = 0;
static uint32_t ping_count = 0;

// -------------------------------------------------------------------------
// Board init — PMIC (MAX14690) via I2CM2, LDO2+LDO3 → 3.3V.
// -------------------------------------------------------------------------
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

// -------------------------------------------------------------------------
// Debug: UART1 on P2.0(RX) / P2.1(TX) — USB-to-serial bridge on FTHR
// -------------------------------------------------------------------------
static void debug_init(void)
{
    const uart_cfg_t cfg = {
        .parity     = UART_PARITY_DISABLE,
        .size       = UART_DATA_SIZE_8_BITS,
        .extra_stop = 0,
        .cts        = 0,
        .rts        = 0,
        .baud       = 115200,
    };
    const sys_cfg_uart_t sys_cfg = {
        .clk_scale = CLKMAN_SCALE_DIV_1,
        .io_cfg    = IOMAN_UART(1, IOMAN_MAP_A, IOMAN_MAP_A, IOMAN_MAP_A, 1, 0, 0),
    };
    UART_Init(MXC_UART1, &cfg, &sys_cfg);
}

static void debug_print(const char *s)
{
    UART_Write(MXC_UART1, (uint8_t *)s, strlen(s));
}

// -------------------------------------------------------------------------
// Device UART: UART2 on P3.0(RX) / P3.1(TX) — connects to ESP32
// -------------------------------------------------------------------------
static void uart2_init(void)
{
    const uart_cfg_t cfg = {
        .parity     = UART_PARITY_DISABLE,
        .size       = UART_DATA_SIZE_8_BITS,
        .extra_stop = 0,
        .cts        = 0,
        .rts        = 0,
        .baud       = UART_BAUD,
    };
    const sys_cfg_uart_t sys_cfg = {
        .clk_scale = CLKMAN_SCALE_DIV_1,
        .io_cfg    = IOMAN_UART(2, IOMAN_MAP_A, IOMAN_MAP_A, IOMAN_MAP_A, 1, 0, 0),
    };
    UART_Init(MXC_UART2, &cfg, &sys_cfg);
}

// -------------------------------------------------------------------------
// Send a framed message over UART2
// -------------------------------------------------------------------------
static void send_message(const char *msg)
{
    uint8_t start = MSG_START;
    uint8_t end   = MSG_END;
    UART_Write(MXC_UART2, &start, 1);
    UART_Write(MXC_UART2, (uint8_t *)msg, strlen(msg));
    UART_Write(MXC_UART2, &end, 1);

    debug_print("[TX] --> ");
    debug_print(msg);
    debug_print("\r\n");
}

// -------------------------------------------------------------------------
// Handle a complete framed message received from UART2
// -------------------------------------------------------------------------
static void handle_message(uint8_t *buf, int len)
{
    buf[len] = '\0';
    debug_print("[RX] <-- ");
    debug_print((char *)buf);
    debug_print("\r\n");
}

// -------------------------------------------------------------------------
// Non-blocking drain of UART2 RX FIFO — call on every loop tick
// -------------------------------------------------------------------------
static void read_incoming(void)
{
    while (UART_NumReadAvail(MXC_UART2)) {
        uint8_t b;
        int     n = 0;
        UART_Read(MXC_UART2, &b, 1, &n);
        if (n != 1)
            break;

        if (b == MSG_START) {
            rx_pos     = 0;
            in_message = 1;

        } else if (b == MSG_END) {
            if (in_message && rx_pos > 0)
                handle_message(rx_buf, rx_pos);
            rx_pos     = 0;
            in_message = 0;

        } else if (in_message) {
            if (rx_pos < BUF_SIZE - 1) {
                rx_buf[rx_pos++] = b;
            } else {
                debug_print("[WARN] RX overflow, discarding\r\n");
                rx_pos     = 0;
                in_message = 0;
            }
        }
    }
}

// -------------------------------------------------------------------------
int main(void)
{
    debug_init();
    uart2_init();

    debug_print("[DEBUG] MAX32630FTHR LPSDK UART bridge ready\r\n");

    int tick = 0;

    while (1) {
        read_incoming();

        // TMR_Delay is blocking for TICK_MS but the hardware RX FIFO
        // (32 bytes) holds incoming bytes while we wait.
        TMR_Delay(MXC_TMR0, MSEC(TICK_MS));

        if (++tick >= TICKS_PER_PING) {
            tick = 0;
            char msg[32];
            snprintf(msg, sizeof(msg), "PING %lu", ping_count++);
            send_message(msg);
        }
    }
}
