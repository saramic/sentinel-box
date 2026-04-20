# Experiments

Each experiment is a self-contained firmware project for the MAX32630FTHR.
All use the same OpenOCD flash command via `mise run upload:<name>`.

## Build & Flash

| Experiment | Build | Upload |
|---|---|---|
| `in_blink_mbed` | `mise run build` | `mise run upload` |
| `in_blink_LPSDK` | `cd in_blink_LPSDK && make` | `mise run upload:in_blink_LPSDK` |
| `in_blink_rust` | `cd in_blink_rust && cargo build --release` | `mise run upload:in_blink_rust` |

## Experiments

### `in_blink_mbed` — mbed blink
Baseline blink using PlatformIO + mbed framework. Working reference.
mbed EOL June 2026 — do not start new work here.

### `in_blink_LPSDK` — LPSDK blink
Blink using Maxim LPSDK C drivers. Requires `~/Maxim` installed.
Cycles red → green → blue LED at 500ms each.
Key: override `Board_Init()` to avoid PMIC hang on FTHR board.

### `in_blink_rust` — Rust bare-metal blink
Blink using direct register access. No SDK required.
Foundation for the Rust HAL — see ARCHITECTURE.md.

### `in_attitude_meter` — IMU attitude display _(planned)_
Read BMI160 accel + gyro over I2C. Display roll/pitch on serial.
Requires Rust I2C HAL or LPSDK `i2cm.h` drivers.

### `in_nfc_reader` — NFC card read _(planned)_
PN532 over SPI or I2C. Read MIFARE / NFC tags.

### `in_fingerprint` — Fingerprint sensor _(planned)_
R307 or AS608 over UART. Enroll and match fingerprints.

## Prerequisites

```sh
# LPSDK (required for in_blink_LPSDK)
# Download SFW0001660A from analog.com/en/products/max32630.html
# Install to ~/Maxim

# Rust (required for in_blink_rust and future Rust experiments)
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
rustup target add thumbv7em-none-eabihf

# Flash tool
brew install open-ocd  # or build from analogdevicesinc/openocd
```
