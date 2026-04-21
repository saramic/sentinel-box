#include <stddef.h>
#include "mxc_config.h"
#include "gpio.h"
#include "tmr_utils.h"
#include "i2cm.h"
#include "ioman.h"
#include "clkman.h"

/*
 * Override the weak Board_Init() from EvKit_V1/board.c.
 * On the FTHR the PMIC (MAX14690) is on I2CM2, not I2CM0 — the EvKit init
 * hangs because it tries I2CM0. We init I2CM2 directly and bring up LDO2 and
 * LDO3 to 3.3V so the board boots standalone after a cold power cycle.
 *
 * Register values (from LPSDK max14690.c / board.c):
 *   LDO2_VSET 0x15 = (3300-800)/100 = 25   LDO2_CFG  0x14 = 2 (LDO_ENABLED)
 *   LDO3_VSET 0x17 = 25                    LDO3_CFG  0x16 = 2 (LDO_ENABLED)
 */
int Board_Init(void)
{
    const sys_cfg_i2cm_t i2cm2_cfg = {
        .clk_scale = CLKMAN_SCALE_DIV_1,
        .io_cfg = IOMAN_I2CM2(IOMAN_MAP_A, 1)
    };
    if (I2CM_Init(MXC_I2CM2, &i2cm2_cfg, I2CM_SPEED_100KHZ) != E_NO_ERROR)
        return E_NO_ERROR; /* soft-fail: LED still blinks without LDOs */

    /* MAX14690 I2C address 0x28; each write is [reg, value] */
    uint8_t ldo2_vset[2] = {0x15, 25};
    uint8_t ldo2_cfg[2]  = {0x14,  2};
    uint8_t ldo3_vset[2] = {0x17, 25};
    uint8_t ldo3_cfg[2]  = {0x16,  2};

    I2CM_Write(MXC_I2CM2, 0x28, NULL, 0, ldo2_vset, 2);
    I2CM_Write(MXC_I2CM2, 0x28, NULL, 0, ldo2_cfg,  2);
    I2CM_Write(MXC_I2CM2, 0x28, NULL, 0, ldo3_vset, 2);
    I2CM_Write(MXC_I2CM2, 0x28, NULL, 0, ldo3_cfg,  2);

    return E_NO_ERROR;
}

/* MAX32630FTHR RGB LED — active-low, open-drain */
static const gpio_cfg_t led_r = { PORT_2, PIN_4, GPIO_FUNC_GPIO, GPIO_PAD_OPEN_DRAIN };
static const gpio_cfg_t led_g = { PORT_2, PIN_5, GPIO_FUNC_GPIO, GPIO_PAD_OPEN_DRAIN };
static const gpio_cfg_t led_b = { PORT_2, PIN_6, GPIO_FUNC_GPIO, GPIO_PAD_OPEN_DRAIN };

static void leds_init(void)
{
    GPIO_Config(&led_r);
    GPIO_Config(&led_g);
    GPIO_Config(&led_b);
    GPIO_OutSet(&led_r);
    GPIO_OutSet(&led_g);
    GPIO_OutSet(&led_b);
}

int main(void)
{
    leds_init();

    while (1) {
        GPIO_OutClr(&led_r);
        TMR_Delay(MXC_TMR0, MSEC(500));
        GPIO_OutSet(&led_r);

        GPIO_OutClr(&led_g);
        TMR_Delay(MXC_TMR0, MSEC(500));
        GPIO_OutSet(&led_g);

        GPIO_OutClr(&led_b);
        TMR_Delay(MXC_TMR0, MSEC(500));
        GPIO_OutSet(&led_b);
    }
}
