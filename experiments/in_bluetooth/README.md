# in_bluetooth

Brings up the **PAN1326B** Bluetooth module on the MAX32630FTHR using raw HCI
over UART0 (Maxim LPSDK, bare-metal C). On power-up:

1. Initialises UART0 (115200 baud, Mapping B, hardware flow control, 1.8V pins)
2. Enables 32 kHz RTC output on P1.7 for the module clock reference
3. Releases the PAN1326B from reset via P1.6
4. Sends **HCI Reset** and verifies the module responds
5. Attempts to start **BLE advertising** as `SentinelBox`

LED status:

| Colour       | Meaning                                                      |
|--------------|--------------------------------------------------------------|
| Blue         | Hardware initialising                                        |
| Red          | HCI Reset failed — check wiring                             |
| Yellow       | Module alive but LE commands rejected (need CC256XB service pack — see below) |
| Green blink  | Advertising as `SentinelBox` — visible to phones and Chrome  |

## Hardware

### Components

| Item               | Notes                                              |
|--------------------|----------------------------------------------------|
| MAX32630FTHR       | Host MCU, 3.3V supply rail                         |
| PAN1326B           | Panasonic module containing TI CC2564B BT/BLE chip |

### Pin mapping

The PAN1326B is soldered directly on the MAX32630FTHR board. No external wiring
is needed — these pins are internal connections.

| Signal             | MAX32630FTHR | Direction | Notes                                         |
|--------------------|--------------|-----------|-----------------------------------------------|
| UART0 RX           | P0.0         | Input     | Mapping B crossover — Map A puts TX here      |
| UART0 TX           | P0.1         | Output    | Mapping B crossover — Map A puts RX here      |
| UART0 CTS input    | P0.2         | Input     | Module (PAN1326B RTS) signals MCU may transmit |
| UART0 RTS output   | P0.3         | Output    | MCU signals module it may transmit            |
| nSHUTD (reset)     | P1.6         | Output    | Active-low; drive HIGH to enable module       |
| 32kHz SLW_CLK      | P1.7         | Output    | RTC oscillator output, dedicated pin          |
| RGB LED R          | P2.4         | Output    | Active-low open-drain (on-board)              |
| RGB LED G          | P2.5         | Output    | Active-low open-drain (on-board)              |
| RGB LED B          | P2.6         | Output    | Active-low open-drain (on-board)              |

**All P0 and P1 BLE pins operate at VDDIO (1.8V)**, not the 3.3V VDDIOH rail.
The firmware clears the `use_vddioh_0`/`use_vddioh_1` IOMAN bits for these pins.

### UART0 "Mapping B" crossover

The MAX32630FTHR board hardwires the PAN1326B so that P0.0 is the MCU's
**receive** pin and P0.1 is the **transmit** pin. In UART0 Mapping A (the
default), P0.0 would be TX and P0.1 would be RX — the opposite of what the
hardware needs. Selecting Mapping B crossovers TX and RX to match the board.
No software swapping is needed; the hardware does it automatically.

## HCI protocol

The PAN1326B exposes a standard HCI (Host Controller Interface) over UART. The
MCU acts as the HCI host and the CC2564B acts as the controller. HCI packets:

```
Command:  01 <opcode-lo> <opcode-hi> <param-len> [params...]
Event:    04 <event-code> <param-len>  [params...]
```

HCI Reset example:

```
Send:    01 03 0C 00
Receive: 04 0E 04 01 03 0C 00   (Command Complete, status=00 OK)
```

## ⚠ CC256XB service pack requirement

The TI CC2564B chip inside the PAN1326B ships without any firmware for the LE
(Bluetooth Low Energy) subsystem. Before LE commands are accepted, an
initialisation service pack must be loaded over HCI. Without it:

- **HCI Reset works** — the chip is alive and the basic HCI transport is confirmed.
- **LE commands** (`0x20xx` opcode range) return "Unknown HCI Command" (event 0x01).

### How to get the service pack

1. Download **CC256XB-BT-SP** from the TI website (search "CC256XB-BT-SP").
   - [https://www.ti.com/tool/CC256XB-BT-SP#downloads](
     https://www.ti.com/tool/CC256XB-BT-SP#downloads)
2. Inside the archive, find:
   - `initscripts-TIInit_6.7.16_bt_spec_4.1.bts`
   - `initscripts-TIInit_6.7.16_ble_add-on.bts`
3. Convert to a C array using the BTstack tool:
   ```sh
   python btstack/tool/convert_bts_init_scripts.py \
     initscripts-TIInit_6.7.16_bt_spec_4.1.bts \
     initscripts-TIInit_6.7.16_ble_add-on.bts \
     bluetooth_init_cc2564B_1.8_BT_Spec_4.1.c
   ```
4. Place `bluetooth_init_cc2564B_1.8_BT_Spec_4.1.c` in this directory and add
   it to `SRCS` in the Makefile.
5. Uncomment `#define USE_CC256XB_INIT` in `main.c` and call `cc256xb_init()`
   after the HCI Reset succeeds.

### Alternative: use BTstack

[BTstack](https://github.com/bluekitchen/btstack) has a ready-made port for the
MAX32630FTHR (`port/max32630-fthr`). It handles the service pack upload, baud
rate renegotiation (115200 → 4 Mbit/s), and provides a full BLE GATT stack.
For GATT server support (needed for Chrome Web Bluetooth), BTstack is the
recommended long-term path.

## Chrome Web Bluetooth

Yes — once the device is advertising with BLE General Discoverable flags and
exposes GATT characteristics, a Chrome (or Edge) tab can connect via the
[Web Bluetooth API](https://developer.chrome.com/docs/capabilities/bluetooth):

```js
const device = await navigator.bluetooth.requestDevice({
  filters: [{ name: 'SentinelBox' }],
  optionalServices: ['your-service-uuid']
});
const server = await device.gatt.connect();
```

A Chrome web app can then read/write GATT characteristics for configuration
(unlock timeout, authorised fingerprints, etc.) without a native phone app.
This requires the CC256XB service pack + GATT server (BTstack recommended).

## Build and upload

```sh
mise run build:in_bluetooth_LPSDK
mise run upload:in_bluetooth_LPSDK
# or clean → build → upload in one step:
mise run cbu:in_bluetooth_LPSDK
```
