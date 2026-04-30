---
layout: post
title:  "Sentinel Box - Part II - back to C"
date:   2026-04-30 22:00:00 +1000
categories: sentinel-box update
---

I had fun with my **Rust 🦀** experiment but it was time to get back to **C 🅲**
and **LPSDK** (Low Power ARM Micro SDK) if I was going to get anything complete
for this challenge.

## Recap

The idea is to build a smart lock box for digital devices to help control
digital addiction, more on the idea can be found in `part I`

- [Sentinel Box - Part I - the plan](sentinel-box-part-I-the-plan)

## Rust for Hermetic builds

As I was not super happy in having to depend on EOL (End-of-life) software
stacks like Mbed or LPSDK, I thought, how hard can it be writing a HAL
(Hardware Abstraction Layer) from scratch in Rust. I had some initial wins
getting the LED flashing, so I continued. In **Part I** I even started
implementing some commands against the **MAX14690 PMIC** (Power Management
Integrated Circuit) - I was actually going above and beyond and looking at the
schematic of the dev board to see how it hangs together. This should have
**turned me OFF** the idea of rust, but it gave me some false hope and I
continued.

![part of the MAX32630FTHR diagram highlighting the BMi160 accelerometer chip](
/sentinel-box/assets/20260430_01_max32630_BMi160_accelerometer.png)

The **BMi160** accelerometer chip highlighted on the block diagram of the
**MAX32630FTHR**

Next on the diagram there was an Accelerometer and Gyroscope using the
**BMi160** 6-Axis Inertial Motion Sensor. With a bit of AI I soon enough had a
driver to get X, Y, Z accelerations and using the **MAX7219** I built a simple
"attitude meter" which would show you which way to move the board to correct it
being right way round. At this point I started to notice that some of the
timing was out, my LED refresh rate was out by anywhere from 4x to 20x - that
should have **turned me OFF** the idea of rust but with false hopes I
continued.

<video width="740" controls>
  <source src="/sentinel-box/assets/20260423_01_attitude_meter.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>

The video above shows the attitude meter 🛩️ 🧭 that displays on the LED matrix
the direction to turn the device to flatten the chip.

These were all distractions from the core of the Smart Security Challenge, so
it was time to use a finger print reader. This would need **UART** (Universal
Asynchronous Receiver-Transmitter) surely that cannot be hard? Well this is
where the speed changes of 4X – 20X really started to bite. You see if you
don't know how fast your clock speed is going, you are not going to be able to
transmit or receive at a given Baud. I tried a bunch of things. As I don't have
a logic analyzer nor oscilloscope on hand, I ended up hooking up an **ESP32**
to measure the duration of pulses. I didn't know there was a function to
measure pulse length
[`pulseIn`](https://docs.arduino.cc/language-reference/en/functions/advanced-io/pulseIn/)

```c
unsigned long width = pulseIn(RX2_PIN, LOW, 5000000UL); // 5 s timeout
if (width == 0) {
  Serial.println("timeout — no signal on RX pin");
  return;
}
// Filter noise: allow 1200–115200 baud = 8–833 µs.
if (width < 8 || width > 833) {
  Serial.printf("noise/glitch: %lu µs (ignored)\n", width);
  return;
}
unsigned long estimated_baud = 1000000UL / width;
  // Square-wave formula: CPU MHz = N / width_µs, where N is the delay_cycles count.
  // Firmware step 1 uses N=4800. Steps double: 4800, 9600, 19200, 38400, 76800.
  // Read the first group of pulses (smallest width) and use N=4800.
Serial.printf(
  "LOW pulse: %lu µs | sq-wave asm::delay(4800)~9600 cycles: CPU ~%lu MHz\n",
  width,
  9600UL / width); // asm::delay(N) ≈ 2N cycles → CPU MHz = 9600 / width_µs
```

The above code also made me realise that `\n` does not cut it and my serial
monitor only displayed the output when I had `\r\n` - need that Carriage
return.

And all my Rust results were conclusive on 1 part, I was never getting 96 MHz.
After some digging around I found the C file in LPSDK that seems to do the
setup of the frequency

`Maxim/Firmware/MAX3263X/Libraries/CMSIS/Device/Maxim/MAX3263X/Source/system_max3263x.c`

1. **Enable the 32 kHz RTC oscillator** — this serves as the stable reference
   clock for calibration, then wait for it to warm up and settle.
2. **Enable the RO calibration complete interrupt** — so the system can signal
   when calibration is done.
3. **Clear the calibration complete interrupt flag** — removing any stale state
   from a previous run.
4. **Write an initial trim** value into the frequency calibration initial
   condition register — giving the hardware a starting point rather than
   hunting from scratch.
5. **Load that initial trim** into the active frequency trim register — making
   it live.
6. **Enable the frequency control loop** — the hardware mechanism that will
   drive the RO trim toward the target frequency.
7. **Start calibration in atomic mode** — the hardware runs the measurement and
   adjustment cycle uninterrupted.
8. **Wait for the ro_cal_done flag** — polling until the hardware signals
   completion.
9. **Stop the calibration engine.**
10. **Disable the calibration complete interrupt.**
11. **Read back the final trim value** — the digital code the loop converged on.
12. **Write the final trim to the RO flash trim shadow register** — persisting
    the result across resets.
13. **Restore the RTC to its previous state** — since it may have been off
    before the routine borrowed it.
14. **Disable the frequency control loop.**

no can do - I tried and tried again but could not get a consistent 96 MHz set
which meant the UART was not going to work and the rust experiment was over for
the time being

<p style="font-size: 100px; text-align: center;">❌🦀</p>

# Back to LPSDK

I couldn't get myself to go to Mbed, a platform that is slated for EOL in July
2026, so it was back to C and the legacy LPSDK stack.

Hitting up my ESP32 setup, proved I had the right frequency set on UART and in
no time I was connected to the fingerprint reader. The problem was that now my
system would need to be multi modal. I pulled in a rotary encoder and got some
runs on the board. First just to read turns to the left and right as well as a
button press and display it on the LED MAX7219 matrix display. Then I added in
the finger print reader.

<img src="/sentinel-box/assets/20260430_02_fingerprint_demo_short.gif"
  alt="Fingerprint reading success and failure" width="740" />


<video width="740" controls>
  <source src="/sentinel-box/assets/20260430_03_fingerprint_demo.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>

Getting the fingerprint, LED Matrix and rotary encoder was the easy bit, but
having a state diagram that can save a bunch of fingerprints and identify them
was starting to get a bit complicated, as is the setup on my workbench.

<img src="/sentinel-box/assets/20260430_04_encoder_state_machine.png"
  alt="Encoder and fingerprint state machine flow chart" width="740" />

## Next

Now that I have a basic fingerprint reader and a build system that I am
confident in, I think I need to get the required piece for a minimal complete
build, some kind of actuator working: stepper motor, servo motor or just a
motor with a worm drive. This will allow me to create a lock box with multi
finger print triggering. If I get time, I may be able to expand on that. Time
will tell.

## Source

[https://github.com/saramic/sentinel-box][github-sentinel-box]

[smart-security-challenge]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/
[sentinel-box-part-I-the-plan]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56869/sentinel-box---part-i---the-plan
[github-sentinel-box]: https://github.com/saramic/sentinel-box
