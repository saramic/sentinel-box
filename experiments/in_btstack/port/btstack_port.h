#ifndef BTSTACK_PORT_H
#define BTSTACK_PORT_H

/* Platform console write — UART1 MAP_A (DAPLink serial) */
void console_write(const char *s);
void console_write_u8hex(uint8_t v);   /* write 2-char hex e.g. "2a" */
void console_write_u16dec(uint16_t v); /* write decimal string */

/* Full BTstack bring-up: board init → HCI init → btstack_main() */
void bluetooth_main(void);

/* UART HAL run-loop tick — call from while(1) in main() */
void hal_btstack_run_loop_execute_once(void);

/* Set to 1 from packet_handler after LE connect to trace BLE UART I/O. */
extern volatile uint8_t ble_io_log;

#endif
