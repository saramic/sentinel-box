/*
 * Phase 2 — BTstack BLE GATT server — MAX32630FTHR.
 *
 * Write 1 byte to LED characteristic (0xF002):
 *   0=off  1=red  2=green  3=blue
 *
 * Platform bring-up (UART, 32kHz, PMIC, nSHUTD, HCI) is in port/btstack_port.c.
 * Board_Init() sets VDDIOH (Port-2 supply) to 3.3V via the MAX14690 PMIC so that
 * UART1 TX (P2.1) and the LEDs (P2.4-6) drive at the correct level before main().
 *
 * -- Fall back to Phase 1 (serial + LED blink, no BLE) -------------------
 * See the PHASE 1 block at the bottom of this file.
 * Build with the Phase 1 Makefile (BTstack lines commented out).
 */

/* ---- Board_Init — PMIC (MAX14690) LDO2+LDO3 → 3.3V (VDDIOH for Port 2) - *
 * Called automatically by the SDK startup (SystemInit → Board_Init) before
 * main() runs. Sets Port-2 VDDIOH so UART1 TX and LEDs drive at 3.3V.
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
#include "uart.h"    /* MXC_UART1 for the inline char write in packet_handler */
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
    case BTSTACK_EVENT_STATE: {
        uint8_t s = btstack_event_state_get_state(pkt);
        console_write("[pkt] BTSTACK_EVENT_STATE s=");
        { char c = '0' + s; UART_Write(MXC_UART1, (uint8_t *)&c, 1); }
        console_write("\r\n");
        if (s == HCI_STATE_WORKING) {
            console_write("[pkt] HCI_STATE_WORKING\r\n");
            gap_advertisements_set_params(160, 160, 0, 0, (uint8_t *)zero_addr, 0x07, 0);
            gap_advertisements_set_data(sizeof(adv_data), (uint8_t *)adv_data);
            gap_advertisements_enable(1);
            console_write("[pkt] led_set green\r\n");
            led_set(0, 1, 0);   /* green = advertising */
        }
        break;
    }
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        console_write("[pkt] DISCONNECTION_COMPLETE -> green\r\n");
        ble_io_log = 0;
        gap_advertisements_enable(1);
        led_set(0, 1, 0);
        break;
    case HCI_EVENT_LE_META:
        if (hci_event_le_meta_get_subevent_code(pkt) == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
            console_write("[pkt] LE_CONNECTION_COMPLETE -> blue\r\n");
            console_write("[pkt] can_send_le=");
            { char c = hci_can_send_acl_le_packet_now() ? '1' : '0'; UART_Write(MXC_UART1, (uint8_t *)&c, 1); }
            console_write("\r\n");
            ble_io_log = 1;
            led_set(0, 0, 1);   /* blue = connected */
        }
        break;
    default:
        console_write("[pkt] evt=0x");
        console_write_u8hex(hci_event_packet_get_type(pkt));
        console_write("\r\n");
        break;
    }
}

/* ---- ATT write handler --------------------------------------------------- */
static int att_write_handler(hci_con_handle_t h, uint16_t att_h, uint16_t mode,
                             uint16_t offset, uint8_t *buf, uint16_t len)
{
    UNUSED(h); UNUSED(mode); UNUSED(offset);
    console_write("[att] write att_h=0x");
    console_write_u8hex((uint8_t)(att_h >> 8));
    console_write_u8hex((uint8_t)att_h);
    console_write(" val=0x");
    if (len >= 1) console_write_u8hex(buf[0]);
    console_write("\r\n");
    if (att_h == ATT_CHARACTERISTIC_0000F002_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE
            && len >= 1) {
        switch (buf[0]) {
        case 0: led_set(0,0,0); break;
        case 1: led_set(1,0,0); break;
        case 2: led_set(0,1,0); break;
        case 3: led_set(0,0,1); break;
        }
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
    }
    return 0;
}

/* ---- btstack_main — called by bluetooth_main() in port/btstack_port.c --- */
int btstack_main(int argc, const char *argv[])
{
    UNUSED(argc); UNUSED(argv);

    led_init();
    led_set(0, 0, 1);   /* blue = initialising */

    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);

    att_server_init(profile_data, NULL, att_write_handler);
    att_server_register_packet_handler(packet_handler);

    hci_event_cb.callback = packet_handler;
    hci_add_event_handler(&hci_event_cb);

    hci_power_control(HCI_POWER_ON);
    return 0;
}

/*
 * ==========================================================================
 * PHASE 1 fallback — serial + LED blink only, no BTstack
 * ==========================================================================
 * If BLE stops working, strip back to this to verify hardware.
 * In Makefile: comment out the BTstack SRCS/VPATH/IPATH lines.
 *
 * Replace the BTstack includes + all functions above with:
 *
 * #include <string.h>
 * #include "mxc_config.h"
 * #include "gpio.h"
 * #include "uart.h"
 * #include "tmr_utils.h"
 *
 * static void console_write(const char *s) { UART_Write(MXC_UART1,(uint8_t *)s,strlen(s)); }
 * static void console_init(void) {
 *     const uart_cfg_t cfg = {UART_PARITY_DISABLE,UART_DATA_SIZE_8_BITS,0,0,0,115200};
 *     const sys_cfg_uart_t sys = {CLKMAN_SCALE_DIV_1,
 *         IOMAN_UART(1,IOMAN_MAP_A,IOMAN_MAP_A,IOMAN_MAP_A,1,0,0)};
 *     UART_Init(MXC_UART1, &cfg, &sys);
 * }
 * [keep led_r/led_g/led_b + led_init() + led_set() as-is]
 *
 * Replace main() with:
 *
 * int main(void) {
 *     console_init();
 *     console_write("\r\n[boot] serial OK\r\n");
 *     led_init();
 *     console_write("[boot] LED init OK\r\n");
 *     int step = 0;
 *     while (1) {
 *         switch (step & 3) {
 *         case 0: led_set(1,0,0); console_write("[led] red\r\n");   break;
 *         case 1: led_set(0,1,0); console_write("[led] green\r\n"); break;
 *         case 2: led_set(0,0,1); console_write("[led] blue\r\n");  break;
 *         case 3: led_set(0,0,0); console_write("[led] off\r\n");   break;
 *         }
 *         step++;
 *         TMR_Delay(MXC_TMR0, MSEC(500));
 *     }
 * }
 */
