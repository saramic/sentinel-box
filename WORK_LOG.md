# Work Log

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
