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
 * Application: R → G → B blink at 200ms each, with debug output.
 * ------------------------------------------------------------------------- */
int main(void)
{
    Board_Init();
    debug_init();
    led_init();

    dbg("\r\n[BOOT] sentinel-box blink init\r\n");

    while (1) {
        dbg("[LED] red\r\n");
        led_set(1, 0, 0);
        TMR_Delay(MXC_TMR0, MSEC(200));

        dbg("[LED] green\r\n");
        led_set(0, 1, 0);
        TMR_Delay(MXC_TMR0, MSEC(200));

        dbg("[LED] blue\r\n");
        led_set(0, 0, 1);
        TMR_Delay(MXC_TMR0, MSEC(200));
    }
}
