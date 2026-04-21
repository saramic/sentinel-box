---
layout: post
title:  "Sentinel Box - Part I - the plan"
date:   2026-04-19 12:00:00 +1000
categories: sentinel-box update
---

My first forum post for the element14 [Smart security and
surveillance](smart-security-challenge) Design Challenge.

# The Idea

My house has a problem every parent recognises: devices. Tablets, phones,
controllers, earbuds as well as the odd jar of nutella, lollies and chocolate —
the endless negotiation about screen time, bedtime, weekend limits and self
control. As a family we have tried various self control mechanism, device only
zones, apps and even a time locking container. All have been defeated. The
closest was the time locking container, except that once set, there is no
override - you lock your phone in for 2 days, that's it. Also it had poor
battery life and not big enough to fit a laptop or block of chocolate.

**TODO** add picture of time locking container and stack of things to lock

The Smart **Sentinel Box** is a Perspex vault that physically locks devices
inside, and can only be opened by a "complex" and orchestrated process of more
than one party. The key is that it can be opened, just that more than one
person needs to make that decision, someone with opening rights. The
orchestrated part just means I can add more elaborate mechanisms as I learn to
work with the **MAX32360FTHR**. Simple ones first — a button, finger print
reader, a tap pattern — escalating to genuinely absurd ones: an audio quiz
streamed from a cloud lambda, a TOTP NFC card that expires every 30 seconds,
requiring both parents' fingerprints simultaneously, or a GPS geofence that
locks the box if it gets carried to a different room. Maybe through in a wake
word as well.

There's also a honeypot button — clearly labelled "EASY UNLOCK". It plays a
fake unlocking sound, does nothing, and silently texts both parents. Kids will
be furious.

The plan is to make the **MAX32630FTHR** the brain orchestrating everything
around it.

# Order placed and MAX32630 arrived

I was not lucky enought to get selected for a sponsored pack, but as I am new to
the Element14 Design Challenge community, I didn't want to give up that easy. I
placed an order and the **MAX32360** arrived. As I was waiting I was intriguted
by all the not so triviall posts on the setup required to program the
**MAX32360**. It seems that the platform is already EOL (End of life). I
thought:

- **1** ⚠️ no USB-C port
- **2** ⚠️ platform is EOL
  - [Analog Devices MAX32360](https://www.analog.com/en/products/max32630.html)
  - **NOT RECOMMENDED FOR NEW DESIGNS**
  - created almost 10 years ago - 2018 Maxim Integrated Products, Inc.

Too late now, dive in ... 🤿

I was lucky that others had documented their setups, which seemed more
complicated than I would like like this one by @skruglewicz

- [Forum#2 - MAX32630FTHR Dev Environments - skruglewicz](
  https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56843/forum-2---max32630fthr-development-environments---adaptive-sentinel-security-intelligence-hub)

- **3** ⚠️ works well with **Mbed** - EOL July 2026
  - [https://os.mbed.com/blog/entry/Important-Update-on-Mbed/](
     https://os.mbed.com/blog/entry/Important-Update-on-Mbed/)

It seems some people had luck with the **Arduino IDE** but I thought that may be a
last resort - I want my version control, plugins and AI that I have already
connected to **VSCode**.

  - [Programming the MAX32630FTHR with the Arduino IDE - Alistair](
    https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56838/programming-the-max32630fthr-with-the-arduino-ide-don-t-forget-to-set)

As the **MAX32360** arrived, I had to get down to program it. I started with
**PlatformIO** in VSCode as that has been my goto for Arduino and ESP32
projects. After a bit of pain, I could build it but I had no idea how to
upload? there was a seperate board for that? Going back to the Element14
community saved me having to read any manuals 👍.

  - [Forum Thread 2 EchoGuard – MAX32630FTHR Blink - Nidhee](
    https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56852/forum-thread-2-echoguard-max32630fthr-setup-first-blink-program-upload)

## OpenOCD

**OpenOCD** seems is what I needed, at this stage, I probably should have read
the fine print of where this needs to come from. Again, I went for my
preference to use Homebrew as a package manager and sure enough there was a
`brew install open-ocd` package. After working out I need to power "both"
boards and connect it to the programmer, I still got errors. Turns out I needed
an **AnalogDevices fork** of the **OpenOCD** package. After some time, I
installed it and it seemed to work - a flashing LED - that was a good 3 hours
of pain, just to flash a red LED.

- **4** ⚠️ requires special fork of OpenOCD to work
  - NOT This one [https://github.com/openocd-org/openocd/](
    https://github.com/openocd-org/openocd/)
  - THIS **fork** from Analog Devices Inc.
    [https://github.com/analogdevicesinc/openocd](
    https://github.com/analogdevicesinc/openocd)

for those interested, here are my steps to get the **AnalogDevices fork**
compiled and installed on a Mac OS (_similar for linux 🐧 maybe even WSL,
Windows Subsystem Linux 🪟🐧_).

```sh
# get the code
git clone https://github.com/analogdevicesinc/openocd --depth 1
rm -rf openocd/.git
cd openocd

# install some libraries requierd for building the package
brew install autoconf automake libtool pkg-config libusb hidapi jimtcl

  # UNTESTED
  # "likely" equivalent if using linux 🐧
  sudo apt-get install -y \
    autoconf automake libtool pkg-config \
    libusb-1.0-0-dev libhidapi-dev libjim-dev

./bootstrap

# some configurations to deal with some harmless warning
./configure \
  --enable-cmsis-dap \
  --disable-xds110 \
  CFLAGS="-g -O2 -Wno-error=gnu-folding-constant"

make -j$(sysctl -n hw.ncpu)

# install like a boss into /usr/local/bin/openocd
sudo make install
```

## Mbed

My first attempt was using the soon to be EOL **Mbed** system. It worked, but
given it's EOL status I wasn't satisfied.

## Maxim SDK

So next I was onto MSDK (Maxim which I worked out has dropped support for the
MAX32360.

- **5** ⚠️ current MSDK does not support MAX32**360**
  - [https://developer.analog.com/solutions/msdk](
    https://developer.analog.com/solutions/msdk) support starts at the
    MAX32**650** and it seems like [**MAX78000FTHR**](
    https://in.element14.com/new-products/development-boards-evaluation-tools/adi-max78000-ai-development-platforms)
    would be the way to go for some of the things I wanted to do like
    Multi-Keyword Recognition 🤦‍♂️
- **6** ⚠️ not a modern platform - see **MAX78000FTHR** (circa 2020)

So it was back to the legacy LPSDK - (Low Power ARM Micro SDK). This took me on
a number of mis adventures of massive installs and the like, that I thought
there has to be another way, why not use **Rust**.

## Rust

With the help of AI, I managed to get a build going. My custom built `OpenOCD`
succeeded in pushing it onto the board and another flashing LED. Looking at the
code was a bit tough, with snippets like

{% highlight rust %}
// raw register poke
(0x4000_A000 + 0x0080 + port * 4) as *mut u32).write_volatile(...)
{% endhighlight %}

Sounds like there is no HAL (Hardware Abstraction Layer) so if I want to use
any `GPIO`, `SPI`, `I2C`, `UART`, `Timers`, `ADC`,  I would need to write my
own. The good thing is that for plain old `GPIO` there is not that much, just
an offset of the address - so that part would just need a clean up. The bad
thing is that AI said it would take **2-3 weeks** to get that solid for full
hardware support.

## LPSDK (Low Power ARM Micro SDK)

Revisiting **LPSDK**, I worked out the install didn't intially work. Now post
install I have a blinking LED courtesy of the article.

- [Identity Protocol - Part 3 - arvindsa](
  https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56840/identity-protocol---part-3---unboxing-and-blinking-with-maxim-lpsdk)

Oh and this comes with, you guessed it, the correct OpenOCD in the toolchain -
maybe I should have RTFM 📚.

## Why the MAX32360?

That is 6 warnings why not to build on this platform. I really wander why this
was chosen as the central piece of the design challenge - it's not even cheaper
to buy the hardware over newer revisions.

1. ⚠️ no USB-C port
2. ⚠️ platform is EOL
3. ⚠️ **Mbed** is EOL July 2026
4. ⚠️ requires fork of **OpenOCD** to work
5. ⚠️ not supported by current **MSDK**
6. ⚠️ there are comparable more modern platforms like **MAX78000FTHR**

Did I miss a memo? Still, I bought it now, and I need to build something
`¯\_(ツ)_/¯` I need to get some credit on Element14 community!

## My hardware list

**TODO** image with TinyML and hardware


**TODO** image of design somewhere below

## Hermetic Builds — Going Full Unix Purist

I'm a Unix philosophy purist at heart — small, composable tools that each do
one thing well. So naturally I wanted to push the Rust experiment further and
pursue a fully hermetic, reproducible build pipeline with zero click-ops.

The goal: **infrastructure as code**, end to end. `git clone` && `mise run
build` on a fresh machine — no proprietary SDK downloads, no vendor portals
requiring account creation, no DMG files opened with a mouse. Every dependency
declared, every tool pinned, every flash command scripted.

Vendor-agnostic. SDK-free. GitOps-driven. Bare-metal Rust.

Where the **LPSDK** path requires a manual download from Analog Devices behind
a login wall, and **Mbed** is hurtling toward EOL, the Rust path gives us a
fully open, bootstrappable toolchain: `rustup`, `cargo`, `arm-none-eabi` via
the **Rust** target system, and OpenOCD from source. No mouse required. No
account required. No 6 GB installer required.

> **NOTE:** `arm-none-eabi` is the GNU cross-compiler toolchain designed for
> building "bare-metal" applications on 32-bit and 64-bit Arm Cortex-M,
> Cortex-R and other embedded processors.
>
> **NOTE:** `arm-none-eabi` supports C and C++ but **Rust** 🦀 is "cool" 😎
> 
> [https://developer.arm.com/downloads/-/gnu-rm](https://developer.arm.com/downloads/-/gnu-rm)

The results for a **hermetic build** with LED blink were optimistic

Platform    | build    | from scratch
------------|---------:|-------------------------------------------------------
**Mbed**    | ~ 95 sec | ? ~ 30 min<br> (_Via PlatfromIO install_)
**LPSDK**   |  ~ 6 sec | ? ~ 60 min<br> (_LPSDK signup download and install_)
**Rust 🦀** |  ~ 5 sec |   ~ 15 sec<br> _with clear cache_<br>`rm -rf ~/.cargo/registry ~/.cargo/git`

In pursuit of this zero click-ops embedded development dream — I decided to
continue a little bit more with the **Rust** build.

**TODO** can I get animated gif and some basics running for an attitude meter
and 6 axis accelerometer?


[smart-security-challenge]: https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/
