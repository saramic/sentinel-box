# in_attitude_meter

Reads X/Y acceleration from the BMI160 IMU on the MAX32630FTHR and displays
a rolling/tilting bar on a MAX7219 8×8 LED matrix.

**Approach:** Rust bare-metal (no HAL, direct register access)
**Status:** Working — bar responds to board tilt and rolls on velocity

## Hardware wiring

### MAX7219 LED matrix — P3.0 / P3.1 / P3.2

Bit-banged SPI (write-only; no MISO required).

| MAX7219 pin | MAX32630FTHR pin | Notes |
|-------------|-----------------|-------|
| DIN (MOSI)  | P3.3            | |
| CLK (SCLK)  | P3.4            | |
| CS (/SS)    | P3.5            | active low |
| VCC         | 3V3 header      | LDO3 rail |
| GND         | GND header      | |

### BMI160 IMU — I2CM2 (on-board, no wiring needed)

The BMI160 is soldered on the MAX32630FTHR and connected to I2CM2.
Both BMI160 and the MAX14690 PMIC share this bus.

| Signal | MAX32630FTHR pin | Notes |
|--------|-----------------|-------|
| SDA    | P5.7            | I2CM2 Map A, 4.7 kΩ pull-up on board |
| SCL    | P6.0            | I2CM2 Map A, 4.7 kΩ pull-up on board |

BMI160 I2C address: `0x68` (SDO tied low on board).

### MAX14690 PMIC — same I2CM2 bus

| Signal | MAX32630FTHR pin | Notes |
|--------|-----------------|-------|
| SDA    | P5.7            | shared with BMI160 |
| SCL    | P6.0            | shared with BMI160 |

PMIC I2C address: `0x28`. `pmic::init()` enables LDO3 (3.3 V rail) and
must be called before any peripheral that needs 3V3.

## How it works

- `sys::init()` selects the 96 MHz ring oscillator and loads factory trim values
- `pmic::init()` enables LDO3 via I2CM2 (brings up 3V3 rail)
- `bmi160::acc_init(AccOdr::Hz200)` sets accelerometer to 200 Hz normal mode
- Main loop reads X and Y acceleration, maps them to a diagonal scrolling bar:
  - Y tilt → bar velocity (tilt one way, bar scrolls; counter-tilt to slow it)
  - X tilt → diagonal slope across the 8 columns

## Diagnostic display (first 3 seconds after reset)

Rows 1–4 show I2C diagnostic values before the loop starts:

| Row | Value | Expected |
|-----|-------|----------|
| 1   | BMI160 chip_id (before ACC_NORMAL) | `0xD1` = `●●·●···●` |
| 2   | I2CM2 INTFL (after chip_id read)   | `0x01` = TX_DONE only |
| 3   | BMI160 chip_id (after ACC_NORMAL)  | `0xD1` still |
| 4   | I2CM2 INTFL (after CMD write)      | `0x01` = TX_DONE only |

`0x03` in rows 2 or 4 means TX_DONE + TX_NACK_ERR — device not responding.

## Build and upload

```sh
mise run build:in_attitude_meter
mise run upload:in_attitude_meter
# or combined:
mise run cbu:in_attitude_meter
```
