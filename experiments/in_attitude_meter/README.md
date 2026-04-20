# in_attitude_meter

Read roll, pitch, and yaw from the MAX32630FTHR's on-board BMI160 IMU
and display on a MAX7219-powered LED matrix module.

**Approach:** Rust
**Status:** Planned — display unblocked (bit-bang GPIO); IMU blocked on I2C HAL

---

## Hardware

### MAX7219 LED matrix (display)

Pins: VCC, GND, DIN, CS, CLK — this is **SPI** (write-only, no MISO).

| MAX7219 | SPI role | Direction |
|---|---|---|
| DIN | MOSI | MCU → device |
| CS | /SS (active low) | MCU → device |
| CLK | SCLK | MCU → device |

Because the MAX7219 never sends data back, it can be **bit-banged with 3
GPIO pins** today — no SPI HAL required:

```rust
fn send_byte(din: &mut Pin, clk: &mut Pin, byte: u8) {
    for i in (0..8).rev() {
        set_pin(din, (byte >> i) & 1 != 0);
        pulse_high(clk);
    }
}

fn write_reg(din: &mut Pin, cs: &mut Pin, clk: &mut Pin, reg: u8, val: u8) {
    set_low(cs);
    send_byte(din, clk, reg);
    send_byte(din, clk, val);
    set_high(cs);
}
```

When the SPI HAL exists, swap bit-bang for hardware SPI and the
`max7219` driver crate.

### BMI160 IMU (attitude source)

The BMI160 is connected to the MAX32630 via I2C:
- Bus: I2CM1 (MXC_I2CM1), base address `0x4001_8000`
- Address: `0x68` (SDO low) or `0x69` (SDO high)
- Registers of interest:
  - `0x0F` CHIP_ID — should read `0xD1` to confirm comms
  - `0x12` ACC_X_LSB / `0x13` ACC_X_MSB (and Y, Z)
  - `0x0C` GYR_X_LSB / `0x0D` GYR_X_MSB (and Y, Z)
  - `0x7E` CMD — write `0x11` to set accel normal mode

## What needs building first

### Option A: LPSDK (faster start)

```c
#include "i2cm.h"

// init
const sys_cfg_i2cm_t i2cm_cfg = { ... };
I2CM_Init(MXC_I2CM1, &i2cm_cfg, I2CM_SPEED_400KHZ);

// read chip ID
uint8_t chip_id;
I2CM_Read(MXC_I2CM1, 0x68, 0x0F, NULL, 0, &chip_id, 1, NULL);
// expect chip_id == 0xD1
```

### Option B: Rust (requires HAL work)

**Step 1** — create `crates/max32630-hal/` with I2C peripheral impl:

```
crates/
  max32630-hal/
    Cargo.toml       # depends on wez/max32630 PAC + embedded-hal
    src/
      lib.rs
      i2c.rs         # implement embedded_hal::i2c::I2c for MAX32630
      timer.rs       # implement embedded_hal::delay::DelayMs
```

I2CM1 register map (from LPSDK `i2cm_regs.h`):
- Base: `0x4001_8000`
- Key offsets: `CN` (control), `INT_FL` (flags), `FIFO` (data), `HS` (hs mode)

**Step 2** — use `bmi160` crate once I2C trait is implemented:

```toml
[dependencies]
bmi160 = "0.1"
max32630-hal = { path = "../../crates/max32630-hal" }
```

```rust
let i2c = I2c::new(p.I2CM1, 400.kHz());
let mut imu = Bmi160::new_with_i2c(i2c, SlaveAddr::Default);
imu.set_accel_power_mode(AccelPowerMode::Normal)?;
let data = imu.data()?;
```

## Tests

Integration tests should run against the real hardware via a serial
loopback fixture. Unit tests for the HAL can run on host using
`embedded-hal-mock`.

```sh
# host unit tests (no hardware needed)
cd crates/max32630-hal && cargo test

# on-hardware test (requires board connected)
# flash in_attitude_meter and read serial output
```
