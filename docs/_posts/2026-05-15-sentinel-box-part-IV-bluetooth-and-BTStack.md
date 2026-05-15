---
layout: post
title:  "Sentinel Box - Part IV - Bluetooth and BTStack"
date:   2026-05-15 09:00:00 +1000
categories: sentinel-box update
---

As the competition end 🏁 is nearing, I am hoping to connect to some
Bluetooth<svg style="vertical-align: middle; width: 2em; height: 2em; display:
inline;" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24"
id="Bluetooth--Streamline-Svg-Logos" height="24" width="24"> <path
fill="#0060a9" d="m12.950025 6.0523 1.942975 1.94295 -1.941775 1.9414 -0.0012
-3.88435Zm0 11.893825 1.942975 -1.942575 -1.941775 -1.941775 -0.0012
3.88435Zm-2.0745 -5.946925L6.6753 7.78825l1.217425 -1.21705L11.2387
9.91875V1.8965575L17.3239 7.98135 13.306025 11.9992 17.324675 16.01825 11.2391
22.10305V14.08045l-3.34675 3.34795 -1.21745 -1.217825 4.200625
-4.211375ZM12.000775 23.75c5.1312 0 8.6667 -2.43765 8.6667 -11.7504C20.667475
2.68725 17.131975 0.25 12.000775 0.25c-5.130775 0 -8.66825 2.43765 -8.66825
11.7496C3.332525 21.31235 6.8696 23.75 12.000775 23.75Z"
stroke-width="0.25"></path></svg> to help speed up some of the configuration
options of my Lock 🔐 Box 📦 — **Sentinel Box**.

## Recap

The idea is to build a smart lock box for digital devices to help control
digital addiction, more on the idea can be found in **`part I`** and coding
with LPSDK and accessing a fingerprint scanner in **`part II`**, and mechanical
stepper motors and vault lock mechanisms in **`part III`**:

- [Sentinel Box - Part I - the plan][sentinel-box-part-I-the-plan]
- [Sentinel Box - Part II - back to C][sentinel-box-part-II-back-to-C]
- [Sentinel Box - Part III - Stepper Motor and Vault lock mechanism
  ][sentinel-box-part-III-stepper-motor-and-vault-lock-mechanism]

## Getting over my fears

I have a somewhat scarred history in some parts of microprocessor hobbyist
programming. I recall back in the early 2000s making some relatively expensive
development board unresponsive, I think it may have been running some version
of **J2ME** Java **¯\_(ツ)_/¯**, I shoved it under the house and gave up for
some 5 years. I thought with the advent of [**PIC microcontrollers**
][pic-microcontrollers-wikipedia], and when I got around to looking at them
around 2005, I would give it a try, but after a few different approaches I
failed and gave up again. By the time Arduino came around I was a little
hesitant and didn't really think much of it. Finally, at a coding hackathon in
Melbourne Australia, I was lucky enough to get a Particle board — one you had
to program through a web browser and by default it was internet connected by
WiFi 🛜 when the implications of an internet connected device sparked something
in me. Noting that most Arduinos do not support wireless communications, I
quickly switched to ESP32 as my go-to and was relatively happy ever since. This
was until a few years ago when I wanted to connect via Bluetooth — and I
failed, the project went into a box.

So with this background I got myself into this competition, where all of a
sudden I would need a DAP link programmer to connect to a device, and the only
wireless comms that it came with was Bluetooth. Numerous times when my device
was unresponsive, I thought I had fried it somehow and it was time to put it in
a box under the house. Numerous times I would touch and check voltages just in
case something was heating up, or I had mixed up a wire or a voltage. This was
exacerbated by my reliance on AI guiding me in a bunch of ideas like hermetic
builds in a new **Rust 🦀** tool chain only to work out I had not thought about
setting up the **PMIC** (power management) and pins were at random voltages. I
got this far, touch wood, I can finish this off.

When it came to Bluetooth <svg style="vertical-align: middle; width: 1em;
height: 1em; display: inline;" xmlns="http://www.w3.org/2000/svg" fill="none"
viewBox="0 0 24 24" id="Bluetooth--Streamline-Svg-Logos" height="24"
width="24"> <path fill="#0060a9" d="m12.950025 6.0523 1.942975 1.94295
-1.941775 1.9414 -0.0012 -3.88435Zm0 11.893825 1.942975 -1.942575 -1.941775
-1.941775 -0.0012 3.88435Zm-2.0745 -5.946925L6.6753 7.78825l1.217425
-1.21705L11.2387 9.91875V1.8965575L17.3239 7.98135 13.306025 11.9992 17.324675
16.01825 11.2391 22.10305V14.08045l-3.34675 3.34795 -1.21745 -1.217825 4.200625
-4.211375ZM12.000775 23.75c5.1312 0 8.6667 -2.43765 8.6667 -11.7504C20.667475
2.68725 17.131975 0.25 12.000775 0.25c-5.130775 0 -8.66825 2.43765 -8.66825
11.7496C3.332525 21.31235 6.8696 23.75 12.000775 23.75Z"
stroke-width="0.25"></path></svg> and not really understanding the
communications protocol and **GATT** (Generic Attribute Profile) vs **ATT**
(Bluetooth Attribute Protocol) and what is **Bluetooth** vs **BLE** (Bluetooth
Low Energy) and all the write ups on bluetooth that came before me in this
competition:

- [Guardian Sentinel <Part 3> — Ethernet & BLE - meera_hussien
  ][guardian-sentinel-part-3-ethernet-ble]
- [BLE scanning and CC256X firmware uploading - Alistair
  ][ble-scanning-and-cc256x-firmware-uploading]
- [Identity Protocol - Part 9 - BLE GATT Challenge/Response with BTstack - arvindsa
  ][identity-protocol---part-9---ble-gatt-challenge-response-with-btstack]
- [Accessing the PAN1326B Bluetooth module with the Arduino IDE (Don't Forget to Set) - Alistair
  ][accessing-the-pan1326b-bluetooth-module-with-the-arduino-ide-don-t-forget-to-set]
- [Accessing the PAN1326B Bluetooth module on the MAX32630fTHR - Alistair
  ][accessing-the-pan1326b-bluetooth-module-on-the-max32630fthr]
- [Identity Protocol - Part 4 - BLE using PAN1326B and BTstack - arvindsa
  ][identity-protocol---part-4---ble-using-pan1326b-and-btstack]
  - [github:arvindsa ble_beacon test using LPSDK
    ](https://github.com/arvindsa/identity-protocol-e14-challenge/tree/main/firmware/tests/ble-beacon)

With things like:

> CC256X firmware uploading ... returned `0x01` (Unknown **HCI** Command), and
> that shown new firmware needs uploading. Has anyone done this?

and

> GATT Service Definition ... BTstack generates an ATT database at plaintext
> .gatt file. Refer: `tool/compile_gatt.py`
> ...
> If you are new to GATT - I recommend to watch this video ...

and

> The first thing I do is to pull the reset pin low (aka active) white we set
> up some other things. There is an obscure hardware bug in the module design
> that at times prevents the reset from functioning as it should later
> ```c
> #define BT_RST P1_6
> digitalWrite(BT_RST, LOW);
> ```

and

> I figured since **BTStack** was kind enough to share the **LPSDK** I thought
> they would also include the **TI**'s Init Script for the **CC2564B**.

and

> AI <svg height="1em" style="flex:none;line-height:1" viewBox="0 0 24 24" width="1em" xmlns="http://www.w3.org/2000/svg"><title>Claude Code</title><path clip-rule="evenodd" d="M20.998 10.949H24v3.102h-3v3.028h-1.487V20H18v-2.921h-1.487V20H15v-2.921H9V20H7.488v-2.921H6V20H4.487v-2.921H3V14.05H0V10.95h3V5h17.998v5.949zM6 10.949h1.488V8.102H6v2.847zm10.51 0H18V8.102h-1.49v2.847z" fill="#D97757" fill-rule="evenodd"></path></svg>
> no longer having access to reading artilces on Element 14 - seems like that
> block came in recently?

I was scared 😬 — was I going to fry my device? Was this ever going to work?
Was my reliance on AI <svg height="1em" style="flex:none;line-height:1"
viewBox="0 0 24 24" width="1em"
xmlns="http://www.w3.org/2000/svg"><title>Claude Code</title><path
clip-rule="evenodd" d="M20.998
10.949H24v3.102h-3v3.028h-1.487V20H18v-2.921h-1.487V20H15v-2.921H9V20H7.488v-2.921H6V20H4.487v-2.921H3V14.05H0V10.95h3V5h17.998v5.949zM6
10.949h1.488V8.102H6v2.847zm10.51 0H18V8.102h-1.49v2.847z" fill="#D97757"
fill-rule="evenodd"></path></svg> going to just burn tokens and not get me
anywhere?

here goes nothing ...

## Random paths with AI

The Element 14 apparent block of AI <svg height="1em"
style="flex:none;line-height:1" viewBox="0 0 24 24" width="1em"
xmlns="http://www.w3.org/2000/svg"><title>Claude Code</title><path
clip-rule="evenodd" d="M20.998
10.949H24v3.102h-3v3.028h-1.487V20H18v-2.921h-1.487V20H15v-2.921H9V20H7.488v-2.921H6V20H4.487v-2.921H3V14.05H0V10.95h3V5h17.998v5.949zM6
10.949h1.488V8.102H6v2.847zm10.51 0H18V8.102h-1.49v2.847z" fill="#D97757"
fill-rule="evenodd"></path></svg> hit me day one. Given all the great resources
of other people's findings, I thought I would point AI at these articles and
have it give me a strategy to go forward. Not paying too much attention, I just kept
agreeing to the prompt to come up with a plan; only after hours of going around
in circles I worked out it was actually having trouble accessing the
articles and nothing to do with coming up with a plan of how to get things
going. I was triggered when I saw it trying to install and set up **Jina AI
Reader**.

> **Jina AI** is a specialized tool designed to scrape website data and convert
> it into clean, **LLM**-friendly text or Markdown format. It is particularly
> useful for **RAG** (Retrieval-Augmented Generation) systems, AI agents, and
> content extraction by removing HTML clutter.

A bunch of tokens burnt and the long way round to getting Bluetooth working, as we will
see shortly.

### HCI (Host Control Interface)

The **MAX32630FTHR** has a **PAN1326B** Bluetooth module soldered directly on
it — no external module or extra wiring needed. Inside the PAN1326B is a **TI
CC2564B** dual-mode BT/BLE chip. It talks to the **MAX32630** via `UART0`, via
`P0.0` and `P0.1`, simple enough, at least I couldn't get the wiring wrong.

As I had somehow gone down the direct **HCI** (Host Control Interface) method, I
seemed to have to start to do a lot of low-level stuff like setting up the 32kHz
reference clock on `P1.7` needed by the BLE module. So this required starting
the crystal oscillator and enabling it


```c
MXC_RTCCFG->clk_ctrl |= MXC_F_RTC_CLK_CTRL_NANO_EN;
MXC_RTCCFG->osc_ctrl |= MXC_F_RTC_OSC_CTRL_OSC_WARMUP_ENABLE;
MXC_PWRSEQ->reg4     |= MXC_F_PWRSEQ_REG4_PWR_PSEQ_32K_EN;
TMR_Delay(MXC_TMR0, MSEC(50));
```

Skipping the first two lines gives you a valid `GPIO` voltage on `P1.7` — just
no clock signal. The module boots but never initialises its `HCI` `UART`
properly.

Next there was a **CTS** trap. The module relies on hardware flow
control — specifically **RTS** (Ready to Send) and **CTS** (Clear to Send) — to
manage data transmission.

With hardware flow control enabled (`.cts = 1` in `uart_cfg_t`), the UART
peripheral refuses to transmit while the CTS input (P0.2) is HIGH. During boot,
the PAN1326B holds its RTS output HIGH — which feeds directly into P0.2. Result:
every HCI command sits in the transmit buffer and never goes out. The fix is to
**disable hardware CTS checking** and let the MCU transmit freely:

```c
const uart_cfg_t ble_cfg = {
    .parity = UART_PARITY_DISABLE, .size = UART_DATA_SIZE_8_BITS,
    .extra_stop = 0, .cts = 0, .rts = 0, .baud = 115200,
};
```

Finally, to run BLE (Bluetooth Low Energy), the CC256XB requires a service
pack???

The CC2564B chip ships without any LE (Bluetooth Low Energy) subsystem
firmware. After a plain HCI Reset, standard BLE commands like
`HCI_LE_Set_Advertising_Parameters` (opcode `0x2006`) come back as "Unknown HCI
Command". Before BLE works, a **service pack** of ~150 TI vendor-specific HCI
commands must be uploaded.

TI distributes this as `CC256XB-BT-SP`. The Bluetopia variant (`CC256XB.h`)
packages all the commands as two C arrays — `BasePatch[]` and `LowEnergyPatch[]`
— each a flat stream of raw HCI packets. Uploading is straightforward iteration:

```c
while (p + 4 <= end) {
    unsigned int cmd_len = 4 + p[3];
    hci_send(p, cmd_len);
    hci_drain_event(200);
    p += cmd_len;
}
```

The last command in `LowEnergyPatch` is `0xFD5B` (LE enable). It takes
noticeably longer than the others and its Command Complete event can still be
sitting in the UART buffer when you send the follow-up HCI Reset. The symptom:
`hci_reset()` receives `04 0E 04 01 5B FD 00` — a valid response, but for the
wrong command. The fix is to loop in `hci_reset()` and skip any Command Complete
that isn't for the Reset opcode `0x0C03`.

The file is TI proprietary (TSPA licence)

> TI TSPA License
> TECHNOLOGY AND SOFTWARE PUBLICLY AVAILABLE

so it lives in `reference/CC256XB_BT_SP/` which is gitignored.

The license to get it reminds me of distributing encryption back in the 90s

> I certify that the following is true:
> (a) I understand that this Software/Tool/Document is subject to export
> controls under the U.S. Commerce Department’s Export Administration
> Regulations ("EAR").
>
> (b) I am NOT located in Cuba, Iran, North Korea, Sudan or Syria. I understand
> these are prohibited destination countries under the EAR or U.S. sanctions
> regulations.
>
> (c) I am NOT listed on the Commerce Department’s Denied Persons List, the
> Commerce Department's Entity List, the Commerce Department’s General Order
> No. 3 (in Supp. 1 t o EAR Part 736), or the Treasury Department’s Lists of
> Specially Designated Nationals.
>
> (d) I WILL NOT EXPORT, re-EXPORT or TRANSFER this Software/Tool/Document to
> any prohibited destination, entity, or individual without the necessary
> export license(s) or authorization(s) from the U.S. Government.
>
> (e) I will NOT USE or TRANSFER this Software/Tool/Document for use in any
> sensitive NUCLEAR, CHEMICAL or BIOLOGICAL WEAPONS, or MISSILE TECHNOLOGY
> end-uses unless authorized by the U.S. Government by regulation or specific
> license.
> ...

something was working, so now to connect

### Chrome Web Bluetooth scanner

Chrome (and Edge) support the **Web Bluetooth API** — a page served from
`localhost` can scan for and connect to nearby BLE peripherals without any
native app. The device picker shows all advertising BLE devices:

```js
const device = await navigator.bluetooth.requestDevice({
  acceptAllDevices: true,
  optionalServices: ["generic_access"],
});
const server = await device.gatt.connect();
const services = await server.getPrimaryServices();
```

The MAX32630FTHR was advertising and I had connectivity 🎉

but what now? ...

## BTStack is the way

Seems I had gone a roundabout way to try to manually install packages and set
oscillators as well as use HCI, but this was a lot of code and was not going to
scale well to actually having 2-way comms between a configuration browser page
and the BLE device. I re-read some of the above mentioned resources and it
seemed BTStack was the way.

AI still seemed to take me down some custom "polling" method it claimed

> `src/btstack_port.c` uses the same cooperative polling approach — **not**
> interrupt-driven DMA — but with a **"simpler"** single-pass structure: RX
> drain → TX drain → run loop tick.

but I finally corrected it to pretty much use the code from `@arvindsa`

Switching from the raw HCI to BTstack — a proper BLE stack — meant the code was
simpler, faster and more robust.

Following adding BTstack as a git submodule

```sh
git submodule add https://github.com/bluekitchen/btstack.git third_party/btstack
```

I could now compile a human-readable GATT database — `.gatt` → `.h` file into a
C byte array. The service definition is trivial:

```
PRIMARY_SERVICE, 0000F001-0000-1000-8000-00805F9B34FB
    CHARACTERISTIC, 0000F002-..., DYNAMIC | WRITE | WRITE_WITHOUT_RESPONSE,
```

And I could now access this from the browser and by writing data, see changes
on the board.

<img src="/sentinel-box/assets/20260514_BLE_demo_simple.gif"
  alt="BLE control via Chrome browser changes LED colour" width="740" />

shows that changes on the Chrome page change the LED colour: `01` — RED 🔴, `02` —
GREEN 🟢, `03` — BLUE 🔵.

<video width="740" controls>
  <source src="/sentinel-box/assets/20260514_BLE_demo.mp4" type="video/mp4">
  Your browser does not support the video tag.
</video>


## Next

Ok, I have some Bluetooth, a dash of stepper and a finger scan or two. I am away
down the coast at the moment so I cannot attempt the build of the vault
mechanism till I have access to my limited tools. I am feeling bullish and
maybe I should also get access to writing some things to the SD card, but maybe
it is time to start to bring these things together in a final design of a
Bluetooth-controlled setup for capturing fingerprints, locking and unlocking
once the required family members have scanned their fingers.

## Source

[https://github.com/saramic/sentinel-box][github-sentinel-box]

[smart-security-challenge]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/
[sentinel-box-part-I-the-plan]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56869/sentinel-box---part-i---the-plan
[sentinel-box-part-II-back-to-C]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56894/sentinel-box---part-ii---back-to-c
[sentinel-box-part-III-stepper-motor-and-vault-lock-mechanism]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56937/sentinel-box---part-iii---stepper-motor-and-vault-lock-mechanism
[github-sentinel-box]: https://github.com/saramic/sentinel-box
[pic-microcontrollers-wikipedia]: https://en.wikipedia.org/wiki/PIC_microcontrollers

[guardian-sentinel-part-3-ethernet-ble]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56925/guardian-sentinel-part-3-ethernet-ble
[ble-scanning-and-cc256x-firmware-uploading]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56886/ble-scanning-and-cc256x-firmware-uploading
[identity-protocol---part-9---ble-gatt-challenge-response-with-btstack]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56883/identity-protocol---part-9---ble-gatt-challenge-response-with-btstack
[accessing-the-pan1326b-bluetooth-module-with-the-arduino-ide-don-t-forget-to-set]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56850/accessing-the-pan1326b-bluetooth-module-with-the-arduino-ide-don-t-forget-to-set
[accessing-the-pan1326b-bluetooth-module-on-the-max32630fthr]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56831/accessing-the-pan1326b-bluetooth-module-on-the-max32630fthr
[identity-protocol---part-4---ble-using-pan1326b-and-btstack]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56853/identity-protocol---part-4---ble-using-pan1326b-and-btstack
