---
layout: post
title:  "Sentinel Box — Consensus-Based Lock Box: Final Project Summary"
date:   2026-05-24 23:59:59 +1000
categories: sentinel-box update
---

## TL;DR

A lock box that opens only when **multiple parties agree** — whether guarding
digitally addictive devices or just the good chocolate, an unlock requires
consensus from enough family members via a biometric fingerprint reader. The end
result is unfortunately nothing more than a stepper motor controlled via a
Bluetooth console in a browser, oh and a big arse round box.

---

## Introduction & Objective

Digital addiction is real — family members cannot go a few minutes before pulling
out their smart device and checking on their social status or just trying to
prove a point with Wikipedia or AI. We need to learn to STOP 🛑. But it ain't
just digital: sugar, sweets and chocolate can unfairly be disposed of by
teenagers around the house. Standard lock boxes are too small and often go to
the other extreme of having no override at all.

The idea of Sentinel Box was to have consensus in the family to open the box.
Be it 2 parents, a parent and a kid, or all the kids conspiring together.
**Consensus authorisation** — the same idea behind nuclear launch codes or
dual-control bank vaults — shrunk into a handy box that requires **deliberate,
multi-party action** authenticated via a biometric fingerprint reader.

I did not make the cut in my first ever element14 [Smart Security and
Surveillance Design Challenge][smart-security-challenge] to be selected to
receive the hardware for the challenge, but given there was a control board
involved — the **MAX32630FTHR**, a compact ARM Cortex-M4F with a TI CC2564B
Bluetooth module — I thought I would buy one, strap myself in and take part
anyway.

What started as a brainstorm of [18 possible unlock
mechanisms](#unlock-mechanisms) — from wake-word recognition to geofenced GPS
checks to two-phone proximity — was focused down to what could actually be
shipped: a fingerprint reader for biometrics, a stepper-motor vault bolt, and
BLE for remote configuration and override. (The astronomical time lock — where
the unlock window is calculated from that day's sunset — remains on the ideas
list. It is genuinely achievable. It just did not make the deadline.)

![](/sentinel-box/assets/20260421_01_timer_lock_box.jpg)
> *The initial concept — a simple timer lock box that inspired the final design.
> The perspex structure and stepper-based bolt mechanism evolved from here.*

---

## System Architecture

```
┌──────────────────────────────────────────────────────────┐
│                    MAX32630FTHR (96 MHz ARM M4F)         │
│                                                          │
│  UART0 (P0.0/P0.1) ──── PAN1326B ─────── BLE (CC2564B)   │
│  UART2             ──── AS608/R307 ────── Fingerprint    │
│  GPIO (P2.4/5/6)   ──── RGB LED                          │
│  GPIO (P4-5)       ──── ULN2003 ──────── 28BYJ-48 Stepper│
└──────────────────────────────────────────────────────────┘
         │
         │ BLE GATT (Web Bluetooth API)
         ▼
┌────────────────────────────────────┐
│  Chrome / sentinel-console         │
│  Vite + React + Lit web components │
│  Connect · Setup · Debug tabs      │
└────────────────────────────────────┘
```

**Key hardware choices:**

| Component                         | Role            | Why                                                     |
|-----------------------------------|-----------------|---------------------------------------------------------|
| MAX32630FTHR                      | MCU + brain     | Challenge board. 512 KB RAM, 2 MB flash, onboard BLE    |
| PAN1326B / CC2564B                | BLE radio       | Soldered onto the FTHR board — no extra wiring          |
| AS608 / R307 optical fingerprint  | Biometric auth  | UART interface with onboard template matching (5 slots) |
| 28BYJ-48 + ULN2003                | Vault mechanism | Cheap, geared, precise — 4096 half-steps per revolution |
| Perspex sheet                     | Enclosure       | Cut, filed and polished by hand                         |

The software stack is equally lean: bare-metal C on LPSDK, with BTstack as the
BLE layer. I did look at a hermetic build using Rust from scratch but got stuck
on the complexities of the power-on procedure of the **MAX32630FTHR**.

---

## Development Process

The project evolved through five published posts and a fair amount of dead ends.

### [Part I — The Plan][sentinel-box-part-I-the-plan]

The challenge specification landed and I dug through my junk box to find some
related items: RFID readers, cameras, and mmWave proximity detectors.

![](/sentinel-box/assets/20260421_02_parts_list.jpg)

Immediately the scope wanted to be enormous. Eighteen unlock mechanisms were
sketched out — everything from morse-code tap patterns on a piezo to two-phone
proximity checks.

### The Rust Detour

Before committing to C, the project spent time exploring Rust on the MAX32630. A
working 6-axis accelerometer attitude meter was built — the onboard BMI160
driving an external MAX7219 LED matrix to display a tilting artificial horizon.

<video width="740" controls>
  <source src="/sentinel-box/assets/20260423_01_attitude_meter.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>

> *The attitude meter: tilt left, the bar tilts left. Tilt forward and the bar
> scrolls. Built entirely in Rust with raw register access.*

This was real progress — cold-boot issues were traced and fixed (the 96 MHz ring
oscillator needs calibration constants copied from the INFO block on every POR;
the MAX7219 needs 100 ms after LDO3 comes up). But the real obstacle was the
peripherals. No Rust HAL exists for the MAX32630, which means UART, I2C, and SPI
would all require writing drivers from register access up. Fingerprint reader,
BLE module — everything needed for the actual lock box — would each be weeks of
work before the application code could start.

C and LPSDK it was.

### [Part II — Back to C][sentinel-box-part-II-back-to-C]

Switching to LPSDK unlocked the fingerprint reader immediately. The AS608
optical sensor connects via UART2 and handles template capture, merging, and
matching entirely onboard. A rotary encoder selects fingerprint slots (0–F
displayed on a debug terminal), a button triggers enrol or wipe, and the state
machine covers five states: Startup, Idle (scan mode), Enrol, SensorError, and
SlotsFull.

![](/sentinel-box/assets/20260430_02_fingerprint_demo_short.gif)

> *Finger placed on sensor → Letter to match the finger and a red ❌ if the
> finger print has no match*

### [Part III — Stepper Motor and Vault Lock Mechanism][sentinel-box-part-III-stepper-motor-and-vault-lock-mechanism]

The 28BYJ-48 geared stepper motor — the cheap five-wire motor that comes in
every Arduino starter kit — turns out to be well-suited for a vault mechanism.
4096 half-steps per revolution, a gear ratio of roughly 64:1, and the ULN2003
Darlington array driver keeps the MAX32630's GPIO pins well clear of the motor's
current draw.

The vault door design took several iterations. A central rotating cam pushes
four separate bolts outward when locked, and the stepper drives the cam.

<img src="/sentinel-box/assets/20260511_lock_unlock_vault_door.gif"
  alt="Lock and Unlock vault door" width="740" />

> *The vault mechanism design: a stepper drives a central disc which in turn
> drives 5 cams, the bolts extend and retract. Lock and unlock in one motion.*

<img src="/sentinel-box/assets/20260512_perspex_model.gif"
  alt="perspex model going through motion" width="740" />

> *More of a reality of a single cam drive, and no idea how to attach the
> stepper motor.*

The perspex construction revealed its own learning curve and difficulties.
Cutting in a straight line is not that easy and then it takes a bit of work to
file and sand pieces to spec.

### [Part IV — Bluetooth and BTstack][sentinel-box-part-IV-bluetooth-and-btstack]

Getting BLE working on the **MAX32630FTHR** is more involved than the datasheet
suggests. Just to get a HCI layer bare bones demo to work, required a
proprietary TI vendor-specific **CC256XB service pack** to be uploaded — ~150
TI vendor-specific HCI commands that load the LE firmware subsystem.

Fortunately the forum posts of other element14 design challenge battlers
directed me towards **BTstack** to replace all my hand-rolled HCI with a full
BLE stack — L2CAP, ATT, SM, GATT. The GATT database is compiled from a `.gatt`
file into a C byte array using BTstack's Python tool. The ATT read and write
handlers are straightforward callbacks.

Although I still ran into a number of advertising issues between the device and
a console running in a Chrome browser, I did manage to get full read/write
capabilities at full speed.

![](/sentinel-box/assets/20260514_BLE_demo_simple.gif)

> *The first successful BLE demo: connecting from Chrome, writing `01`/`02`/`03`
> to the LED characteristic, watching the RGB LED change colour remotely.*

---

## The Web Console — `sentinel-console`

Rather than a native phone app, the control interface is a **web app served from
localhost** using the Web Bluetooth API. Chrome and Edge support Web Bluetooth
natively — any nearby BLE peripheral is reachable from a browser tab without
installation.

The stack: **Vite + React + TypeScript** for the shell, **shadcn/ui** components
for the UI, and **Lit** (Google's 5KB web components library) for the two pieces
that need genuine encapsulation — the BLE hardware abstraction layer and the
debug terminal.

```
sentinel-console/
├── src/ble/
│   ├── SentinelBleManager.ts   ← Lit web component, owns Web Bluetooth API
│   ├── useSentinelBle.ts       ← React hook wrapping BLE events as state
│   └── gatt.ts                 ← UUID constants, command byte enums
└── src/components/
    ├── terminal/SentinelTerminal.ts  ← Lit terminal web component
    └── LedOverrideCard.tsx           ← shadcn colour buttons → 0xF002
```

The `<sentinel-ble-manager>` Lit element owns all Web Bluetooth API calls and
fires typed custom events (`sentinel-connected`, `sentinel-state-change`,
`sentinel-gatt-notify`). React hooks listen to those events and expose typed
state to the rest of the tree. The `<sentinel-terminal>` Lit element renders a
colour-coded scrolling log (tx/rx/ok/warn/error) with auto-scroll and a clear
button.

![](/sentinel-box/assets/20260523_Red_Green_Blue_via_bluetooth.gif)

> *Clicking colour buttons in the sentinel-console web app: the physical RGB LED
> on the board switches through red, green, and blue in real time over BLE.*

---

## Final Build

The final `src/main.c` build had a plan to bring everything above together. As
time was running out and hardware is not as simple as software, I found myself
hand filing 100s of teeth in perspex - all I could do was hope the teeth would
mesh with a wheel on the stepper motor. I did get a last day success in that the
gears mostly worked!!! but the reality of how long a build would take started to
kick in.

<video width="740" controls>
  <source src="/sentinel-box/assets/20260524_hand_cut_gear_teeth.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>

In the end, I had to cut it short, I had all the electronics mounted, which I
now wish I had done a month ago, but I only got around to having the stepper
motor going.

![](/sentinel-box/assets/20260524_end_build.jpg)
![](/sentinel-box/assets/20260524_mechanism_01.jpg)
![](/sentinel-box/assets/20260524_mechanism_02.jpg)

All in all I had a great time, even if all I got was a Bluetooth, browser
controlled stepper motor controlling a cam.

<video width="740" controls>
  <source src="/sentinel-box/assets/20260524_project_overview_bluetooth_stepper_vault_cam.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>

---

## Lessons Learned

**The Rust exploration was worth it.** Writing bare-metal Rust on hardware with
no HAL forces a deep understanding of the peripherals — the POR vs SWD reset
distinction, the factory calibration constants in the INFO block, the power
sequencer registers. Although the knowledge was deeper than needed for my
switch to C, it did give me a bigger appreciation for development boards and all
that goes into their design, as well as the benefit of reading all the manuals
and specifications.

**BTstack is excellent.** I have never successfully worked with Bluetooth and
although I had some hiccups, it works!

**Web Bluetooth is genuinely useful for hardware debugging.** The
`ble_debug.html` page served from localhost was the fastest way to verify BLE
behaviour — no app install, no pairing ceremony, just a browser tab that reads
and writes characteristics directly. Chrome's
`chrome://bluetooth-internals/#devices` panel shows connected device GATT tables
in real time.

**Hardware takes time.** Although I was lucky to get a bunch of pieces working
throughout the competition, and I had deferred a lot of things to software,
there was still a need for hardware and that takes a lot of meticulous
preparation and time.

---

## Future Completion

- I have committed to a big arse box so I need to do something with it.
- I am pretty sure the perspex I went for is a little too thin for the vault
  door mechanism but I do hope to build that out.
- I am not sure I will be going back to the **MAX32630FTHR** anytime soon,
  opting for a Nano, ESP32 or STM32 instead, but the learnings have been
  invaluable

---

## Source & Links

| Resource                          | Link                                                                            |
|-----------------------------------|---------------------------------------------------------------------------------|
| GitHub                            | [saramic/sentinel-box][github-sentinel-box]                                     |
| Design challenge                  | [Smart Security and Surveillance][smart-security-challenge]                     |
| Project page                      | [Sentinel Box — Consensus-based Lock Box][sentinel-box-project]                 |
| Part I — The Plan                 | [element14 forum][sentinel-box-part-I-the-plan]                                 |
| Part II — Back to C               | [element14 forum][sentinel-box-part-II-back-to-C]                               |
| Part III — Stepper Motor          | [element14 forum][sentinel-box-part-III-stepper-motor-and-vault-lock-mechanism] |
| Part IV — Bluetooth and BTstack   | [element14 forum][sentinel-box-part-IV-bluetooth-and-btstack]                   |
| Part V — Concept vs Perfec Build  | [element14 forum][sentinel-box-part-V-concept-vs-perfect-build]                 |

---

[smart-security-challenge]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/
[sentinel-box-part-I-the-plan]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56869/sentinel-box---part-i---the-plan
[sentinel-box-part-II-back-to-C]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56894/sentinel-box---part-ii---back-to-c
[sentinel-box-part-III-stepper-motor-and-vault-lock-mechanism]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56937/sentinel-box---part-iii---stepper-motor-and-vault-lock-mechanism
[sentinel-box-part-IV-bluetooth-and-btstack]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56942/sentinel-box---part-iv---bluetooth-and-btstack
[sentinel-box-part-V-concept-vs-perfect-build]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56979/sentinel-box---part-v---concept-vs-perfect-build
[sentinel-box-project]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/b/projects/posts/sentinel-box---consensus-based-lock-box
[github-sentinel-box]: https://github.com/saramic/sentinel-box
