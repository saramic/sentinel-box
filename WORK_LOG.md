# Work Log

## TODO Links

- [ ] https://forum.arduino.cc/t/using-arduino-to-generate-quadrature-signals-for-sdr/212059/6
- [ ] https://github.com/merbanan/rtl_433/blob/master/docs/BUILDING.md
- [ ] https://www.crowdsupply.com/lime-micro/limesdr
- [ ] https://www.crowdsupply.com/lime-micro/limesdr-mini
- [ ] https://github.com/pothosware/SoapySDR/
- [ ] https://github.com/osmocom/rtl-sdr/
- [ ] https://forum.arduino.cc/t/using-arduino-to-generate-quadrature-signals-for-sdr/212059/5
- [ ] https://hackaday.com/2023/01/13/arduino-library-brings-rtl_433-to-the-esp32/#:~:text=If%20you%20have%20an%20RTL,as%20much%20of%20a%20surprise.
- [ ] The new Elektor SDR Shield for Arduino – Travelling the waves Elektor TV
    - https://www.youtube.com/watch?v=KHdqskbfFhA&t=14s
- [ ] New LoRa 32 V4 ESP32 SX1262 Low Power Dev-Board 0.96inch
  OLED Supports Wi-Fi BLE LoRa communication Compatible Meshtastic
    - https://www.aliexpress.com/item/1005010203038481.html
- [ ] Scanning ESP32 Radar Tracks Multiple Targets in Real Time! - Circuit Helper
    - https://www.youtube.com/watch?v=ZnkWKowQYXg
- [ ] I made Esp32 based Radar : It has a Built-In Display - Dsn Industries
    - https://www.youtube.com/watch?v=t4QVxeeEtEQ

- [ ] official docs
  - https://www.analog.com/en/products/max32630.html#documentation
  - downloaded the data sheet
  - also
    - https://www.analog.com/media/en/reference-design-documentation/design-notes/ds147-take-a-weight-off-your-chest-with-a-wrist-worn-ecg-monitor.pdf
    - https://www.analog.com/en/resources/design-notes/how-to-create-a-remote-medical-sensing-system-using-maxim-integrateds-iot-development-platform.html
    - https://www.analog.com/en/resources/technical-articles/take-a-weight-off-your-chest-with-a-wristworn-ecg-monitor.html
    - https://www.analog.com/media/en/technical-documentation/user-guides/interface-guide-for-max32664-sensor-hubbased-reference-design-platforms.pdf
    - https://www.analog.com/media/en/technical-documentation/user-guides/max32630-users-guide.pdf

- another competition from 2017
- [ ] https://www.allaboutcircuits.com/giveaways/get-creative-makewithmaxim-design-contest/
     - https://forum.allaboutcircuits.com/ubs/makewithmaxim-vr-glove.970/
     - https://forum.allaboutcircuits.com/ubs/pelvic-sensor.968/
     - https://forum.allaboutcircuits.com/ubs/makewithmaxim-maxbot-a-low-cost-robotic-kit.975/
     - https://forum.allaboutcircuits.com/ubs/max32630fthr-as-a-vr-controller-makewithmaxim.966/
     - https://forum.allaboutcircuits.com/ubs/fitness-wearable.980/
     - https://forum.allaboutcircuits.com/ubs/hygromax-630-made-with-maxim-final-submission.989/
     - https://forum.allaboutcircuits.com/ubs/impact-sensor.979/
     - https://forum.allaboutcircuits.com/ubs/autonomous-quadcopter.987/
     - https://forum.allaboutcircuits.com/ubs/makewithmaxim-sciencesensorhub.988/
     - https://forum.allaboutcircuits.com/ubs/max32630fthr-wearable-ekg.990/
     - https://forum.allaboutcircuits.com/ubs/makewithmaxim-model-rocket-data-acquisition-and-telemetry.976/

- [ ] Digikey schematics
  - https://www.digikey.com/en/schemeit/project/max32630fthr-pegasus-board-TS7G7N03027G

---

## Wed 24 Apr 2026

### experiments in attitude meter 🛩️ 🧭

<video width="740" controls>
  <source src="./docs/assets/20260423_01_attitude_meter.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>

Made a reasonable LED matrix "artificial horizon" with scrolling velocity
feedback using the onboard `BMI160` intertial measurement unit and an external
LED matrix display powered by a MAX7219 serial display driver.

The core logic is in the [./experiments/in_attitude_meter/src/main.rs](
./experiments/in_attitude_meter/src/main.rs) file.

```rust
    loop {
        // Get the X and Y acceleration
        let ax = bmi160::read_accel_x();
        let ay = bmi160::read_accel_y();

        // ay drives velocity: tilt makes bar scroll, urging user to counter-tilt
        // 2^15 = 32,768
        // Sensitivity = 16384 LSB/g (1g = 16384 counts)
        // by ay / 32768.0 we normalizes to ±1.0 where ±1.0 = ±2g (full scale).
        let velocity = -(ay as f32) / 32768.0;
        let next = bar_pos + velocity;
        let r = next % 8.0;
        bar_pos = if r < 0.0 { r + 8.0 } else { r };

        // ax tilts the bar diagonally: ±3.5 rows across 8 columns at max tilt
        let slope = (ax as f32) / 32768.0 * 3.5;

        let mut rows = [0u8; 8];
        for col in 0..8u32 {
            let col_pos = bar_pos + slope * (col as f32 - 3.5);
            let r = col_pos % 8.0;
            let wrapped = if r < 0.0 { r + 8.0 } else { r };
            let row = ((wrapped + 0.5) as usize) % 8;
            rows[row] |= 1 << col;
        }

        display.clear();
        for (i, &mask) in rows.iter().enumerate() {
            if mask != 0 {
                display.write_reg((i + 1) as u8, mask);
            }
        }

        // Refresh every ~10ms assuming 96 Mhz clock
        asm::delay(960_000); // ~10 ms
    }
```

based on the BMI160 data sheet
- [https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi160-ds000.pdf](
  https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi160-ds000.pdf)

1. Two I2C transactions per frame (easy fix)
read_accel_x() does a 2-byte read, read_accel_y() does a separate 6-byte read —
two transactions. read_accel_y() already reads all 6 bytes and discards X and
Z. One function reading all 6 at once and returning a struct would halve your
I2C traffic.

2. Z axis — already in your buffer, thrown away
The 6 bytes at REG_ACC_X_LSB give X, Y, Z in order. buf[4..5] = Z (vertical
when flat). Z lets you detect if the board is nearly flat vs steeply tilted,
and x²+y²+z² ≈ 16384² is a vibration/free-fall check.

3. Gyroscope — completely unused
The chip has a full 3-axis gyroscope — currently in suspend. Wake it with
CMD_GYR_NORMAL = 0x15 (startup takes 55 ms). Gyro data is at registers
0x0C–0x11 (same 6-byte pattern as accel). At default ±2000°/s range,
sensitivity = 16.4 LSB/°/s.

For the attitude meter this is the biggest win: gyro gives angular rate
(rotation speed in °/s), which doesn't noise-up with vibration. A simple
complementary filter blends them:

angle = 0.98 × (angle + gyro_rate × dt) + 0.02 × accel_angle
This gives stable, smooth tilt — gyro handles fast motion, accel corrects slow
drift.

4. ACC_CONF (0x40) — ODR not set explicitly
Your loop runs at ~5 ms (200 Hz) but the default accelerometer ODR is 100 Hz
(acc_odr=8). You're reading stale data half the time. Write 0x29 to ACC_CONF to
set 200 Hz, or 0x2A for 400 Hz.

5. Fast Offset Compensation — one-shot hardware calibration
From §2.9.1: a built-in calibration sequence removes mounting bias. With the
board held flat, write FOC_CONF (0x69) then issue start_foc to the CMD register
(0x7E). Takes ≤250 ms, then writes trim values into the OFFSET registers
(0x71–0x77) automatically. The accuracy is 3.9 mg. Can be saved to NVM (≤14
write cycles lifetime). This would zero out any bias so "level" truly reads
ax=0, ay=0.

6. Temperature sensor — free when gyro is active
Registers 0x20–0x21, 16-bit, 0.002°C/LSB, centre at 23°C. No extra init. Useful
for knowing if thermal drift is affecting readings.

Priority    | Change                                    | Benefit
------------|-------------------------------------------|----------------------
1           | Single burst read for X+Y+Z               | Efficiency + Z axis
2           | Set ACC_CONF ODR to 200 Hz                | No stale reads
3           | Enable gyroscope + complementary filter   | Smooth stable attitude
4           | FOC calibration at startup                | True zero at level

## Wed 23 Apr 2026

### 6 Axis acceleromenter and an attitude meter in Rust 🦀

The 6 Axis acccelerometer inside the MAX32630FTHR can be used for a balance bot
- https://www.hackster.io/justin-jordan/max32630fthr-balance-bot-621f0f

- **MAX32630FTHR Balance Bot - Justin Jordan**

  [![
    MAX32630FTHR Balance Bot - Justin Jordan
  ](
    http://img.youtube.com/vi/Uu7QbEvHTG8/0.jpg
  )](https://youtu.be/Uu7QbEvHTG8)

- **MAX32630FTHR Balance Bot part 2 - Justin Jordan**

  [![
    MAX32630FTHR Balance Bot part 2 - Justin Jordan
  ](
    http://img.youtube.com/vi/A3T340ZXMZY/0.jpg
  )](https://youtu.be/A3T340ZXMZY)

most of the code has been downloaded into
[./reference/MAX32630FTHR_balance_bot_code](reference/MAX32630FTHR_balance_bot_code)

TODO

## Tue 22 Apr 2026

### Cold restart working with Rust 🦀

Two terms that kept coming up:

- **POR** (Power-On Reset) — what happens when power is physically applied to
  the chip. Every register resets to its factory default. Nothing is remembered
  from any previous run.
- **SWD** (Serial Wire Debug) — the two-wire interface (SWDIO + SWDCLK) used
  by OpenOCD and DAPLink to program and debug ARM Cortex-M chips. An SWD reset
  is a software-triggered reset through the debug interface. **Crucially, SWD
  reset is not the same as POR** — many peripheral registers survive an SWD
  reset.

This distinction is exactly why the Rust firmware always worked after
`mise run upload:in_attitude_meter` (which ends with an SWD reset) but failed
on cold restart (POR): the previous mbed or LPSDK run had configured certain
registers, and those values survived the SWD reset but were wiped on POR.

Three things were needed to fix cold boot:

#### 1. `sys::init()` — clock, oscillator trim, flash controller

A new `experiments/in_attitude_meter/src/sys.rs` replicating what the LPSDK
does in `PreInit()` and `SystemInit()` (source:
`~/.platformio/packages/framework-mbed/targets/TARGET_Maxim/TARGET_MAX32630/device/system_max3263x.c`).

The `cortex_m_rt` crate used by Rust does generic ARM Cortex-M startup only —
it copies `.data`, zeroes `.bss`, and calls `main()`. It knows nothing about
the MAX32630's chip-specific initialisation. Three things go wrong after a POR
without it:

- **Clock source** — `CLKMAN_CLK_CTRL = 0x1` at `0x4000_0404` explicitly
  selects the 96 MHz ring oscillator. Without it the chip may run at half speed
  or an undefined frequency. (`PreInit()` in the LPSDK.)

- **Oscillator trim** — factory calibration values live in the device INFO
  block at `TRIM_PWR_REG5/6` (`0x4000_1034` / `0x4000_1038`). They must be
  copied to the power sequencer at `PWRSEQ_REG5/6` (`0x4000_0814` /
  `0x4000_0818`) on every POR, because those registers lose state on a full
  power cycle but survive an SWD reset. Without the correct trim the 96 MHz
  oscillator is uncalibrated and unstable — I2C and delay timing become
  unreliable. (`SystemInit()` in the LPSDK.)

- **Flash `AUTO_CLKDIV`** — `FLC_PERFORM |= 0x3701_0000` at `0x4000_2050`
  lets the flash controller derive its own clock divider automatically.
  OpenOCD configures this during programming (which is why warm restart always
  worked), but it resets on POR. (`SystemInit()` in the LPSDK.)

```rust
// sys.rs — called as the very first line of main()
unsafe fn sys_init() {
    CLKMAN_CLK_CTRL.write_volatile(0x0000_0001);          // 96 MHz RO
    // copy trim from INFO block → PWRSEQ (oscillator calibration)
    let trim5 = TRIM_PWR_REG5.read_volatile();
    let trim6 = TRIM_PWR_REG6.read_volatile();
    if (FLC_CTRL.read_volatile() & (1 << 25)) != 0
        && trim5 != 0xFFFF_FFFF && trim6 != 0xFFFF_FFFF
    {
        PWRSEQ_REG5.write_volatile(trim5);
        PWRSEQ_REG6.write_volatile(trim6);
    } else {
        let r6 = PWRSEQ_REG6.read_volatile();
        PWRSEQ_REG6.write_volatile((r6 & !0x01FF_0000) | (0x1E0 << 16));
    }
    let p = FLC_PERFORM.read_volatile();
    FLC_PERFORM.write_volatile(p | 0x3701_0000);          // AUTO_CLKDIV etc.
}
```

#### 2. `pmic.rs` — enable LDO3 as well as LDO2

The original mbed reference (`low_level_init.c`) only writes LDO2 because
that's the minimum needed for the MCU to run. LDO3 powers the expansion header
3V3 rail — the MAX7219 is wired there. The LPSDK `Board_Init` writes both (see
`experiments/in_blink_LPSDK/main.c`).

```rust
pmic_write(0x15, LDO_3300MV);  // LDO2_VSET — core supply
pmic_write(0x14, LDO_ENABLED); // LDO2_CFG
pmic_write(0x17, LDO_3300MV);  // LDO3_VSET — 3V3 header rail
pmic_write(0x16, LDO_ENABLED); // LDO3_CFG
```

No delay between LDO2 and LDO3 is needed (tested and confirmed).

#### 3. 100 ms delay after `pmic::init()` before touching the MAX7219

LDO3 comes up inside `pmic::init()` on cold boot. The MAX7219 needs to
complete its own internal power-on reset sequence before it will accept SPI
commands. On warm restart LDO3 was already on, so the chip was already up and
this made no difference. On cold boot, hitting the MAX7219 immediately after
`pmic::init()` returns meant the init commands were silently ignored and the
chip stayed in shutdown.

```rust
sys::init();
pmic::init();
cortex_m::asm::delay(9_600_000); // ~100 ms — let MAX7219 complete POR
let mut display = Max7219::new(...);
display.init();
```

## Tue 21 Apr 2026

### Wrestling with Rust 🦀

The last few days have been a blur of rust and AI. First it was some **pmic**
(Power Management Integrated Circuit) setting that was missing. Meant that the
blink programs I had written in both LPSDK and Rust failed to run post a cold
start - vibe coded that away. Then it was time to connect to an LED Matrix - I
actually got to pull out the multimeter to solder on the headers to be able to
connect it 👨‍🏭 . As I connect it to a MAX7219 LED driver, I realise that the 3V3
pin is not 3V3 - I think I may have overpowered the device? But vibe and AI to
the rescue, supposedly a previous vibe sesh had not changed the voltage on the
3V3 pin to 3V3. The comment

```c
// (mbed only writes LDO2; LDO3 omitted here to match the reference exactly.)
```

Luckly the board was OK and after some more vibing I worked out that  to set
the voltage output (3.3V) for the MAX14690 PMIC's LDO (Low Dropout Regulator)
you need to:

$$
V_{OUT} = V_{min} + N \times \text{step\_size}
$$

```
Register value = (V_desired - V_min) / step_size
Example: (3300 - 800) / 100 = 25

25 in HEX is 0x19
```

```c
// LDO2_VSET: (3300 - 800) / 100 = 25 = 0x19
const LDO2_3300MV: u8 = 0x19;

...

  // LDO2 (VDDB) and LDO3 (3.3V header pin) both to 3.3V.
  // LDO3 powers the expansion header 3V3 rail — needed for external peripherals.
  pmic_write(0x15, LDO_3300MV);  // LDO2_VSET
  pmic_write(0x14, LDO_ENABLED); // LDO2_CFG
  pmic_write(0x17, LDO_3300MV);  // LDO3_VSET
  pmic_write(0x16, LDO_ENABLED); // LDO3_CFG
```

and the `0x14 ... 0x17` etc are register addresses for the MAX14690 PMIC (Power
Management IC). Each register controls a specific function or setting in the
chip.

Here's what they mean in this context:

`0x14` (LDO2_CFG): Register to enable/configure LDO2.
`0x15` (LDO2_VSET): Register to set the output voltage for LDO2.
`0x16` (LDO3_CFG): Register to enable/configure LDO3.
`0x17` (LDO3_VSET): Register to set the output voltage for LDO3.

## Mon 20 Apr 2026 - late morning

### Low Power ARM Micro SDK - LPSDK - Again

Seems, the install didn't work correctly (I was using the newer MSDK without MAX32360 support). Following the steps again

  - https://www.analog.com/en/products/max32630.html
  - [Low Power ARM Micro SDK (Mac) 1.2.0](
    https://www.analog.com/en/resources/evaluation-hardware-and-software/embedded-development-software/software-download.html?swpart=SFW0001660A)
    - login
    - **`ARMCortexToolhchain.dmg`**

```sh
# check the firmware is there
ls $HOME/Maxim/Firmware/ && \
  echo "---" && \
  find $HOME/Maxim/Firmware -maxdepth 3 -name "*3263*" | \
  head -20

MAX32520 MAX32600 MAX32620 MAX32625 MAX3263X MAX32650 MAX32660 MAX32665
---
/Users/michael/Maxim/Firmware/MAX3263X
/Users/michael/Maxim/Firmware/MAX3263X/Libraries/MAX3263XPeriphDriver
```

## Mon 20 Apr 2026 - morning

### Why not Rust?

With a bit of help from AI I got [./experiments/in_blink_rust/src/main.rs](
./experiments/in_blink_rust/src/main.rs] up and running with

```sh
cd experiments/in_blink_rust
cargo build --release
mise run upload:in_blink_rust
```

but there was a bunch of HAL (Hardware Abstraction Layer) that is missing which meant the code requires some raw register pocking like `0x4000_A000 + 0x0080`

```rust
// raw register poke
(0x4000_A000 + 0x0080 + port * 4) as *mut u32).write_volatile(...)
```

The reality is

> **The problem:** to use any of those drivers you need a HAL crate that
> implements `embedded-hal` for the `MAX32630`'s `SPI/I2C/UART` peripherals. No
> such `HAL` exists. So before touching a fingerprint reader you'd need to write
> raw register drivers for `I2C` or `SPI` — the same kind of work you just did
> for `GPIO`, but more complex.
>
> **The real issue is the chip, not the language.** The MAX32630 has:
>
> * mbed EOL in June
> * No MSDK support
> * No Rust HAL
>
> If you switched to a chip with a Rust HAL you'd get the full ecosystem for
> free. Best options:
>
> - RP2040 (`rp-hal`) — largest hobbyist Rust embedded community, cheap
> - STM32 (`stm32f4xx-hal` etc.) — mature, huge peripheral driver library
> - nRF52840 (`nrf-hal`) — if you need BLE
>
> If you're committed to the `MAX32630`, Rust is still viable but you're writing
> every peripheral driver yourself from register access up.

Can I write the HAL myself? is it worth it?

> **How hard to write for MAX32630:**
>
> The register definitions are the easy part — `wez/max32630` on **GitHub**
> already generated a `PAC` from the `SVD`, so the typed register access exists.
> Building a **HAL** on top means implementing embedded-hal traits for each
> peripheral:
>
> Peripheral          | Effort                 | Notes
> --------------------|------------------------|---------------------------------
> GPIO (Input/Output) | Done — you just did it | ~50 lines
> SPI                 | Medium — ~200 lines    | need clock config, modes
> I2C                 | Hard — ~300 lines      | clock stretching, error handling
> UART                | Medium                 | easier than I2C
> Timers              | Medium                 | needed for real delays
> ADC                 | Medium                 |
>
> Total realistic estimate: **2–4 weeks** of focused work to get `SPI` + `I2C` +
> `GPIO` to a usable state — the minimum needed for your fingerprint/`NFC` use
> case.
>
> It's a legitimate project (the `wez/max32630` `PAC` is a starting point), but
> you'd essentially be building infrastructure before building your actual
> product. For a new project that's probably not the best use of time.

Well not sure about the **2-4 weeks** if it has already pretty much sorted 1/25
of it? The bigger problem is that no one would see any benefit in it as the chip
is pretty much marked obsolete

## Mon 20 Apr 2026 - overnight

### MSDK and LPSDK

Decided to move off the **mbed** platform and try the **MSDK** platfrom,MAXIM
Micro SDK, which is 6GB worth of content even without selecting Eclipse. After
downloading the installer (see below on Sun 19 Apr - Setup project) and
overriding Apple to open the DMG and then Open the installer, I chose to install
it into `/usr/local/bin/MaximSDK`. Leaving it to download and install ...

**6GB of MSDK** and ...

Seems that the MSDK I downloaded is for "newer" `MAX32690` or `MAX78000` - I
need the older LPSDK (Low Power SDK) to get support for the legach `MAX32360`

Back to Element14 community post by [@arvindsa identity protocol - part 3](
https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56840/identity-protocol---part-3---unboxing-and-blinking-with-maxim-lpsdk)

go to
  - https://www.analog.com/en/products/max32630.html
  - [Low Power ARM Micro SDK (Mac) 1.2.0](
    https://www.analog.com/en/resources/evaluation-hardware-and-software/embedded-development-software/software-download.html?swpart=SFW0001660A)
    - login
    - **`ARMCortexToolhchain.dmg`**

running install mostly may have worked, got this error

```sh
Could not fetch archives: Downloading hash signature failed.
Error while loading http://www.mxim.net/product/dist/max32665/com.maximintegrated.dist.max32665.toolchain/0.6.6Toolchain.7z.sha1
```

not the `MAX32360` so might be OK

checking if the correct libraries are found

```sh
ls /usr/local/bin/MaximSDK/Libraries/CMSIS/Device/Maxim/ && \
  echo "---" && \
  ls /usr/local/bin/MaximSDK/Examples/ | \
  grep -i "MAX326"

GCC      MAX32650 MAX32660 MAX32665 MAX32672 MAX32680 MAX78000
MAX32520 MAX32655 MAX32662 MAX32670 MAX32675 MAX32690 MAX78002
---
MAX32650
MAX32655
MAX32660
MAX32662
MAX32665
MAX32670
MAX32672
MAX32675
MAX32680
MAX32690

# and
find /usr/local/bin/MaximSDK -name "*32630*" -o -name "*LPSDK*" 2>/dev/null | \
  head -20 && \
  echo "---" && \
  ls /usr/local/bin/ | \
  grep -i maxim

/usr/local/bin/MaximSDK/Tools/SBT/src/build_scp_session/include/ucl/max32630_crypto.h
/usr/local/bin/MaximSDK/Tools/SBT/src/sign_app/include/ucl/max32630_crypto.h
---
MaximSDK
```

from some internet searching

> The LPSDK appears to have been quietly retired. All the old Maxim repos have
> migrated to `analogdevicesinc` and the LPSDK is not there — only mbed-based
> examples using it remain.
>
> **Bottom line:** The LPSDK is effectively gone. Analog Devices replaced it
> with the MSDK which dropped MAX32630 support entirely.
>
> The real options for MAX32630 seem to be:
>
> * mbed (works now, EOL June 2026)
> * Rust (working today, as you just proved) - see above ^^
>
> Sources:
>
> * [analogdevicesinc GitHub - MAX32630 repos](https://github.com/analogdevicesinc?q=MAX32630)
> * [MaximIntegratedTechSupport migration notice](https://github.com/MaximIntegratedTechSupport)

## Sun 19 Apr 2026

### Setup project

```sh
brew install platformio
# and dependencies
brew install certifi python@3.14
```

setup a PlatformIO project

```sh
platformio project init
```

```sh
# in PlatformIO searching for 32630
# and clicking link
# https://docs.platformio.org/en/latest/boards/maxim32/max32630fthr.html
# add to platform.ini
[env:max32630fthr]
platform = maxim32
board = max32630fthr
```

Following [Forum Thread 2 EchoGuard – MAX32630FTHR Setup & First Blink Program Upload - Nidhee](
https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56852/forum-thread-2-echoguard-max32630fthr-setup-first-blink-program-upload)

```sh
brew install open-ocd

# find the board
find $(dirname $(which openocd))/../share/openocd/scripts -name "max3263*.cfg"
/opt/homebrew/bin/../share/openocd/scripts/target/max3263x.cfg

# and update the platform.ini
[env:max32630fthr]
platform = maxim32
board = max32630fthr

framework = mbed ; or arduino depending on your preference

upload_protocol = custom
upload_command = openocd

debug_tool = cmsis-dap
```

Seems to do a full compile each time and ultimately the open-ocd upload fails

Trying from source

```sh
git clone https://github.com/analogdevicesinc/openocd --depth 1
rm -rf openocd/.git
cd openocd

brew install autoconf automake libtool pkg-config libusb hidapi
brew install jimtcl

./bootstrap

# ./configure --enable-cmsis-dap
# ./configure --enable-cmsis-dap --with-jimtcl-static
# ./configure --enable-cmsis-dap --disable-xds110
./configure --enable-cmsis-dap --disable-xds110 \
  CFLAGS="-g -O2 -Wno-error=gnu-folding-constant"

make -j$(sysctl -n hw.ncpu)
sudo make install
```

which installs it to

```sh
which openocd
/usr/local/bin/openocd
```

should probably follow the instructions on Analog Devices site
- [https://analogdevicesinc.github.io/msdk//USERGUIDE/#completing-the-installation-on-macos](
  https://analogdevicesinc.github.io/msdk//USERGUIDE/#completing-the-installation-on-macos)

```sh
brew install libusb-compat libftdi hidapi libusb
```

Download:
- [https://analogdevicesinc.github.io/msdk//USERGUIDE/#download](
  https://analogdevicesinc.github.io/msdk//USERGUIDE/#download)
  - [https://www.analog.com/en/resources/evaluation-hardware-and-software/embedded-development-software/software-download.html?swpart=SFW0018610B](
    https://www.analog.com/en/resources/evaluation-hardware-and-software/embedded-development-software/software-download.html?swpart=SFW0018610B)
    - sign up for an account

Back in just using the manually built and installed openocd

I decided to move my experiments into `./experiments` folder this caused an
issue with the command line `pio` (platformio) program finding the wrong python
`python@3.14` from homebrew and the `framework-mbed` is too old. The
recommendation was to brew uninstall platformio and use mise to install a
specific python and pip to install platformio

```sh
brew uninstall platformio
brew uninstall --ignore-dependencies python@3.14

mise use python@3.11
pip install platformio

# now
which pio
$USER/.local/share/mise/installs/python/3.11/bin/pio
```

Now the whole thing is a bit more `mise` dirven

```sh
mise run build
mise run upload
```

### Start blogging

setup jekyll and Github pages for https://saramic.github.io/sentinel-box/

install ruby

```sh
# attempt mise self update
mise self-update
# mise ERROR mise is installed via a package manager, cannot update

# realise I isntalled it via homebrew, hence
brew update mise

# list the local rubies
mise list ruby

# and all sources
mise list --all-sources ruby

# finally worked out to list remote ones
mise ls-remote ruby

# and install the latest 4.0.2
mise install ruby 4.0.2
mise use ruby@4.0.2
```

install jekyll following [https://jekyllrb.com/docs/](
https://jekyllrb.com/docs/)

```sh
gem install jekyll bundler
jekyll new docs

# run it
mise run dev-blog
```

but will ruby 4 and jekyll 4.4 run on github pages? do I need the [github-pages
GEM](https://github.com/github/pages-gem)?

and configuring the `main` branch and `./docs` directory to be a **Pages** via
[https://github.com/saramic/sentinel-box/settings/pages](
https://github.com/saramic/sentinel-box/settings/pages)

**NO**

That only exposes the site and does not actually execute **Jekyll** to build
it. Attempting to add the `github-page` gem blows up wit

```less
bundle add github-pages
[DEPRECATED] Platform :mingw, :x64_mingw, :mswin will be removed in the future. Please use platform :windows instead.
Fetching gem metadata from https://rubygems.org/.........
Resolving dependencies...
Could not find compatible versions

    Because github-pages >= 135, < 178 depends on minima = 2.1.1
      and github-pages >= 44, < 147 depends on liquid = 3.0.6,
      github-pages >= 44, < 178 requires minima = 2.1.1 or liquid = 3.0.6.
(1) So, because github-pages >= 178 depends on jekyll-sass-converter = 1.5.2
      and github-pages >= 28, < 44 depends on jekyll = 2.4.0,
      github-pages >= 28 requires minima = 2.1.1 or liquid = 3.0.6 or jekyll-sass-converter = 1.5.2 or jekyll = 2.4.0.

    Because github-pages >= 9, < 14 depends on kramdown = 1.2.0
      and github-pages < 9 depends on kramdown = 1.0.2,
      github-pages < 14 requires kramdown = 1.0.2 OR = 1.2.0.
    And because github-pages >= 14, < 32 depends on kramdown = 1.3.1,
      github-pages < 32 requires kramdown = 1.0.2 OR = 1.2.0 OR = 1.3.1.
    And because github-pages >= 28 requires minima = 2.1.1 or liquid = 3.0.6 or jekyll-sass-converter = 1.5.2 or jekyll = 2.4.0 (1),
      one of minima = 2.1.1 or liquid = 3.0.6 or jekyll-sass-converter = 1.5.2 or jekyll = 2.4.0 or kramdown = 1.0.2 OR = 1.2.0 OR = 1.3.1 must be true.
    And because jekyll >= 4.3.0 depends on jekyll-sass-converter >= 2.0, < 4.0,
      jekyll >= 4.3.0 requires minima = 2.1.1 or liquid = 3.0.6 or kramdown = 1.0.2 OR = 1.2.0 OR = 1.3.1.
    And because jekyll >= 4.3.0 depends on kramdown >= 2.3.1, < 3.A
      and jekyll >= 3.5.0 depends on liquid ~> 4.0,
      jekyll >= 4.3.0 requires minima = 2.1.1.
    So, because Gemfile depends on jekyll ~> 4.4.1
      and Gemfile depends on minima ~> 2.5,
      version solving has failed.
```

at this point, I'm just going to go back to what I know works

```sh
# downgrade to a ruby that should work
mise use ruby@3.2.2
gem install jekyll bundler

# re-create jekyll docs blog
rm -rf docs
jekyll new docs

# add required gems github-pages AND webrick
cd docs
bundle add github-pages
bundle add webrick

# check jekyll works locally
```

seems to build but still not showing a built page in GitHub pages

Also in the GHActions build, I notised a **Warning** which may allow me to
update the version of Jekyll
* [https://jekyllrb.com/docs/continuous-integration/github-actions/](
  https://jekyllrb.com/docs/continuous-integration/github-actions/)

Finally to decide on a better theme:
* [https://docs.github.com/en/pages/setting-up-a-github-pages-site-with-jekyll/adding-a-theme-to-your-github-pages-site-using-jekyll](
  https://docs.github.com/en/pages/setting-up-a-github-pages-site-with-jekyll/adding-a-theme-to-your-github-pages-site-using-jekyll)
  - [Architect](https://pages-themes.github.io/architect/) probably a winner
    with a clear "blue print" style
  - [Caymen](https://pages-themes.github.io/cayman/) nice and clean and more
    modern than the original
  - [Hacker](https://pages-themes.github.io/hacker/) dark and in theme but
    would need some tweaking
  - [leap-day](https://pages-themes.github.io/leap-day/) a bit busy but with
    tweaking could work
  - [minima](https://jekyll.github.io/minima/) seems similar but just a little
    nicer than the default? - but supposedly this is the default in
    `_config.yml`
  - Others that don't really rate:
    [dinky](https://pages-themes.github.io/dinky/),
    [Merlot](https://pages-themes.github.io/merlot/),
    [Midnight](https://pages-themes.github.io/midnight/),
    [Minimal](https://pages-themes.github.io/minimal/)

attempted with the following but did not work so giving up for time being (also
added the `assets/css/style.scss` file, leaving that as it doesn't break
anything)

```diff
diff --git a/docs/_config.yml b/docs/_config.yml
index e3aabcb..e1f68a7 100644
--- a/docs/_config.yml
+++ b/docs/_config.yml
@@ -35,8 +35,10 @@ timezone: Australia/Melbourne
 # Build settings
 markdown: kramdown
+# theme: minima
+remote_theme: pages-themes/architect@v0.2.0
 plugins:
   - jekyll-feed
+  - jekyll-remote-theme

 # Exclude from processing.
 # The following items will not be processed, by default.
```

## Thu 16 Apr 2026

Seems there are a lot of write ups on how to program the **MAX32630FTHR**. I
think I will attempt to use VSCode and Platform.io but in reality a `make`
script with a command line build would be preferable. Some information here

- **GitHub: analogdevicesinc/msdk** Software Development Kit for Analog
  Device's MAX-series microcontrollers
  - [https://github.com/analogdevicesinc/msdk?tab=readme-ov-file](
    https://github.com/analogdevicesinc/msdk?tab=readme-ov-file)

- how to setup MSDK for commandline
  - [https://analogdevicesinc.github.io/msdk//USERGUIDE/#getting-started-with-command-line-development](
    https://analogdevicesinc.github.io/msdk//USERGUIDE/#getting-started-with-command-line-development)

## Mon 13 Apr 2026

I don't have a **MAX32630FTHR** but I do have other options to get started
with:

- **Arduino Nano 33 BLE Sense** — _technically don't have it but was thiking of
  getting one_ the most beginner-friendly with the richest
  on-board sensor suite (IMU, mic, pressure, temp, light). Best for rapid ML
  prototyping with Edge Impulse, but its sleep current is notably worse than
  the others.
- **SAMD21 (MKR / Feather M0)** — the weakest TinyML candidate. No FPU, no DSP,
  minimal RAM, no sensors. Only viable for very simple inference or tight
  budgets where you add external sensors.
- Seeed [XIAO nRF52840 Sense](https://wiki.seeedstudio.com/XIAO_BLE/) - best
  form factor by far (postage-stamp size) with BLE 5, built-in IMU + mic,
  onboard LiPo charging, and excellent deep sleep. Ideal for compact wearables.
- **SparkFun Edge (Apollo3 Blue)** — the standout for always-on audio and power
  efficiency. The Apollo3's burst mode + 1 µA sleep makes it the king of
  battery-powered keyword spotting deployments.
- **MAX32630FTHR** — the most RAM (512 KB) and flash (2 MB) of the group,
  suited to medical and industrial use cases where you need headroom for larger
  models, but has weaker community support and costs more.


### The Sentinel Box — Unlock Mechanisms (Easiest → Most Absurd)

1. **Wake Word**

   Say "Open Sesame" (or whatever you program). Classic TinyML keyword spotting
   via the external PDM mic you'll add. Straightforward first win.

2. **Morse Code Tap**

   Tap a secret pattern on the box via a vibration/piezo sensor. Kids have to
   learn Morse. Delightfully old-school. Easy to implement, hard to guess.

3. **QR Code Scan**

   Phone displays a QR code, a camera module on the box reads and validates it.
   You rotate the QR value daily via a lambda so yesterday's screenshot doesn't
   work.

4. **TOTP NFC Card**

   Phone or card writes a time-based one-time password via NFC. Valid for 30
   seconds only. Dead cool to demo with a card writer. Genuinely teaches kids
   how banking auth works.

5. **Fingerprint — Single Parent**

   One registered parent fingerprint required. Clean, physical, hard to fake.
   Good introduction to biometric concepts.

6. **Audio Quiz via Lambda**

   Box speaks a question (streamed from your lambda), you speak the answer
   back, lambda validates via speech-to-text. Question changes daily. This is
   absolutely possible — the mic captures audio, you stream it to your lambda,
   AWS Transcribe or Whisper processes it, response comes back. Latency is the
   main challenge.

7. **Hand Gesture Sequence**

   Perform a specific sequence of gestures in front of the box (wave left, wave
   right, thumbs up). IMU-based if wearing a glove with sensor, or camera-based
   gesture recognition. Feels like a magic spell.

8. **Dual Fingerprint — Both Parents**

   Both parents must scan within a 30-second window. Introduces the concept of
   multi-party authorisation — same principle as nuclear launch codes. Kids
   will be furious.

9. **GPS Geofence Check**

   Box pings its own GPS location to your lambda. If the box has been moved
   outside the home geofence, all unlock mechanisms are disabled. Physically
   moving it to grandma's house doesn't help. Teaches kids that context
   matters.

10. **Secret Handshake via IMU**

    Box has an IMU. You physically pick up and shake/tilt/rotate the box in a
    specific sequence — like a combination lock but with motion axes. Recorded
    gesture pattern must match within tolerance.

11. **Rhythm Tap Pattern**

    Tap the box to the rhythm of a specific song (you define the BPM pattern).
    Piezo sensor captures timing. Harder than Morse — requires musical memory,
    not just code.

12. **Phone Compass Orientation**

    Your phone app reads its compass heading and you must physically point it
    in a specific secret direction (e.g. exactly NNE) and hold for 3 seconds.
    Lambda validates. Completely invisible mechanism — no visible sensor on the
    box.

13. **Two-Phone Proximity**

    Both parents must have their phones within Bluetooth range of the box
    simultaneously. Box detects two specific BLE beacons before even allowing
    the primary unlock mechanism to proceed. No sneaking off to unlock it
    alone.

14. **Time-Locked with Astronomical Trigger**

    Box only allows unlock attempts during a specific daily window — but the
    window is calculated from that day's local sunset time, fetched by your
    lambda. Not a fixed clock time. Kids can't predict it without looking up
    astronomical data.

15. **Voice Stress / Tone Classifier**

    Speak the wake word but the model also classifies whether your voice sounds
    calm vs. panicked/forced. If it detects a stressed voice pattern (a child
    doing an impression), it rejects and triggers a honeypot. Genuinely creepy
    and impressive at a demo.

16. **Visual Secret — Specific Object Shown to Camera**

    Hold up a specific physical object (a red Lego brick, a specific toy) to
    the camera. A tiny image classifier on the MAX32630 recognises it. The
    "key" is a physical object that lives on your keyring.

17. **Multi-Factor Chain**

    Any 3 of the above mechanisms must be completed in sequence within 60
    seconds of each other. Kids solving one mechanism triggers a countdown for
    the next. Fail any step, reset. This isn't a new sensor — it's an
    orchestration layer that makes all the above dramatically harder.

18. **Honeypot Mode — The Decoy**

    A clearly labelled "EASY UNLOCK" button that does nothing except silently
    text both parents that a child attempted to use it, plays a fake unlocking
    sound, and then claims a "system error." You know immediately. They think
    they nearly had it.

### Core Hardware List

Component                       | What & Why
--------------------------------|-----------
MAX32630FTHR                    | The brain. Runs all local inference, drives the motor, orchestrates unlock logic
Stepper motor + A4988/DRV8825   | driverDrives the vault mechanism. Stepper gives you precise rotational control for the locking bolt. Driver handles current the MAX32630 can't supply directly
Perspex enclosure + servo-actuated latch    | A servo or small stepper turns a cam that physically moves a bolt. 3D print the bolt mechanism
ICM-42688-P IMU breakout        | Gesture sequences, shake pattern unlock, tilt combination. I2C to the MAX32630
Piezo vibration sensor          | Tap patterns, Morse code, rhythm detection. Analogue input, very cheap
PDM MEMS microphone (ICS-43434 or SPH0641)  | Wake word, audio quiz capture, voice stress analysis. PDM interface to MAX32630
Arducam Mini 2MP (OV2640, SPI)  | QR code reading, object recognition unlock, gesture via vision. SPI to MAX32630
Optical fingerprint sensor (R307 or AS608)  | Parent fingerprint(s). UART interface, has its own onboard template matching
PN532 NFC module                | Read/write NFC cards and phones. I2C or SPI to MAX32630. Handles TOTP card reading
GPS module (u-blox NEO-6M or NEO-M8N)       | Geofence validation. UART to MAX32630. NEO-M8N is more accurate
ESP32 or ESP8266 co-processor   | Wi-Fi bridge. MAX32630 has no Wi-Fi — this handles lambda calls, audio streaming, BLE beacon scanning. UART to MAX32630
Small speaker + PAM8403 amp     | Plays audio quiz questions, fake unlock sounds, honeypot feedback
WS2812B LED strip (small)       | Visual feedback on unlock state, honeypot animations, countdown timers
LiPo battery + TP4056 charger   | Portable power so moving it doesn't mean it dies
Tactile buttons (x3-4)          | Manual admin reset, pairing mode, honeypot button

### Purchase list

- ✅ [Digikey: MAX32630FTHR](https://www.digikey.com.au/en/products/detail/analog-devices-inc-maxim-integrated/MAX32630FTHR/6575544) $61
- ✅ [Digikey: AdaFruit 4690 fingerprint sensor](https://www.digikey.com.au/en/products/detail/adafruit-industries-llc/4690/13170958) $30
  - or
  - [AliExpress: R307 finger print sensor](https://www.aliexpress.com/item/32815391770.html) $16
- [Digikey: AdaFruit 364 NFC evaluation board](https://www.digikey.com.au/en/products/detail/adafruit-industries-llc/364/6238001) $44
  - or
  - [AliExpress: PN532 NFC Arduino board](https://www.aliexpress.com/item/1005006837891461.html) $13/4pcs
- [Digikey: DFRobot DFR0119-0 Eval board for PAM8403 Amp](https://www.digikey.com.au/en/products/detail/dfrobot/DFR0119-O/13978501) $7
  - or
  - [AliExpress: PAM8403 Audio Amp](https://www.aliexpress.com/item/1005010021895446.html) $3/10pcs
- [AliExpress: TTP223 Touch Sensor](https://www.aliexpress.com/item/1005006087171183.html) $9/70pcs
- [AliExpress: OV2640 Camera Module 2MP Megapixel](https://www.aliexpress.com/item/33046344720.html) $8

and related-ish

- [AliExpress: AD8317 RF Signal Power Meter](https://www.aliexpress.com/item/1005009041453030.html) $10
- ✅ [DigiKey: DRV2605L eval board (haptic driver)](https://www.digikey.com.au/en/products/detail/adafruit-industries-llc/2305/5356831) $9
- ✅ [DigiKey: Vibrating Motor](https://www.digikey.com.au/en/products/detail/olimex-ltd/VIBRATING-MOTOR/21661954) $1
- ✅ [DigiKey: IRLML6344 N channel MOSFET](https://www.digikey.com.au/en/products/detail/infineon-technologies/IRLML6344TRPBF/2538152) $1

### On Box mechanisms

- valut like mechanism
  - [https://www.instructables.com/Simple-Vault-Mechanism/](
    https://www.instructables.com/Simple-Vault-Mechanism/)
  - using timber but simple mechanism of 1 spinning centre piece moving 4
    separate outer bars

- 3D printed vault with gears
  - [https://makerworld.com/en/models/988716-key-safe-vault-bank-money-bank-piggy-bank#profileId-963873](
     https://makerworld.com/en/models/988716-key-safe-vault-bank-money-bank-piggy-bank#profileId-963873)

- [https://makezine.com/projects/keyless-lock-box/](
  https://makezine.com/projects/keyless-lock-box/)
  - using a bolt on the end of a servo to hook around a metal bar
  - uses Arduino
  - and a Parallax OFN, optical finger navigation, sensor as a combination
    decoder.
    - acts like a mini track pad
