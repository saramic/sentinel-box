# Sentinel Box — Firmware Architecture

## Hardware

**MCU:** MAX32630FTHR (ARM Cortex-M4F, 96 MHz, 512 KB RAM, 2 MB Flash)

**On-board peripherals:**
| Peripheral | Interface | Notes |
|---|---|---|
| BMI160 IMU (accel + gyro) | I2C | 6-axis, address 0x68/0x69 |
| MAX14690 PMIC | I2C (addr 0x28) | Power management — on I2CM2, not I2CM0 |
| MAX30101 optical sensor | I2C | Pulse ox / heart rate |
| RGB LED (P2.4/5/6) | GPIO | Active low, open-drain |

---

## Firmware Approaches

Three firmware stacks are available. All use the same OpenOCD flash command.

### 1. mbed (`experiments/in_blink_mbed`)

```sh
mise run build        # pio run -e in_blink_mbed
mise run upload       # openocd flash
```

- **Status:** Working
- **EOL:** ARM mbed OS end-of-life June 2026 — no new updates after that
- **Tooling:** PlatformIO + maxim32 platform
- **Use for:** Rapid prototyping today; do not start new long-lived work here

### 2. LPSDK (`experiments/in_blink_LPSDK`)

```sh
cd experiments/in_blink_LPSDK && make
mise run upload:in_blink_LPSDK
```

- **Status:** Working
- **SDK:** Maxim LPSDK 1.2.0 installed at `~/Maxim`
  - Download: analog.com → MAX32630 → Software Downloads (SFW0001660A)
  - Toolchain: `~/Maxim/Toolchain/bin/arm-none-eabi-gcc`
- **Key gotcha:** Always override `Board_Init()` — the EvKit BSP init
  hangs on FTHR because the PMIC is on I2CM2, not I2CM0
- **Use for:** Production-quality C firmware using Maxim's official peripheral
  drivers (GPIO, I2C, SPI, UART, timers, ADC all provided)
- **Risk:** SDK is effectively retired; no new releases expected

### 3. Rust (`experiments/in_blink_rust`)

```sh
cd experiments/in_blink_rust && cargo build --release
mise run upload:in_blink_rust
```

- **Status:** Working (bare-metal, direct register access)
- **Target:** `thumbv7em-none-eabihf` (Cortex-M4F)
- **PAC:** `github.com/wez/max32630` — SVD-generated, not on crates.io
- **HAL:** Does not exist yet — see HAL Roadmap below
- **Use for:** Long-term path; no EOL risk; growing embedded-hal ecosystem

---

## Rust HAL Roadmap

Goal: implement `embedded-hal` traits for MAX32630 so driver crates
(NFC, fingerprint, IMU) work without modification.

Foundation: use `wez/max32630` PAC for typed register access.
Create crate: `max32630-hal` inside `crates/max32630-hal/`.

| Peripheral | Effort | `embedded-hal` trait | Needed for |
|---|---|---|---|
| GPIO | ~50 lines | `OutputPin`, `InputPin` | Everything |
| Timers | ~150 lines | `DelayMs`, `DelayUs` | Everything |
| I2C | ~300 lines | `I2c` | BMI160, PN532, MAX14690 |
| UART | ~200 lines | `serial::Read/Write` | Fingerprint, GPS, ESP32 |
| SPI | ~200 lines | `SpiDevice` | Camera (OV2640), some NFC |
| ADC | ~150 lines | `Channel` | Piezo vibration sensor |

Suggested build order: GPIO → Timers → I2C → UART → SPI → ADC

### LPSDK vendoring (optional)

To remove the `~/Maxim` dependency, vendor the minimum LPSDK subset:

```
vendor/lpsdk/
  Libraries/MAX3263XPeriphDriver/   ← peripheral drivers (~2 MB)
  Libraries/CMSIS/                  ← ARM CMSIS + startup + linker scripts
  Libraries/Boards/EvKit_V1/        ← board BSP (used as build base)
```

Update `MAXIM_PATH = $(CURDIR)/../../vendor/lpsdk` in experiment Makefiles.

---

## SparkFun Edge as Wake-Word Co-processor

The SparkFun Edge (Apollo3 Blue) has excellent always-on keyword spotting
at ~1 µA sleep current. Use it to wake the MAX32630 from LP mode:

```
SparkFun Edge                MAX32630FTHR
─────────────────            ────────────────
keyword detected ──GPIO──▶  GPIO interrupt pin
                             (wakes from LP1/LP2 via NVIC)
```

SparkFun Edge firmware: TensorFlow Lite Micro + PDM mic.
MAX32630 firmware: configure GPIO interrupt, sleep in LP2, wake and act.

---

## 6-Axis IMU (BMI160)

The MAX32630FTHR has an on-board BMI160 connected via I2C.

**With LPSDK:**
```c
#include "i2cm.h"
// use I2CM_Read / I2CM_Write on MXC_I2CM1 at address 0x68
```

**With Rust (once I2C HAL exists):**
```toml
bmi160 = "0.1"   # embedded-hal compatible driver crate
```
Until the HAL exists: read registers directly via raw I2C register writes
(same pattern as the GPIO blink — see in_blink_rust/src/main.rs).

---

## Future Experiments

| Experiment | Firmware | Key HAL needed |
|---|---|---|
| `in_attitude_meter` | Rust | I2C (BMI160) |
| `in_nfc_reader` | LPSDK or Rust | SPI or I2C (PN532) |
| `in_fingerprint` | LPSDK or Rust | UART (R307/AS608) |
| `in_wake_word` | SparkFun Edge + MAX32630 | GPIO interrupt |
| `in_sentinel_box` | Rust | All of the above |

---

## Flash Command (all experiments)

```sh
openocd -s /usr/local/share/openocd/scripts \
  -f interface/cmsis-dap.cfg \
  -f target/max3263x.cfg \
  -c 'program <path/to/firmware.elf> verify reset exit'
