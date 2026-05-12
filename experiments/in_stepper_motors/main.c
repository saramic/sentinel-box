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
 * 28BYJ-48 stepper via ULN2003 driver board.
 *
 * Pin mapping (avoids all fingerprint-LPSDK wiring on P3.0-P3.5, P5.3-P5.5):
 *   IN1 = P5.2   IN2 = P5.1   IN3 = P5.0   IN4 = P4.0
 *
 * Physical coil layout (viewed from shaft end, CW order):
 *
 *              IN1  (0°)
 *               |
 *   IN4 --------+-------- IN2  (IN2 at 90° CW from IN1)
 *  (270°)       |        (90°)
 *              IN3
 *             (180°)
 *
 * Adjacent pairs going CW: IN1→IN2→IN3→IN4→IN1
 * The Arduino Stepper library confirms this — it passes pins as
 * (IN1, IN3, IN2, IN4) into a generic bipolar step table, which resolves
 * to the same adjacent pairs: IN1+IN2, IN2+IN3, IN3+IN4, IN4+IN1.
 *
 * Wave drive (single-coil), 2038 steps/rev:
 *   step  {IN1,IN2,IN3,IN4}
 *     0    1  0  0  0      IN1
 *     1    0  1  0  0      IN2
 *     2    0  0  1  0      IN3
 *     3    0  0  0  1      IN4
 *
 * Half-step, 4076 steps/rev:
 *   step  {IN1,IN2,IN3,IN4}
 *     0    1  0  0  0      IN1
 *     1    1  1  0  0      IN1+IN2
 *     2    0  1  0  0      IN2
 *     3    0  1  1  0      IN2+IN3
 *     4    0  0  1  0      IN3
 *     5    0  0  1  1      IN3+IN4
 *     6    0  0  0  1      IN4
 *     7    1  0  0  1      IN4+IN1
 *
 * Gear ratio: 32 internal steps × 63.68 ≈ 2037.9 → use 2038 full / 4076 half.
 * Reverse (-steps): table index retreats instead of advances.
 * Coils stay energised after a move — holds gearbox meshed for clean reversal.
 * ------------------------------------------------------------------------- */
static const gpio_cfg_t s_in1 = { PORT_5, PIN_2, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };
static const gpio_cfg_t s_in2 = { PORT_5, PIN_1, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };
static const gpio_cfg_t s_in3 = { PORT_5, PIN_0, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };
static const gpio_cfg_t s_in4 = { PORT_4, PIN_0, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL };

static const uint8_t wave_step[4][4] = {
    { 1, 0, 0, 0 },   /* IN1        */
    { 0, 1, 0, 0 },   /* IN2        */
    { 0, 0, 1, 0 },   /* IN3        */
    { 0, 0, 0, 1 },   /* IN4        */
};

static const uint8_t half_step[8][4] = {
    { 1, 0, 0, 0 },   /* IN1        */
    { 1, 1, 0, 0 },   /* IN1+IN2    */
    { 0, 1, 0, 0 },   /* IN2        */
    { 0, 1, 1, 0 },   /* IN2+IN3    */
    { 0, 0, 1, 0 },   /* IN3        */
    { 0, 0, 1, 1 },   /* IN3+IN4    */
    { 0, 0, 0, 1 },   /* IN4        */
    { 1, 0, 0, 1 },   /* IN4+IN1    */
};

/* active mode — switch with stepper_use_wave() / stepper_use_half_step() */
static const uint8_t (*active_table)[4] = half_step;
static int      table_len     = 8;
static uint32_t step_delay_us  = 2944;   /* 5 RPM – 60,000,000 / 4076 / 5 */
static int      steps_per_rev  = 4076;   /* 32 × 63.68 × 2 */

static int step_idx = 0;

/* -------------------------------------------------------------------------
 * Step delay formula (from Arduino Stepper library):
 *   step_delay_us = 60,000,000 / steps_per_rev / RPM
 *
 * Half-step (4076 steps/rev):         Wave drive (2038 steps/rev):
 *   5 RPM  → 2,944 µs                  10 RPM → 2,944 µs
 *  10 RPM  → 1,472 µs                  15 RPM → 1,963 µs  ← max reliable
 *                                       20 RPM → 1,472 µs  ← stalls
 * ------------------------------------------------------------------------- */
static void stepper_use_half_step(void)
{
    active_table   = half_step;
    table_len      = 8;
    step_delay_us  = 2944;   /* 5 RPM – 60,000,000 / 4076 / 5 */
    steps_per_rev  = 4076;   /* 32 × 63.68 × 2 */
    step_idx       = 0;
}

static void stepper_use_wave(void)
{
    active_table   = wave_step;
    table_len      = 4;
    step_delay_us  = 1963;   /* 15 RPM – 60,000,000 / 2038 / 15 */
    steps_per_rev  = 2038;   /* 32 × 63.68 */
    step_idx       = 0;
}

static void stepper_apply(void)
{
    const uint8_t *s = active_table[step_idx];
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

/* positive steps = forward (CW), negative = reverse (CCW) */
static void stepper_move(int steps)
{
    int dir = (steps > 0) ? 1 : -1;
    int n   = (steps > 0) ? steps : -steps;
    for (int i = 0; i < n; i++) {
        step_idx = (step_idx + dir + table_len) % table_len;
        stepper_apply();
        TMR_Delay(MXC_TMR0, USEC(step_delay_us));
    }
}

/* -------------------------------------------------------------------------
 * Application:
 *   1. Half-step  360° forward (green) then 360° reverse (blue) — ~10 RPM
 *   2. Wave-drive 360° forward (green) then 360° reverse (blue) — ~20 RPM
 *   Red LED + halt when done. Reset board to repeat.
 * ------------------------------------------------------------------------- */
int main(void)
{
    led_init();
    stepper_init();

    stepper_use_half_step();
    led_set(0, 1, 0);
    stepper_move(+steps_per_rev);
    led_set(0, 0, 1);
    stepper_move(-steps_per_rev);
    TMR_Delay(MXC_TMR0, MSEC(500));

    stepper_use_wave();
    led_set(0, 1, 0);
    stepper_move(+steps_per_rev);
    led_set(0, 0, 1);
    stepper_move(-steps_per_rev);

    stepper_off();
    led_set(1, 0, 0);
    while (1) {}
}
