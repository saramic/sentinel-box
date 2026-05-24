/*
 * sentinel-box — BLE GATT LED control.
 *
 * Write 1 byte to LED characteristic (0xF002):
 *   0=off  1=red  2=green  3=blue
 *
 * LED state:
 *   blue   — initialising
 *   green  — advertising as "SentinelBox"
 *   blue   — connected
 *   green  — disconnected (resumes advertising)
 */

/* ---- Board_Init — PMIC (MAX14690) LDO2+LDO3 → 3.3V (VDDIOH for Port 2) -
 * Called automatically by the SDK startup before main(). Sets Port-2 VDDIOH
 * so UART1 TX and LEDs drive at 3.3V.
 */
#include <stddef.h>
#include "mxc_config.h"
#include "i2cm.h"
#include "ioman.h"
#include "clkman.h"

int Board_Init(void)
{
    const sys_cfg_i2cm_t cfg = {
        .clk_scale = CLKMAN_SCALE_DIV_1,
        .io_cfg    = IOMAN_I2CM2(IOMAN_MAP_A, 1),
    };
    if (I2CM_Init(MXC_I2CM2, &cfg, I2CM_SPEED_100KHZ) != E_NO_ERROR)
        return E_NO_ERROR;
    uint8_t cmds[][2] = { {0x15,25},{0x14,2},{0x17,25},{0x16,2} };
    for (int i = 0; i < 4; i++)
        I2CM_Write(MXC_I2CM2, 0x28, NULL, 0, cmds[i], 2);
    return E_NO_ERROR;
}

/* ---- BTstack + application includes ------------------------------------- */
#include "btstack.h"
#include "gpio.h"
#include "uart.h"
#include "led_service.h"
#include "btstack_port.h"

/* ---- RGB LED — P2.4=R, P2.5=G, P2.6=B, active-low open-drain ----------- */
static const gpio_cfg_t led_r = {PORT_2, PIN_4, GPIO_FUNC_GPIO, GPIO_PAD_OPEN_DRAIN};
static const gpio_cfg_t led_g = {PORT_2, PIN_5, GPIO_FUNC_GPIO, GPIO_PAD_OPEN_DRAIN};
static const gpio_cfg_t led_b = {PORT_2, PIN_6, GPIO_FUNC_GPIO, GPIO_PAD_OPEN_DRAIN};

static void led_init(void) {
    GPIO_Config(&led_r); GPIO_OutSet(&led_r);
    GPIO_Config(&led_g); GPIO_OutSet(&led_g);
    GPIO_Config(&led_b); GPIO_OutSet(&led_b);
}

static void led_set(int r, int g, int b) {
    r ? GPIO_OutClr(&led_r) : GPIO_OutSet(&led_r);
    g ? GPIO_OutClr(&led_g) : GPIO_OutSet(&led_g);
    b ? GPIO_OutClr(&led_b) : GPIO_OutSet(&led_b);
}

/* ---- LED state ----------------------------------------------------------- */
static uint8_t led_value = 0;   /* 0=off 1=red 2=green 3=blue */

static void led_apply(uint8_t v)
{
    led_value = v;
    switch (v) {
    case 0: console_write("[led] off\r\n");   led_set(0,0,0); break;
    case 1: console_write("[led] red\r\n");   led_set(1,0,0); break;
    case 2: console_write("[led] green\r\n"); led_set(0,1,0); break;
    case 3: console_write("[led] blue\r\n");  led_set(0,0,1); break;
    }
}

/* ---- 28BYJ-48 stepper via ULN2003 — P5.2=IN1 P5.1=IN2 P5.0=IN3 P4.0=IN4 --- */
static const gpio_cfg_t s_in1 = {PORT_5, PIN_2, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL};
static const gpio_cfg_t s_in2 = {PORT_5, PIN_1, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL};
static const gpio_cfg_t s_in3 = {PORT_5, PIN_0, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL};
static const gpio_cfg_t s_in4 = {PORT_4, PIN_0, GPIO_FUNC_GPIO, GPIO_PAD_NORMAL};

static const uint8_t s_half[8][4] = {
    {1,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,1,1,0},
    {0,0,1,0}, {0,0,1,1}, {0,0,0,1}, {1,0,0,1},
};
static int      s_step_idx  = 0;
static int32_t  s_current   = 0;   /* absolute position in half-steps */
static int32_t  s_target    = 0;
static uint32_t s_last_ms   = 0;
#define STEPPER_STEP_MS  3         /* 2944µs ≈ 3ms per half-step at 5 RPM */

static void stepper_init(void)
{
    GPIO_Config(&s_in1); GPIO_OutClr(&s_in1);
    GPIO_Config(&s_in2); GPIO_OutClr(&s_in2);
    GPIO_Config(&s_in3); GPIO_OutClr(&s_in3);
    GPIO_Config(&s_in4); GPIO_OutClr(&s_in4);
}

static void stepper_apply(void)
{
    const uint8_t *s = s_half[s_step_idx];
    s[0] ? GPIO_OutSet(&s_in1) : GPIO_OutClr(&s_in1);
    s[1] ? GPIO_OutSet(&s_in2) : GPIO_OutClr(&s_in2);
    s[2] ? GPIO_OutSet(&s_in3) : GPIO_OutClr(&s_in3);
    s[3] ? GPIO_OutSet(&s_in4) : GPIO_OutClr(&s_in4);
}

/* ---- Connection state for notifications ---------------------------------- */
static hci_con_handle_t s_con_handle      = HCI_CON_HANDLE_INVALID;
static uint8_t          s_steps_notify    = 0;
static uint8_t          s_notify_pending  = 0;

/* ---- Non-blocking stepper tick — call from main while(1) ----------------- */
static void stepper_tick(void)
{
    if (s_current == s_target) return;
    uint32_t now = hal_time_ms();
    if (now - s_last_ms < STEPPER_STEP_MS) return;
    s_last_ms = now;

    int dir = (s_target > s_current) ? 1 : -1;
    s_step_idx = (s_step_idx + dir + 8) % 8;
    stepper_apply();
    s_current += dir;

    if (s_current == s_target || (s_current % 10) == 0) {
        console_write("[step] pos=");
        console_write_u16dec((uint16_t)(s_current < 0 ? 0 : s_current));
        console_write("\r\n");
    }

    if (s_steps_notify && s_con_handle != HCI_CON_HANDLE_INVALID && !s_notify_pending) {
        s_notify_pending = 1;
        att_server_request_can_send_now_event(s_con_handle);
    }
}

/* ---- Advertising payload ------------------------------------------------- */
static const uint8_t adv_data[] = {
    0x02, 0x01, 0x06,
    0x0C, 0x09, 'S','e','n','t','i','n','e','l','B','o','x',
};

/* ---- Packet handler ------------------------------------------------------- */
static btstack_packet_callback_registration_t hci_event_cb;
static const bd_addr_t zero_addr = {0,0,0,0,0,0};

static void packet_handler(uint8_t type, uint16_t ch, uint8_t *pkt, uint16_t sz)
{
    UNUSED(ch); UNUSED(sz);
    if (type != HCI_EVENT_PACKET) return;
    switch (hci_event_packet_get_type(pkt)) {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(pkt) == HCI_STATE_WORKING) {
            console_write("[ble] advertising\r\n");
            gap_advertisements_set_params(160, 160, 0, 0, (uint8_t *)zero_addr, 0x07, 0);
            gap_advertisements_set_data(sizeof(adv_data), (uint8_t *)adv_data);
            gap_advertisements_enable(1);
            led_apply(2);   /* green = advertising */
        }
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        console_write("[ble] disconnected\r\n");
        s_con_handle   = HCI_CON_HANDLE_INVALID;
        s_steps_notify = 0;
        s_notify_pending = 0;
        ble_io_log = 0;
        gap_advertisements_enable(1);
        led_apply(2);   /* green = advertising again */
        break;
    case HCI_EVENT_LE_META:
        if (hci_event_le_meta_get_subevent_code(pkt) == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
            console_write("[ble] connected\r\n");
            s_con_handle = hci_subevent_le_connection_complete_get_connection_handle(pkt);
            ble_io_log = 1;
            led_apply(3);   /* blue = connected */
        }
        break;
    case ATT_EVENT_CAN_SEND_NOW:
        if (s_notify_pending) {
            s_notify_pending = 0;
            uint8_t val[2] = { (uint8_t)s_current, (uint8_t)(s_current >> 8) };
            att_server_notify(s_con_handle,
                ATT_CHARACTERISTIC_0000F011_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE,
                val, 2);
        }
        break;
    default:
        break;
    }
}

/* ---- ATT read handler ---------------------------------------------------- */
static uint16_t att_read_handler(hci_con_handle_t h, uint16_t att_h, uint16_t offset,
                                 uint8_t *buf, uint16_t buf_size)
{
    UNUSED(h); UNUSED(offset); UNUSED(buf_size);
    if (att_h == ATT_CHARACTERISTIC_0000F002_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        if (buf) buf[0] = led_value;
        return 1;
    }
    if (att_h == ATT_CHARACTERISTIC_0000F011_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        if (buf) { buf[0] = (uint8_t)s_current; buf[1] = (uint8_t)(s_current >> 8); }
        return 2;
    }
    return 0;
}

/* ---- ATT write handler --------------------------------------------------- */
static int att_write_handler(hci_con_handle_t h, uint16_t att_h, uint16_t mode,
                             uint16_t offset, uint8_t *buf, uint16_t len)
{
    UNUSED(h); UNUSED(mode); UNUSED(offset);
    if (att_h == ATT_CHARACTERISTIC_0000F002_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE
            && len >= 1) {
        led_apply(buf[0]);
    } else if (att_h == ATT_CHARACTERISTIC_0000F011_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE
            && len >= 2) {
        s_target = (int32_t)(buf[0] | ((uint16_t)buf[1] << 8));
        console_write("[step] target=");
        console_write_u16dec((uint16_t)s_target);
        console_write("\r\n");
    } else if (att_h == ATT_CHARACTERISTIC_0000F011_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE
            && len >= 2) {
        s_steps_notify = buf[0] & 0x01;
        console_write(s_steps_notify ? "[step] notify on\r\n" : "[step] notify off\r\n");
    }
    return 0;
}

/* ---- main --------------------------------------------------------------- */
int main(void)
{
    bluetooth_main();
    console_write("[main] run loop\r\n");
    while (1) {
        hal_btstack_run_loop_execute_once();
        stepper_tick();
    }
    return 0;
}

/* ---- btstack_main — called by bluetooth_main() in port/btstack_port.c --- */
int btstack_main(int argc, const char *argv[])
{
    UNUSED(argc); UNUSED(argv);

    led_init();
    stepper_init();
    led_apply(3);   /* blue = initialising */

    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);

    att_server_init(profile_data, att_read_handler, att_write_handler);
    att_server_register_packet_handler(packet_handler);

    hci_event_cb.callback = packet_handler;
    hci_add_event_handler(&hci_event_cb);

    hci_power_control(HCI_POWER_ON);
    return 0;
}
