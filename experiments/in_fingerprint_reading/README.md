# in_fingerprint_reading

Fingerprint sensor experiment on the MAX32630FTHR using an Adafruit
AS608/R305/R307 - compatible sensor (TTL UART, 57600 bps).  Successful reads are
confirmed on the MAX7219 8×8 LED matrix.

## Reference on Fingerprint sensor

- [https://www.adafruit.com/product/4690](https://www.adafruit.com/product/4690)
- [https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library](
  https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library)
- [https://github.com/adafruit/Adafruit_CircuitPython_Fingerprint](
  https://github.com/adafruit/Adafruit_CircuitPython_Fingerprint)
- [https://learn.adafruit.com/adafruit-optical-fingerprint-sensor](
  https://learn.adafruit.com/adafruit-optical-fingerprint-sensor)
- [https://cdn-shop.adafruit.com/product-files/4690/4690_diagram_C11911.pdf](
  https://cdn-shop.adafruit.com/product-files/4690/4690_diagram_C11911.pdf)

## Hardware wiring

| Signal | Sensor wire | MAX32630FTHR pin | Notes |
|--------|-------------|------------------|-------|
| 3.3 V  | Red (VCC)   | 3V3 header       | LDO3 rail, enabled by pmic::init() |
| GND    | Black (GND) | GND header       | |
| TX→RX  | White (TX)  | P2.0             | UART1 Map A RX |
| RX←TX  | Green (RX)  | P2.1             | UART1 Map A TX |

The sensor Wakeup/Touch-out line is not used in this experiment.

## Implementation order

### Stage 1 — UART bring-up (uart.rs)

- [ ] Confirm STATUS register bit for RX-not-empty and TX-not-full (check `uart_regs.h` from
      mbed TARGET_MAX32630; likely STATUS bits 6=RX_EMPTY, 7=TX_FULL or similar)
- [ ] Replace the placeholder busy-loop in `read_byte()` with a real status-bit poll
- [ ] Verify 57600 bps divisor=13 with a logic analyser or oscilloscope on P2.1

### Stage 2 — Sensor handshake (fingerprint.rs + main.rs)

- [ ] `verify_password()` succeeds → row 1 lit on display (already wired in main.rs)
- [ ] If row 8 blinks: check wiring, baud rate, and try dropping to 9600 bps
      (divisor = 78 for 9600 bps with 128× oversampling at 96 MHz)

### Stage 3 — Finger capture loop (main.rs)

- [ ] Poll `get_image()` until OK (finger detected)
- [ ] Call `image_to_tz(1)` to convert the image to a feature template in slot 1
- [ ] Display intermediate status on rows (e.g., row 2 = waiting, row 3 = converting)

### Stage 4 — Search and match

- [ ] Call `finger_search()` — parse the 2-byte finger_id and 2-byte match_score from the
      ACK payload (currently `finger_search` returns 0 for found_id — needs implementing)
- [ ] On match: display match ID across display rows (e.g., binary or 1-8 mapping)
- [ ] On no-match (ERR_NOT_FOUND): show row 8, offer enrolment path

### Stage 5 — Enrolment

- [ ] Capture finger twice into slots 1 and 2 (`image_to_tz(1)` then `image_to_tz(2)`)
- [ ] Call `reg_model` (0x05) to merge slots 1+2 into a combined template
- [ ] Call `store_model(id)` (0x06) to write to flash at the chosen ID
- [ ] Confirm with all-rows-on flash

### Stage 6 — Database management

- [ ] `empty_database()` (0x0D) — wipe all stored fingerprints
- [ ] Consider a long-press gesture (finger held > 3 s) to trigger wipe

## Known TODOs in the code

| File | Location | Issue |
|------|----------|-------|
| uart.rs | `read_byte()` | Status bit for RX-not-empty not yet confirmed |
| uart.rs | `write_byte()` | Status bit for TX-not-full not yet confirmed |
| fingerprint.rs | `finger_search()` | found_id always returns 0; parse ACK payload |
| uart.rs | baud rate | Divisor=13 (57600 bps) needs oscilloscope verification |

## Modules shared with in_attitude_meter

| Module | Purpose |
|--------|---------|
| sys.rs | 96 MHz clock init, DWT cycle counter, `delay_cycles` |
| pmic.rs | MAX14690 PMIC init — enables LDO3 (3.3 V rail) via I2CM2 |
| gpio.rs | Push-pull / open-drain pin abstraction |
| max7219.rs | Bit-banged SPI driver for the 8×8 LED matrix |

## Build and upload

```sh
mise run build:in_fingerprint_reading
mise run upload:in_fingerprint_reading
# or combined:
mise run cbu:in_fingerprint_reading
```
