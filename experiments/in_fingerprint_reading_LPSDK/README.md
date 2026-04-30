# in_fingerprint_reading_LPSDK

Rotary encoder controls a hex counter (0–F) shown on a MAX7219 8×8 LED matrix.
Fingerprint sensor code is present but set aside — see the `#if 0` block in `main.c`.

## Hardware wiring

All three encoder pins are on the same P3 external header as the MAX7219 — no
extra connectors needed. P0.x pins are not accessible (used internally for
Bluetooth and microSD).

P3.0/P3.1/P3.2 are reserved for the fingerprint UART. P3.3–P3.5 are MAX7219.
P3.6/P3.7 are free for the encoder quadrature signals. The encoder shaft button
moves to P4.0 (AIN0 analog pin — works as GPIO input).

```
P3 header (external)
──────────────────────────────────────────────────────
P3.0  ──── Fingerprint RX  (UART2 Map A, reserved)
P3.1  ──── Fingerprint TX  (UART2 Map A, reserved)
P3.2  ──── Fingerprint     (reserved)
P3.3  ──── MAX7219 DIN
P3.4  ──── MAX7219 CLK
P3.5  ──── MAX7219 CS
P5.3  ──── Encoder CLK     (quadrature A)
P5.4  ──── Encoder DT      (quadrature B)

P4 header (analog/GPIO)
──────────────────────────────────────────────────────
P5.5  ──── Encoder SW      (shaft button, active-low)

MAX32630FTHR          Rotary Encoder (KY-040 / compatible)
─────────────         ──────────────────────────────────────
3.3V     ────────────  +  (VCC)
GND      ────────────  GND
P3.6     ────────────  CLK  (quadrature A)
P3.7     ────────────  DT   (quadrature B)
P5.5     ────────────  SW   (shaft push-button, active-low)

MAX32630FTHR          MAX7219 8×8 LED matrix
─────────────         ───────────────────────
3.3V     ────────────  VCC
GND      ────────────  GND
P3.3     ────────────  DIN  (MOSI)
P3.4     ────────────  CLK
P3.5     ────────────  CS   (active-low, idle HIGH)
```

### Pin summary

| Signal         | MAX32630FTHR | Direction | Notes |
|----------------|-------------|-----------|-------|
| Encoder CLK    | P5.3        | Input ↑   | pull-up, quadrature A |
| Encoder DT     | P5.4        | Input ↑   | pull-up, quadrature B |
| Encoder SW     | P5.5        | Input ↑   | pull-up, active-low push (AIN0) |
| MAX7219 DIN    | P3.3        | Output    | bit-bang SPI MOSI |
| MAX7219 CLK    | P3.4        | Output    | bit-bang SPI clock |
| MAX7219 CS     | P3.5        | Output    | active-low chip select |
| RGB LED R      | P2.4        | Output    | active-low open-drain |
| RGB LED G      | P2.5        | Output    | active-low open-drain |
| RGB LED B      | P2.6        | Output    | active-low open-drain |

## Behaviour

| Action | Result |
|--------|--------|
| Power on | Display shows **0**, blue LED on |
| Turn CW | Value increments 0 → 1 → … → F → 0 (wraps) |
| Turn CCW | Value decrements 0 → F → … → 1 → 0 (wraps) |
| Press shaft | Resets to **0** |

If rotation direction is reversed, swap the CLK and DT wires (or swap `enc_clk`
and `enc_dt` pin definitions in `main.c`).

## Fingerprint sensor (set aside)

The fingerprint code lives in a `#if 0` block in `main.c` and can be re-enabled
once the pin conflict is resolved:

- UART2 Map A needs **P3.0** (RX) and **P3.1** (TX).
- **P3.3 is shared** between MAX7219 DIN and any alternative UART pin mapping —
  both cannot be active at the same time.
- Resolution options: move MAX7219 DIN to P3.2 and shift encoder SW to another
  free pin, or dedicate P3.0/P3.1 to UART2 and reroute encoder to P4.x.

## Build and upload

```sh
mise run build:in_fingerprint_reading_LPSDK
mise run upload:in_fingerprint_reading_LPSDK
# or combined clean→build→upload:
mise run cbu:in_fingerprint_reading_LPSDK
```
