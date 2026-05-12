---
layout: post
title:  "Sentinel Box - Part III - Stepper Motor and Vault lock mechanism"
date:   2026-05-12 10:00:00 +1000
categories: sentinel-box update
---

Time is running out and I need a locking 🔐 mechanism ⚙️ for my smart lock
box.

## Recap

The idea is to build a smart lock box for digital devices to help control
digital addiction, more on the idea can be found in `part I` and a switch back
to coding with LPSDK and accessing a fingerprint scanner in `part II`

- [Sentinel Box - Part I - the plan](sentinel-box-part-I-the-plan)
- [Sentinel Box - Part II - back to C](sentinel-box-part-II-back-to-C)

## Acrylic as a build medium

I have limited resources on the mechanical side, some hand tools and no 3D
printer yet. I do have a mechanical engineering degree so maybe I do know what
is possible but hand cutting intricate parts is hard and a limitation. My most
recent build medium choice has been 3mm acrylic sheet, it is relatively easy to
cut and with a heat gun I can also shape it. Finally it gives things a very
"skeletonized" see-through tech aesthetic.

As the core of my device is a "lock box" I looked around for some inspiration
and found some vault door designs out there. I decided to come up with my own.
First I drew a couple of ideas, then I extended it to what a full 5 pin vault
door would look like with a simple HTML animation (I am sure there are some
good engineering tools for this, keen to hear people's thoughts) and finally to
make sure I get my head around any tolerances and interferences I built a
prototype of one arm.

<img src="/sentinel-box/assets/20260511_lock_unlock_vault_door.gif"
  alt="Lock and Unlock vault door" width="740" />

Above are 5 "T-shaped" pins running through the motion of lock and unlock
driven by a central round piece.

<img src="/sentinel-box/assets/20260512_perspex_model.gif"
  alt="perspex model going through motion" width="740" />

<video width="740" controls>
  <source src="/sentinel-box/assets/20260512_perspex_model.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>

A perspex model to iron out tolerances and my construction limitations.

## Stepper motor

I presume there are a number of ways I could power my vault door, but as I have
never actually used a stepper motor, nothing like now being the right time. I
got some `ULN2003` based **28BYJ-48** 4-Phase Stepper Motor running at 5V. The
`ULN2003` input threshold is 2.4 V so the `MAX32630FTHR` GPIO outputs 3.3 V
should be fine. As each input draws ~1 mA through the built-in 2.7 kΩ base
resistor — no translator required.

The other good thing is, the stepper is geared down, which means it will
prevent being "forced open" ie the gears are 1 way and the vault door will stay
shut unless the decision is made by the device to power the stepper. This is
the case if I used a motor with a worm drive but not the case with most servos
and they can be pushed into position.

As I am deep in C-land with LPSDK, I don't get the benefit of just including a
library like

```c
//Includes the Arduino Stepper Library
#include <Stepper.h>

// Defines the number of steps per rotation
const int stepsPerRevolution = 2038;

// Creates an instance of stepper class
// Pins entered in sequence IN1-IN3-IN2-IN4 for proper step sequence
Stepper myStepper = Stepper(stepsPerRevolution, 8, 10, 9, 11);
```

Fortunately the library is open source so looking at the code
- [https://github.com/arduino-libraries/Stepper/blob/master/src/Stepper.h](
  https://github.com/arduino-libraries/Stepper/blob/master/src/Stepper.h)

shows me the details of controlling the various coils in the stepper motor.
This is good as initially my guesses were wrong and I had it erratically turning
in 1 direction and just buzzing on the spot when directed in reverse.
Fortunately no smoke left the devices.

Finally I got the motor working once I worked out the delay to take between
steps, 3 ms, and the correct coils to power at the correct time.

```c
/* -------------------------------------------------------------------------
 * 28BYJ-48 stepper via ULN2003 driver board.
 *
 * Gear ratio: 32 internal steps × 63.68 ≈ 2037.9 → use 2038 full / 4076 half.
 * Reverse (-steps): table index retreats instead of advances.
 * Coils stay energised after a move — holds gearbox meshed for clean reversal.
 * ------------------------------------------------------------------------- */

static const uint8_t wave_step[4][4] = {
    { 1, 0, 0, 0 },   /* IN1        */
    { 0, 1, 0, 0 },   /* IN2        */
    { 0, 0, 1, 0 },   /* IN3        */
    { 0, 0, 0, 1 },   /* IN4        */
};

static const uint8_t half_step[8][4] = {
    { 1, 0, 0, 0 },   /* IN1        */
    { 1, 1, 0, 0 },   /* IN1+IN2    */
    { 0, 1, 0, 0 },   /* IN2        */
    { 0, 1, 1, 0 },   /* IN2+IN3    */
    { 0, 0, 1, 0 },   /* IN3        */
    { 0, 0, 1, 1 },   /* IN3+IN4    */
    { 0, 0, 0, 1 },   /* IN4        */
    { 1, 0, 0, 1 },   /* IN4+IN1    */
};

static const uint8_t (*active_table)[4] = half_step;
static int      table_len     = 8;
static uint32_t step_delay_us  = 2944;   /* 5 RPM – 60,000,000 / 4076 / 5 */
static int      steps_per_rev  = 4076;   /* 32 × 63.68 × 2 */
```

also some fine tuning on the delay settings based on the [Stepper.cpp](
https://github.com/arduino-libraries/Stepper/blob/master/src/Stepper.cpp) file
- Half-step: `2,944 µs` → 5 RPM (60,000,000 / 4076 / 5)
- Wave drive: `1,963 µs` → 15 RPM (60,000,000 / 2038 / 15)

and success

<img src="/sentinel-box/assets/20260512_stepper_test.gif"
  alt="Stepper motor test" width="740" />

<video width="740" controls>
  <source src="/sentinel-box/assets/20260512_stepper_test.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>

## Next

I am starting to bring a bunch of things together, the MAX32630FTHR connecting
to a fingerprint reader, powering a stepper motor, being configured with a
rotary encoder and displaying things on an LED matrix. Still I am not sure if
that will be enough to enable a lock box that needs more than 1 person to
verify for it to be unlocked. For that I am hoping to investigate BLE
(Bluetooth Low Energy) and maybe a phone app to help configure it, that said
the phone is likely to be locked into the lock box, so maybe only for
configuring the lock box. Then there is connecting the stepper motor to the
vault mechanism and only a few weeks left in the competition — 24th May 2026.

## Source

[https://github.com/saramic/sentinel-box][github-sentinel-box]

[smart-security-challenge]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/
[sentinel-box-part-I-the-plan]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56869/sentinel-box---part-i---the-plan
[sentinel-box-part-II-back-to-C]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56894/sentinel-box---part-ii---back-to-c
[github-sentinel-box]: https://github.com/saramic/sentinel-box
