# in_stepper_motors

Drives a **ULN2003 28BYJ-48 4-phase stepper motor** from the MAX32630FTHR using
the Maxim LPSDK. On power-up the output shaft rotates 360° forward then 360°
reverse, repeated 3 times, then stops (red LED). Reset the board to run again.

Half-step mode is used for smoother rotation (8 microstates per electrical cycle).

## Hardware

### Components

| Item                  | Notes                                                 |
|-----------------------|-------------------------------------------------------|
| MAX32630FTHR          | 3.3 V supply rail                                     |
| ULN2003 driver board  | 5-pin connector to motor, 4 IN pins, 5 V power header |
| 28BYJ-48              | 4-phase unipolar stepper, 5 V, ~240 mA at full load   |
| 5 V supply            | USB VBUS or dedicated rail — see power notes below    |

### Pin mapping

Pins are chosen to leave all existing `in_fingerprint_reading_LPSDK` wiring
untouched: P3.0–P3.5 (fingerprint UART + MAX7219) and P5.3–P5.5 (rotary encoder)
are all avoided. P5.0–P5.2 are in-line on the analog header; P4.0 (AIN0, confirmed
GPIO-capable) is used for IN4. Note: P6.0 looks tempting but PORT_6 does not exist
in the MAX32630 LPSDK — GPIO_Config silently does nothing, breaking the step sequence.

```
MAX32630FTHR          ULN2003 driver board
─────────────         ────────────────────
P5.2     ────────────  IN1
P5.1     ────────────  IN2
P5.0     ────────────  IN3
P4.0     ────────────  IN4   (AIN0 pin, used as GPIO)
GND      ────────────  GND  (signal ground, common with motor supply)
```

Power the motor board separately:

```
5 V rail ────────────  VCC (+) on ULN2003 board
GND      ────────────  GND (–) on ULN2003 board
```

The 28BYJ-48 5-wire connector plugs directly into the matching socket on the
driver board.

### Pin summary

| Signal       | MAX32630FTHR | Direction | Notes                                  |
|--------------|--------------|-----------|----------------------------------------|
| Stepper IN1  | P5.2         | Output    | in-line, before encoder pins P5.3–P5.5 |
| Stepper IN2  | P5.1         | Output    | in-line, before encoder pins P5.3–P5.5 |
| Stepper IN3  | P5.0         | Output    | in-line, before encoder pins P5.3–P5.5 |
| Stepper IN4  | P4.0         | Output    | AIN0 analog pin used as GPIO           |
| RGB LED R    | P2.4         | Output    | active-low open-drain (on-board)       |
| RGB LED G    | P2.5         | Output    | active-low open-drain (on-board)       |
| RGB LED B    | P2.6         | Output    | active-low open-drain (on-board)       |

## Hardware questions

### Do I need level converters between 3.3 V MAX32630FTHR and the 5 V stepper board?

**No.** The ULN2003 input stage is TTL-compatible:
- VIH(min) = 2.4 V — the MAX32630FTHR GPIO outputs 3.3 V, comfortably above threshold.
- Each IN pin has a 2.7 kΩ internal base resistor. At 3.3 V the GPIO sources
  ≈ (3.3 − 0.7) / 2700 ≈ 1 mA, well within the GPIO drive limit and enough to
  fully saturate the Darlington pair.

The ULN2003 then switches the 5 V motor coil current entirely by itself. The
two voltage domains share only a common GND.

### Will I get enough current from the MAX32630FTHR GPIOs?

**Yes.** The GPIOs only need to drive the ULN2003 control inputs (~1 mA each,
~4 mA total with all four active). The heavy motor current (up to ~240 mA at 5 V)
flows entirely through the ULN2003 from the 5 V supply rail — the MCU never sees it.

### Other things to be aware of

**Power supply**
The motor's 5 V must come from a capable rail. A USB host port can usually supply
500 mA; the MAX32630FTHR USB connector gives access to VBUS (5 V). Avoid powering
the motor coils from the FTHR's on-board 3.3 V LDO — it is not rated for motor loads.
Connect the motor GND and the FTHR GND together.

**Built-in flyback protection**
The ULN2003 driver board includes flyback diodes across each coil driver, so you
do not need external snubbers or diodes.

**Hold position between moves (important for direction reversal)**
The firmware keeps the last coil state energised after each move. This holds the
gear train meshed so that reversing direction immediately has torque rather than
fighting the gearbox backlash from a floating position. De-energising between
moves causes the gears to float; the reverse sequence then resonates against the
backlash and the motor vibrates without moving. Call `stepper_off()` explicitly
if you want to save power — but only do so well before the next move starts.
The motor body will run warm when held; this is normal for this motor class.

**Step mode and speed**
Two modes are available — switch by editing the two `#define` lines near the top
of `main.c`:

| Mode              | `#define`           | Steps/rev | Delay  | Speed   |           |
|-------------------|---------------------|-----------|--------|---------|-----------|
| Half-step         | `STEPPER_HALF_STEP` | 4096      | 1.5 ms | ~10 RPM | ← default |
| Full-step (wave)  | `STEPPER_FULL_STEP` | 2048      | 1.5 ms | ~20 RPM |           |

`STEP_DELAY_US` can also be tuned independently of the mode. Below ~1000 µs
the 28BYJ-48 gearbox stalls. Above ~10 ms is just slow.

**Accuracy and backlash**
The 28BYJ-48 gearbox has measurable backlash. 1024 half-steps targets 90° but
the actual shaft position will be within ±2–4°. If precision matters you need an
encoder on the output shaft, which is unusual for this motor class.

**Direction**
`+STEPS_PER_REV` in `main()` drives the forward coil sequence. Which way the
shaft physically turns depends on how the motor is mounted. Change the sign or
swap any two IN wires to reverse it.

**Gear ratio**
The 28BYJ-48 datasheet quotes 1:64 but the actual ratio is closer to 1:63.68.
`STEPS_PER_REV = 4096` (64 × 64 half-steps) is the common convention. For very
accurate positioning you can calibrate with `STEPS_PER_REV = 4076`.

## Behaviour

| LED colour | Meaning                                              |
|------------|------------------------------------------------------|
| Green      | Rotating 360° forward                                |
| Blue       | Rotating 360° reverse                                |
| Red        | Done (3 cycles complete) — reset board to run again  |

## Build and upload

```sh
mise run build:in_stepper_motors_LPSDK
mise run upload:in_stepper_motors_LPSDK
# or combined clean→build→upload:
mise run cbu:in_stepper_motors_LPSDK
```
