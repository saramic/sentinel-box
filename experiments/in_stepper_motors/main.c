#include <stdint.h>
#include <stddef.h>
#include "mxc_config.h"
#include "gpio.h"
#include "tmr_utils.h"
#include "i2cm.h"
#include "ioman.h"
#include "clkman.h"

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
 * 28BYJ-48 stepper via ULN2003 driver board — half-step mode.
 *
 * Pin mapping (avoids all fingerprint-LPSDK wiring on P3.0-P3.5, P5.3-P5.5):
 *   IN1 = P5.2   IN2 = P5.1   IN3 = P5.0   IN4 = P4.0
 *
 * Half-step table — 8 states, {IN1,IN2,IN3,IN4}:
 *   0:1000  1:1100  2:0100  3:0110
 *   4:0010  5:0011  6:0001  7:1001
 *
 * Forward (+steps): index advances 0→1→…→7→0
 * Reverse  (-steps): index retreats 0→7→…→1→0
 *
 * Gear ratio ≈ 64:1, internal motor 64 half-steps/rev
 * → 4096 half-steps per output-shaft revolution
 * → STEPS_90 = 1024 half-steps ≈ 90°
 * ------------------------------------------------------------------------- */
static const gpio_cfg_t s_in1 = { PORT_5, PIN_2, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };
static const gpio_cfg_t s_in2 = { PORT_5, PIN_1, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };
static const gpio_cfg_t s_in3 = { PORT_5, PIN_0, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };
static const gpio_cfg_t s_in4 = { PORT_4, PIN_0, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };

static const uint8_t half_step[8][4] = {
    { 1, 0, 0, 0 },
    { 1, 1, 0, 0 },
    { 0, 1, 0, 0 },
    { 0, 1, 1, 0 },
    { 0, 0, 1, 0 },
    { 0, 0, 1, 1 },
    { 0, 0, 0, 1 },
    { 1, 0, 0, 1 },
};

static int step_idx = 0;

static void stepper_apply(void)
{
    const uint8_t *s = half_step[step_idx];
    s[0] ? GPIO_OutSet(&s_in1) : GPIO_OutClr(&s_in1);
    s[1] ? GPIO_OutSet(&s_in2) : GPIO_OutClr(&s_in2);
    s[2] ? GPIO_OutSet(&s_in3) : GPIO_OutClr(&s_in3);
    s[3] ? GPIO_OutSet(&s_in4) : GPIO_OutClr(&s_in4);
}

static void stepper_off(void)
{
    GPIO_OutClr(&s_in1);
    GPIO_OutClr(&s_in2);
    GPIO_OutClr(&s_in3);
    GPIO_OutClr(&s_in4);
}

static void stepper_init(void)
{
    GPIO_Config(&s_in1); GPIO_OutClr(&s_in1);
    GPIO_Config(&s_in2); GPIO_OutClr(&s_in2);
    GPIO_Config(&s_in3); GPIO_OutClr(&s_in3);
    GPIO_Config(&s_in4); GPIO_OutClr(&s_in4);
}

/* 3 ms per half-step → reliable torque throughout the move.
 * Going below ~2 ms risks losing steps on direction reversal. */
#define STEP_DELAY_US   3000
#define STEPS_PER_REV   4096
#define STEPS_90        (STEPS_PER_REV / 4)   /* 1024 half-steps ≈ 90° */

/* positive steps = forward, negative = reverse.
 * Coils remain energised after returning so the gear train stays meshed —
 * call stepper_off() explicitly if you want to save power / reduce heat. */
static void stepper_move(int steps)
{
    int dir = (steps > 0) ? 1 : -1;
    int n   = (steps > 0) ? steps : -steps;
    for (int i = 0; i < n; i++) {
        step_idx = (step_idx + dir + 8) % 8;
        stepper_apply();
        TMR_Delay(MXC_TMR0, USEC(STEP_DELAY_US));
    }
    /* intentionally no stepper_off() here: holding the last coil state keeps
     * the gearbox meshed so the next move (especially a direction reversal)
     * starts with torque rather than fighting backlash, which causes vibration */
}

/* -------------------------------------------------------------------------
 * Application: spin 90° left, pause 1 s, spin 90° right, pause 1 s, repeat.
 *
 * LED colours:
 *   Blue  — moving left  (reverse)
 *   Green — moving right (forward)
 *   Off   — pausing between moves
 * ------------------------------------------------------------------------- */
int main(void)
{
    led_init();
    stepper_init();

    for (int i = 0; i < 3; i++) {
        led_set(0, 1, 0);              /* green: forward */
        stepper_move(+STEPS_PER_REV);
        TMR_Delay(MXC_TMR0, MSEC(500));

        led_set(0, 0, 1);              /* blue: reverse */
        stepper_move(-STEPS_PER_REV);
        TMR_Delay(MXC_TMR0, MSEC(500));
    }

    stepper_off();
    led_set(1, 0, 0);                  /* red: done */
    while (1) {}
}
