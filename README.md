# Sentinel Box

Element 14 Design Challenge

- [https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance](https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance)

[![CI](https://github.com/saramic/sentinel-box/actions/workflows/ci.yml/badge.svg)](https://github.com/saramic/sentinel-box/actions/workflows/ci.yml)

# SentinelBox: Smart Secure Storage for Families

SentinelBox — Intelligent Device Lockbox Powered by MAX32630FTHR

## Blog

[https://saramic.github.io/sentinel-box/](https://saramic.github.io/sentinel-box/)

## Project Summary

SentinelBox is a smart secure storage system designed to control access to
devices such as gaming consoles, tablets, or phones. The system demonstrates
how the **MAX32630FTHR** platform can implement an intelligent access control
system that combines identity verification, time-based policies, and behavioral
authentication.

Two **MAX32630FTHR** boards manage authentication and actuation. A stepper
motor FeatherWing physically locks the box, while the ICLED display shows
system status and alerts. The Ethernet FeatherWing provides secure logging and
parental monitoring features.

SentinelBox demonstrates how embedded AI can support healthy device usage
policies and secure storage in households or schools.

## Problem Statement

Many families struggle with controlling device access for children or shared
environments. Simple locks or software restrictions can be bypassed.

SentinelBox demonstrates a physical and intelligent solution that integrates
authentication, scheduling, and monitoring into a secure storage system.

## Key Features

- Time-based unlock policies
- Voice authentication
- Authorized phone presence detection
- Manual override for guardians
- Usage logs via Ethernet

## Configuration

### System States and LED Indicators

LED colour conventions follow the Bluetooth SIG advertising guidelines (blue =
BLE discoverable), traffic-light safety semantics (green = safe/locked, red =
alert/open), and common IoT device patterns from the Adafruit Feather
ecosystem and Nordic DK boards.

| State                     | LED Pattern                         | Matrix Display           | Meaning                          |
| ------------------------- | ----------------------------------- | ------------------------ | -------------------------------- |
| Unconfigured / Virgin     | Blue, slow pulse (1 s on / 1 s off) | _(blank)_                | Fresh firmware, no config stored |
| BLE Advertising           | Blue, double-blink every 2 s        | _(blank)_                | Waiting for setup connection     |
| BLE Connected — Setup     | Cyan solid (G+B on)                 | `SETUP`                  | Browser connected, wizard active |
| Setup — Vault Tune        | Yellow pulse (R+G on)               | step count               | Calibrating stepper travel       |
| Setup — Fingerprint Enrol | Magenta pulse (R+B on)              | slot `FP1`…`FP10`        | Enrolling fingerprints           |
| Armed & Locked            | Green breathing (3 s fade)          | padlock icon             | Secured, ready for use           |
| Awaiting Authentication   | Amber fast blink (R+G, 4 Hz)        | fingerprint icon         | Scan requested                   |
| Auth Granted — Opening    | Green triple-flash                  | open padlock             | Valid scan, motor running        |
| Vault Open                | Green solid                         | open padlock + countdown | Relock timer counting down       |
| Auth Failed               | Red 3× rapid flash                  | `FAIL`                   | No match, returns to Armed       |
| Guardian Override         | White slow pulse (R+G+B on)         | `ADMIN`                  | Manual unlock, no auth           |
| Error / Fault             | Red SOS (· · · — — — · · ·)         | `ERR`                    | System fault, check hardware     |

---

### Setup Flow (Chrome Web Bluetooth — one-time after firmware flash)

Setup uses the [Web Bluetooth API](https://developer.chrome.com/docs/capabilities/bluetooth)
(Chrome 56+ or Edge 79+, requires HTTPS or `localhost`). No native app needed.

Config is written to the MAX32630 INFO flash page 1, which survives firmware
updates. On every boot the firmware checks a magic word + CRC16; if either
fails the device boots into the unconfigured advertising state.

#### Phase 1 — Discover & Connect

1. Power on → **Blue slow pulse**, advertising as `SentinelBox` over BLE.
2. Open the Chrome setup page → click **Pair** → browser scans for `SentinelBox`.
3. On connection the firmware shifts to **Cyan solid**; the wizard page loads
   the current (blank) config and shows the three-phase progress bar.

#### Phase 2 — Vault Door Calibration

Determines how many stepper steps fully engage the lock bolt from the home
(open) position. The 28BYJ-48 delivers 2 038 steps/rev in wave-drive mode;
a typical bolt throw needs 200–600 steps depending on the mechanism.

1. Wizard sends `CMD_SETUP_VAULT` on GATT characteristic `0xF010`.
2. Firmware enters **Vault Tune** state → **Yellow pulse**.
3. Matrix displays current step count; motor moves from home to that position.
4. Adjust travel:
   - **Rotary encoder turn** → ±10 steps per click (fine)
   - **Chrome page coarse buttons** → ±100 steps
5. When the bolt sits fully engaged, press the encoder button (or click
   **Confirm** in the wizard) → firmware drives the motor back to home as a
   verification run.
6. Operator confirms open looks correct → step count committed to flash.

#### Phase 3 — Fingerprint Enrolment

Up to 10 fingerprints; at least one must be marked **Adult** before setup can
proceed. Enrolment follows the AS608 / GT521Fx standard 3-scan sequence
(place → lift → place again → template stored internally on the sensor).

1. Wizard sends `CMD_SETUP_FP` → **Magenta pulse**.
2. Matrix shows current slot (`FP1`…`FP10`).
3. Wizard form: **Name** (≤ 8 chars, scrolled on matrix during operation) +
   **Role** toggle (Adult / Child).
4. Click **Enrol** → firmware requests 3 scans:
   - Matrix: `SCAN` → `LIFT` → `SCAN` → `OK` (or `RETRY` on poor read)
   - Success: single **Green flash**, slot advances
   - 3 consecutive failures: **Red flash**, slot skipped with a warning
5. Repeat for each fingerprint; slots can be left empty and filled later via
   a re-enrol flow (single-slot update, no full reset needed).

#### Phase 4 — Policy & Completion

1. Wizard presents policy options:
   - **Unlock requires**: `Any enrolled finger` | `1 Adult` | `1 Adult + 1 Child` | `2 Adults`
   - **Auto-relock after**: `5 min` | `10 min` | `30 min` | `Never`
2. Confirm → wizard sends full config blob via `CMD_WRITE_CONFIG` (`0xF014`).
3. Firmware writes config to INFO flash, verifies readback.
4. **Three slow Green pulses** → transitions to **Armed & Locked** (Green
   breathing). Matrix briefly shows `DONE` then the padlock icon.

---

### Normal Operation (Post-Setup)

#### Locking the Vault

- Press rotary encoder button **once** (while open) → motor drives to locked
  position → **Green breathing** + padlock icon.

#### Unlocking the Vault

- Press rotary encoder button **once** (while locked) → **Amber fast blink** +
  fingerprint icon → waiting for scan (30 s timeout).
- Valid scan received:
  - Policy `Any` or `1 Adult` → single matching scan → **Green triple-flash**
    → vault opens.
  - Policy `1 Adult + 1 Child` → first scan accepted (LED stays Amber), matrix
    shows `2nd` → awaiting second matching scan of the opposite role.
  - Timeout with no valid scan → returns silently to **Armed & Locked**.
- Vault stays open until:
  - Auto-relock timer expires → motor closes, **Green breathing**.
  - Short-press encoder → closes immediately.

#### Guardian / Adult Override

- **Long-press** encoder (3 s) → **White slow pulse** + `ADMIN` on matrix →
  vault opens unconditionally.
- Short-press encoder to relock.

---

### BLE GATT Service Map

Service UUID: `0x180A` (Device Information) for discovery; custom
characteristics under service `0xFF00`:

| UUID     | Name          | Properties   | Notes                                                                                         |
| -------- | ------------- | ------------ | --------------------------------------------------------------------------------------------- |
| `0xF001` | Device Status | Read, Notify | Current state enum (1 byte)                                                                   |
| `0xF002` | LED Override  | Read, Write  | `0=off 1=red 2=green 3=blue` (debug)                                                          |
| `0xF010` | Setup Command | Write        | `0x01=CMD_SETUP_VAULT`, `0x02=CMD_SETUP_FP`, `0x03=CMD_SETUP_POLICY`, `0x04=CMD_WRITE_CONFIG` |
| `0xF011` | Vault Steps   | Read, Write  | u16 LE — steps from home to locked                                                            |
| `0xF012` | FP Slot       | Read, Write  | Active slot index 0–9                                                                         |
| `0xF013` | FP Metadata   | Read, Write  | 8 B name (null-padded) + 1 B role (`0=Adult 1=Child`)                                         |
| `0xF014` | Config Blob   | Write        | Full serialised config, CRC16 appended                                                        |

---

### Persistent Config Layout (INFO Flash Page 1)

```
Offset  Size  Field
──────  ────  ─────────────────────────────────────────────────────
0x00    2     magic            0x5B5A  ("SB" little-endian) — valid config marker
0x02    2     vault_steps      u16 LE  steps from home to locked position
0x04    1     fp_count         number of enrolled fingerprint slots (0–10)
0x05    1     unlock_policy    0=Any  1=1Adult  2=1Adult+1Child  3=2Adults
0x06    1     relock_minutes   0=never, else minutes until auto-relock
0x07    1     reserved         (pad to alignment)
0x08    80    fp_name[10]      8 bytes per slot, null-padded ASCII name
0x58    10    fp_role[10]      1 byte per slot: 0=Adult, 1=Child
0x62    2     crc16            CRC-16/CCITT over bytes 0x00–0x61
```

On boot: if `magic != 0x5B5A` or CRC16 does not match → device is unconfigured
→ **Blue slow pulse** advertising.

## Setup and build

> **NOTE:** requires the download and installation of **LPSDK** (Low Power ARM
> Micro SDK) which is the legacy SDK with support for MAX32360 (new MSDK starts
> support from MAX32690)
>
> - https://www.analog.com/en/products/max32630.html
> - [Low Power ARM Micro SDK (Mac) 1.2.0](https://www.analog.com/en/resources/evaluation-hardware-and-software/embedded-development-software/software-download.html?swpart=SFW0001660A)
>   - login
>   - **`ARMCortexToolhchain.dmg`**

```sh
mise

# default PlatfromIO project - Mbed blink
# blinks 500ms ON/OFF
mise run build
mise run upload
mise run upload

# LPSDK based blink
# blinks RED🔴/GREEN🟢/BLUE🔵 LED based on code by @arvindsa
mise run clean:in_blink_LPSDK
mise run build:in_blink_LPSDK
mise run upload:in_blink_LPSDK

# Rust 🦀 based blink
# blinks 120ms ON/OFF
mise run clean:in_blink_rust
mise run build:in_blink_rust
mise run upload:in_blink_rust
```

### OpenOCD - build and install

OpenOCD can be built and installed form source via
a fork. When programming in Rust this means there
is no need to install LPSD.

```sh
# get the code
git clone https://github.com/analogdevicesinc/openocd --depth 1
rm -rf openocd/.git
cd openocd

# install some libraries requierd for building the package
brew install autoconf automake libtool pkg-config libusb hidapi jimtcl

  # UNTESTED
  # "likely" equivalent if using linux 🐧
  sudo apt-get install -y \
    autoconf automake libtool pkg-config \
    libusb-1.0-0-dev libhidapi-dev libjim-dev

./bootstrap

# some configurations to deal with some harmless warning
./configure \
  --enable-cmsis-dap \
  --disable-xds110 \
  CFLAGS="-g -O2 -Wno-error=gnu-folding-constant"

make -j$(sysctl -n hw.ncpu)

# install like a boss into /usr/local/bin/openocd
sudo make install

# you can now remove the directory where openocd was built
rm -rf openocd
```
