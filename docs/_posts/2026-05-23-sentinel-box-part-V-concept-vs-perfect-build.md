---
layout: post
title:  "Sentinel Box - Part V - Concept vs Perfect Build"
date:   2026-05-23 16:00:00 +1000
categories: sentinel-box update
---

The competition ends tomorrow and I have fallen in a heap a week ago. I need to
re-group on what I can achieve with my abilities, and be content with that. It
will not be the perfect build I was hoping for, it may only partly touch on the
concept I am aiming for, but such is life.

## Recap

The idea is to build a smart lock box for digital devices to help control
digital addiction, more on the idea can be found in **`part I`** and coding with
LPSDK and accessing a fingerprint scanner in **`part II`**, and mechanical
stepper motors and vault lock mechanisms in **`part III`** and Bluetooth and
BTStack in **`part IV`**:

- [Sentinel Box - Part I - the plan][sentinel-box-part-I-the-plan]
- [Sentinel Box - Part II - back to C][sentinel-box-part-II-back-to-C]
- [Sentinel Box - Part III - Stepper Motor and Vault lock mechanism
  ][sentinel-box-part-III-stepper-motor-and-vault-lock-mechanism]
- [Sentinel Box - Part IV - Bluetooth and BTStack
  ][sentinel-box-part-IV-bluetooth-and-btstack]

## Perfect Build

Looking back honestly, I have learnt an enormous amount across this challenge. I
attempted a fully hermetic **Rust** build, maybe next time. I did, with the help
of the amazing Element 14 community, get to successfully drive a stepper motor,
fingerprint scanner and connected **Bluetooth** all for the first time. My final
piece was a vault mechanism that look great in initial previews and I was
genuinely excited about completing.

The perspex work is where the gap between concept and reality opened up.
Precision mechanical assembly requires tolerance, and I underestimated it
badly. Only today I had been watching Clickspring's extraordinary series on
building an [Antikythera
mechanism](https://www.youtube.com/watch?v=dRXI9KLImC4&list=PLZioPDnFPNsHnyxfygxA0to4RXv4_jDU2)
and there is something humbling about watching someone fit-and-file a part to
within a tenth of a millimetre with hand tools. My lock mechanism needed that
same care. It did not get it. I cut fast, missed a few steps and thought it
better try when I had the right tools and patience to actually get it right and
build a mechanism that was "square", worked and worthy of display.

![Vault and door mechanism during build](
/sentinel-box/assets/20260523_vault_and_door_mechanism_in_build.jpg)

The final week did not help. A hectic schedule, a weekend away from home and
tools. Even though I was excited about the mechanical build, the perspex was not
as forgiving as re-burning a new build onto the **MAX32630FTHR**.

What I _did_ get started on is expanding the Bluetooth communications layer with
the browser. As the theoretical setup would need stepper motor setting, reading
a number of finger prints and setting up a consensus framework for who can
co-operatively open the lock box, doing this via a web site seemed the way to
go. The foundation is super sound:

- **React 19** for a modern frontend
- **Lit 3** hybrid with the React to allow for a custom **web
  component** (`<sentinel-ble-manager>`)
- **shadcn + Radix UI + Tailwind CSS** for styling
- **Vite 8** as the build

The part I am most pleased with is that the browser and the firmware speak
exactly the same binary protocol. The **MAX32630FTHR** stores its config in a
100-byte block in INFO flash, and the web console encodes and decodes the
identical layout — offsets and all — using `DataView` and `Uint8Array`:

{% highlight typescript %}
view.setUint16(0x00, CONFIG_MAGIC, true)   // magic  0x5B5A ("SB" LE)
view.setUint16(0x02, cfg.vaultSteps, true) // vault_steps
buf[0x04] = enrolledCount                  // fp_count
buf[0x05] = cfg.unlockPolicy               // unlock_policy
buf[0x06] = cfg.relockMinutes              // relock_minutes
// fp_name[10]: 8 bytes each starting at 0x08
// fp_role[10]: 1 byte each starting at 0x58
view.setUint16(CRC_OFFSET, crc, true)      // CRC-16/CCITT over bytes 0x00–0x61
{% endhighlight %}

To make sure nothing gets corrupted in transit the browser runs the same
**CRC-16/CCITT** the firmware uses, so a tampered or truncated blob is caught
at both ends:

{% highlight typescript %}
export function crc16(data: Uint8Array): number {
  let crc = 0xffff
  for (const byte of data) {
    crc ^= byte << 8
    for (let i = 0; i < 8; i++) {
      crc = crc & 0x8000 ? ((crc << 1) ^ 0x1021) & 0xffff : (crc << 1) & 0xffff
    }
  }
  return crc
}
{% endhighlight %}

It is not the kind of JavaScript you write every day, but it is satisfying when
the numbers match on both sides of the BLE link.

<img src="/sentinel-box/assets/20260523_Red_Green_Blue_via_bluetooth.gif"
  alt="Website to control MAX32630FTHR LED over bluetooth" width="740" />

So the build is not what I imagined. The vault door is not ready, the full
orchestration of fingerprint plus Bluetooth plus stepper motor is yet to run
end-to-end. But the pieces exist. The concept holds. I know now exactly which
joints need filing to tolerance and which parts of the stack are solid. That is
not failure, that is a prototype.

## Next

Well it's crunch time, next step the full write up at [Sentinel box: consensus
based lock box][sentinel-box-project].

## Source

[https://github.com/saramic/sentinel-box][github-sentinel-box]

[smart-security-challenge]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/
[sentinel-box-part-I-the-plan]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56869/sentinel-box---part-i---the-plan
[sentinel-box-part-II-back-to-C]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56894/sentinel-box---part-ii---back-to-c
[sentinel-box-part-III-stepper-motor-and-vault-lock-mechanism]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56937/sentinel-box---part-iii---stepper-motor-and-vault-lock-mechanism
[sentinel-box-part-IV-bluetooth-and-btstack]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56942/sentinel-box---part-iv---bluetooth-and-btstack
[sentinel-box-project]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/b/projects/posts/sentinel-box---consensus-based-lock-box
[github-sentinel-box]: https://github.com/saramic/sentinel-box