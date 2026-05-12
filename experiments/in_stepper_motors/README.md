# in_stepper_motors

Drives a **ULN2003 28BYJ-48 4-phase stepper motor** from the MAX32630FTHR using
the Maxim LPSDK. On power-up:

1. **Half-step** (4076 steps/rev, 5 RPM) — 360° forward then 360° reverse
2. **Wave drive** (2038 steps/rev, 15 RPM) — 360° forward then 360° reverse

Then stops (red LED). Reset the board to repeat.

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

**Coil layout and step sequences**

The 28BYJ-48 ULN2003 pins IN1–IN4 map to physically adjacent coils in order
around the stator. The Arduino Stepper library confirms this — it passes the
pins as `(IN1, IN3, IN2, IN4)` into a generic bipolar step table, which after
remapping resolves to adjacent pairs IN1+IN2, IN2+IN3, IN3+IN4, IN4+IN1.

```
              IN1  (0°)
               |
  IN4 ---------+--------- IN2  (90° CW)
  (270°)       |          
              IN3  (180°)

Clockwise:         IN1 → IN2 → IN3 → IN4 → IN1 ...
Counter-clockwise: IN1 → IN4 → IN3 → IN2 → IN1 ...
```

**Wave drive** (single-coil, 2038 steps/rev, 15 RPM at 1,963 µs/step — 3× faster than half-step; 20 RPM stalls):

| Step | IN1 | IN2 | IN3 | IN4 | Active |
|-----:|----:|----:|----:|----:|-------:|
| 0    | H   | L   | L   | L   | IN1    |
| 1    | L   | H   | L   | L   | IN2    |
| 2    | L   | L   | H   | L   | IN3    |
| 3    | L   | L   | L   | H   | IN4    |

**Half-step** (4076 steps/rev, 5 RPM at 2,944 µs/step):

| Step | IN1 | IN2 | IN3 | IN4 | Active   |
|-----:|----:|----:|----:|----:|---------:|
| 0    | H   | L   | L   | L   | IN1      |
| 1    | H   | H   | L   | L   | IN1+IN2  |
| 2    | L   | H   | L   | L   | IN2      |
| 3    | L   | H   | H   | L   | IN2+IN3  |
| 4    | L   | L   | H   | L   | IN3      |
| 5    | L   | L   | H   | H   | IN3+IN4  |
| 6    | L   | L   | L   | H   | IN4      |
| 7    | H   | L   | L   | H   | IN4+IN1  |

Steps/rev uses the accurate gear ratio 32 × 63.68 ≈ 2038 (wave) / 4076
(half-step), matching the Arduino `stepsPerRevolution = 2038` convention.
`step_delay_us` can be tuned in `main.c`. Below ~1000 µs the gearbox stalls.

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
The accurate steps/rev is 4076 (half-step) / 2038 (wave drive), based on the
real gear ratio 32 × 63.68. The Arduino library uses `stepsPerRevolution = 2038`
for the same reason.

## Behaviour

| LED colour | Meaning                                                    |
|------------|------------------------------------------------------------|
| Green      | Rotating 360° forward (either mode)                        |
| Blue       | Rotating 360° reverse (either mode)                        |
| Red        | Done (both modes complete) — reset board to run again      |

## Build and upload

```sh
mise run build:in_stepper_motors_LPSDK
mise run upload:in_stepper_motors_LPSDK
# or combined clean→build→upload:
mise run cbu:in_stepper_motors_LPSDK
```
