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
 * Stepper mode — uncomment exactly one line.
 *
 * Mode        Steps/rev   Delay    Speed
 * HALF_STEP   4096        1.5 ms  ~10 RPM  smooth, reliable  ← default
 * FULL_STEP   2048        1.5 ms  ~20 RPM  faster, less smooth
 *
 * STEP_DELAY_US can be tuned independently. Below ~1000 µs the 28BYJ-48 stalls.
 * ------------------------------------------------------------------------- */
#define STEPPER_HALF_STEP
/* #define STEPPER_FULL_STEP */

#if defined(STEPPER_HALF_STEP)
#  define STEP_DELAY_US   1500
#  define STEPS_PER_REV   4096
#  define STEP_TABLE_LEN  8
static const uint8_t step_table[8][4] = {
    { 1, 0, 0, 0 },
    { 1, 1, 0, 0 },
    { 0, 1, 0, 0 },
    { 0, 1, 1, 0 },
    { 0, 0, 1, 0 },
    { 0, 0, 1, 1 },
    { 0, 0, 0, 1 },
    { 1, 0, 0, 1 },
};
#elif defined(STEPPER_FULL_STEP)
#  define STEP_DELAY_US   1500
#  define STEPS_PER_REV   2048
#  define STEP_TABLE_LEN  4
/* Wave drive: same single-coil states as half-step, just skipping the in-between */
static const uint8_t step_table[4][4] = {
    { 1, 0, 0, 0 },
    { 0, 1, 0, 0 },
    { 0, 0, 1, 0 },
    { 0, 0, 0, 1 },
};
#else
#  error "Uncomment either STEPPER_HALF_STEP or STEPPER_FULL_STEP above"
#endif

/* -------------------------------------------------------------------------
 * 28BYJ-48 stepper via ULN2003 driver board.
 *
 * Pin mapping (avoids all fingerprint-LPSDK wiring on P3.0-P3.5, P5.3-P5.5):
 *   IN1 = P5.2   IN2 = P5.1   IN3 = P5.0   IN4 = P4.0
 *
 * Forward (+steps): table index advances each step
 * Reverse  (-steps): table index retreats each step
 * Gear ratio ≈ 64:1 → STEPS_PER_REV steps per output-shaft revolution
 * ------------------------------------------------------------------------- */
static const gpio_cfg_t s_in1 = { PORT_5, PIN_2, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };
static const gpio_cfg_t s_in2 = { PORT_5, PIN_1, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };
static const gpio_cfg_t s_in3 = { PORT_5, PIN_0, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };
static const gpio_cfg_t s_in4 = { PORT_4, PIN_0, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };

static int step_idx = 0;

static void stepper_apply(void)
{
    const uint8_t *s = step_table[step_idx];
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

#define STEPS_90  (STEPS_PER_REV / 4)

/* positive steps = forward, negative = reverse.
 * Coils stay energised after returning — keeps the gearbox meshed so direction
 * reversals start with torque rather than fighting backlash (vibration). */
static void stepper_move(int steps)
{
    int dir = (steps > 0) ? 1 : -1;
    int n   = (steps > 0) ? steps : -steps;
    for (int i = 0; i < n; i++) {
        step_idx = (step_idx + dir + STEP_TABLE_LEN) % STEP_TABLE_LEN;
        stepper_apply();
        TMR_Delay(MXC_TMR0, USEC(STEP_DELAY_US));
    }
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
