# Work Log

## Sun 19 Apr 2026

### Setup project

```sh
brew install platformio
# and dependencies
brew install certifi python@3.14
```

setup a PlatformIO project

```sh
platformio project init
```

```sh
# in PlatformIO searching for 32630
# and clicking link
# https://docs.platformio.org/en/latest/boards/maxim32/max32630fthr.html
# add to platform.ini
[env:max32630fthr]
platform = maxim32
board = max32630fthr
```

Following [Forum Thread 2 EchoGuard – MAX32630FTHR Setup & First Blink Program Upload - Nidhee](
https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance/f/forum/56852/forum-thread-2-echoguard-max32630fthr-setup-first-blink-program-upload)

```sh
brew install open-ocd

# find the board
find $(dirname $(which openocd))/../share/openocd/scripts -name "max3263*.cfg"
/opt/homebrew/bin/../share/openocd/scripts/target/max3263x.cfg

# and update the platform.ini
[env:max32630fthr]
platform = maxim32
board = max32630fthr

framework = mbed ; or arduino depending on your preference

upload_protocol = custom
upload_command = openocd

debug_tool = cmsis-dap
```

Seems to do a full compile each time and ultimately the open-ocd upload fails

Trying from source

```sh
git clone https://github.com/analogdevicesinc/openocd --depth 1
rm -rf openocd/.git
cd openocd

brew install autoconf automake libtool pkg-config libusb hidapi
brew install jimtcl

./bootstrap

# ./configure --enable-cmsis-dap
# ./configure --enable-cmsis-dap --with-jimtcl-static
# ./configure --enable-cmsis-dap --disable-xds110
./configure --enable-cmsis-dap --disable-xds110 \
  CFLAGS="-g -O2 -Wno-error=gnu-folding-constant"

make -j$(sysctl -n hw.ncpu)
sudo make install
```

which installs it to

```sh
which openocd
/usr/local/bin/openocd
```

should probably follow the instructions on Analog Devices site
- [https://analogdevicesinc.github.io/msdk//USERGUIDE/#completing-the-installation-on-macos](
  https://analogdevicesinc.github.io/msdk//USERGUIDE/#completing-the-installation-on-macos)

```sh
brew install libusb-compat libftdi hidapi libusb
```

Download:
- [https://analogdevicesinc.github.io/msdk//USERGUIDE/#download](
  https://analogdevicesinc.github.io/msdk//USERGUIDE/#download)
  - [https://www.analog.com/en/resources/evaluation-hardware-and-software/embedded-development-software/software-download.html?swpart=SFW0018610B](
    https://www.analog.com/en/resources/evaluation-hardware-and-software/embedded-development-software/software-download.html?swpart=SFW0018610B)
    - sign up for an account

Back in just using the manually built and installed openocd

I decided to move my experiments into `./experiments` folder this caused an
issue with the command line `pio` (platformio) program finding the wrong python
`python@3.14` from homebrew and the `framework-mbed` is too old. The
recommendation was to brew uninstall platformio and use mise to install a
specific python and pip to install platformio

```sh
brew uninstall platformio
brew uninstall --ignore-dependencies python@3.14

mise use python@3.11
pip install platformio

# now
which pio
$USER/.local/share/mise/installs/python/3.11/bin/pio
```

Now the whole thing is a bit more `mise` dirven

```sh
mise run build
mise run upload
```

### Start blogging

setup jekyll and Github pages for https://saramic.github.io/sentinel-box/

install ruby

```sh
# attempt mise self update
mise self-update
# mise ERROR mise is installed via a package manager, cannot update

# realise I isntalled it via homebrew, hence
brew update mise

# list the local rubies
mise list ruby

# and all sources
mise list --all-sources ruby

# finally worked out to list remote ones
mise ls-remote ruby

# and install the latest 4.0.2
mise install ruby 4.0.2
mise use ruby@4.0.2
```

install jekyll following [https://jekyllrb.com/docs/](
https://jekyllrb.com/docs/)

```sh
gem install jekyll bundler
jekyll new docs

# run it
mise run dev-blog
```

but will ruby 4 and jekyll 4.4 run on github pages? do I need the [github-pages
GEM](https://github.com/github/pages-gem)?

and configuring the `main` branch and `./docs` directory to be a **Pages** via
[https://github.com/saramic/sentinel-box/settings/pages](
https://github.com/saramic/sentinel-box/settings/pages)

**NO**

That only exposes the site and does not actually execute **Jekyll** to build
it. Attempting to add the `github-page` gem blows up wit

```less
bundle add github-pages
[DEPRECATED] Platform :mingw, :x64_mingw, :mswin will be removed in the future. Please use platform :windows instead.
Fetching gem metadata from https://rubygems.org/.........
Resolving dependencies...
Could not find compatible versions

    Because github-pages >= 135, < 178 depends on minima = 2.1.1
      and github-pages >= 44, < 147 depends on liquid = 3.0.6,
      github-pages >= 44, < 178 requires minima = 2.1.1 or liquid = 3.0.6.
(1) So, because github-pages >= 178 depends on jekyll-sass-converter = 1.5.2
      and github-pages >= 28, < 44 depends on jekyll = 2.4.0,
      github-pages >= 28 requires minima = 2.1.1 or liquid = 3.0.6 or jekyll-sass-converter = 1.5.2 or jekyll = 2.4.0.

    Because github-pages >= 9, < 14 depends on kramdown = 1.2.0
      and github-pages < 9 depends on kramdown = 1.0.2,
      github-pages < 14 requires kramdown = 1.0.2 OR = 1.2.0.
    And because github-pages >= 14, < 32 depends on kramdown = 1.3.1,
      github-pages < 32 requires kramdown = 1.0.2 OR = 1.2.0 OR = 1.3.1.
    And because github-pages >= 28 requires minima = 2.1.1 or liquid = 3.0.6 or jekyll-sass-converter = 1.5.2 or jekyll = 2.4.0 (1),
      one of minima = 2.1.1 or liquid = 3.0.6 or jekyll-sass-converter = 1.5.2 or jekyll = 2.4.0 or kramdown = 1.0.2 OR = 1.2.0 OR = 1.3.1 must be true.
    And because jekyll >= 4.3.0 depends on jekyll-sass-converter >= 2.0, < 4.0,
      jekyll >= 4.3.0 requires minima = 2.1.1 or liquid = 3.0.6 or kramdown = 1.0.2 OR = 1.2.0 OR = 1.3.1.
    And because jekyll >= 4.3.0 depends on kramdown >= 2.3.1, < 3.A
      and jekyll >= 3.5.0 depends on liquid ~> 4.0,
      jekyll >= 4.3.0 requires minima = 2.1.1.
    So, because Gemfile depends on jekyll ~> 4.4.1
      and Gemfile depends on minima ~> 2.5,
      version solving has failed.
```

at this point, I'm just going to go back to what I know works

```sh
# downgrade to a ruby that should work
mise use ruby@3.2.2
gem install jekyll bundler

# re-create jekyll docs blog
rm -rf docs
jekyll new docs

# add required gems github-pages AND webrick
cd docs
bundle add github-pages
bundle add webrick

# check jekyll works locally
```

seems to build but still not showing a built page in GitHub pages

Also in the GHActions build, I notised a **Warning** which may allow me to
update the version of Jekyll
* [https://jekyllrb.com/docs/continuous-integration/github-actions/](
  https://jekyllrb.com/docs/continuous-integration/github-actions/)

Finally to decide on a better theme:
* [https://docs.github.com/en/pages/setting-up-a-github-pages-site-with-jekyll/adding-a-theme-to-your-github-pages-site-using-jekyll](
  https://docs.github.com/en/pages/setting-up-a-github-pages-site-with-jekyll/adding-a-theme-to-your-github-pages-site-using-jekyll)
  - [Architect](https://pages-themes.github.io/architect/) probably a winner
    with a clear "blue print" style
  - [Caymen](https://pages-themes.github.io/cayman/) nice and clean and more
    modern than the original
  - [Hacker](https://pages-themes.github.io/hacker/) dark and in theme but
    would need some tweaking
  - [leap-day](https://pages-themes.github.io/leap-day/) a bit busy but with
    tweaking could work
  - [minima](https://jekyll.github.io/minima/) seems similar but just a little
    nicer than the default? - but supposedly this is the default in
    `_config.yml`
  - Others that don't really rate:
    [dinky](https://pages-themes.github.io/dinky/),
    [Merlot](https://pages-themes.github.io/merlot/),
    [Midnight](https://pages-themes.github.io/midnight/),
    [Minimal](https://pages-themes.github.io/minimal/)

attempted with the following but did not work so giving up for time being (also
added the `assets/css/style.scss` file, leaving that as it doesn't break
anything)

```diff
diff --git a/docs/_config.yml b/docs/_config.yml
index e3aabcb..e1f68a7 100644
--- a/docs/_config.yml
+++ b/docs/_config.yml
@@ -35,8 +35,10 @@ timezone: Australia/Melbourne
 # Build settings
 markdown: kramdown
+# theme: minima
+remote_theme: pages-themes/architect@v0.2.0
 plugins:
   - jekyll-feed
+  - jekyll-remote-theme

 # Exclude from processing.
 # The following items will not be processed, by default.
```

## Thu 16 Apr 2026

Seems there are a lot of write ups on how to program the **MAX32630FTHR**. I
think I will attempt to use VSCode and Platform.io but in reality a `make`
script with a command line build would be preferable. Some information here

- **GitHub: analogdevicesinc/msdk** Software Development Kit for Analog
  Device's MAX-series microcontrollers
  - [https://github.com/analogdevicesinc/msdk?tab=readme-ov-file](
    https://github.com/analogdevicesinc/msdk?tab=readme-ov-file)

- how to setup MSDK for commandline
  - [https://analogdevicesinc.github.io/msdk//USERGUIDE/#getting-started-with-command-line-development](
    https://analogdevicesinc.github.io/msdk//USERGUIDE/#getting-started-with-command-line-development)

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

### Purchase list

- ✅ [Digikey: MAX32630FTHR](https://www.digikey.com.au/en/products/detail/analog-devices-inc-maxim-integrated/MAX32630FTHR/6575544) $61
- ✅ [Digikey: AdaFruit 4690 fingerprint sensor](https://www.digikey.com.au/en/products/detail/adafruit-industries-llc/4690/13170958) $30
  - or
  - [AliExpress: R307 finger print sensor](https://www.aliexpress.com/item/32815391770.html) $16
- [Digikey: AdaFruit 364 NFC evaluation board](https://www.digikey.com.au/en/products/detail/adafruit-industries-llc/364/6238001) $44
  - or
  - [AliExpress: PN532 NFC Arduino board](https://www.aliexpress.com/item/1005006837891461.html) $13/4pcs
- [Digikey: DFRobot DFR0119-0 Eval board for PAM8403 Amp](https://www.digikey.com.au/en/products/detail/dfrobot/DFR0119-O/13978501) $7
  - or
  - [AliExpress: PAM8403 Audio Amp](https://www.aliexpress.com/item/1005010021895446.html) $3/10pcs
- [AliExpress: TTP223 Touch Sensor](https://www.aliexpress.com/item/1005006087171183.html) $9/70pcs
- [AliExpress: OV2640 Camera Module 2MP Megapixel](https://www.aliexpress.com/item/33046344720.html) $8

and related-ish

- [AliExpress: AD8317 RF Signal Power Meter](https://www.aliexpress.com/item/1005009041453030.html) $10
- ✅ [DigiKey: DRV2605L eval board (haptic driver)](https://www.digikey.com.au/en/products/detail/adafruit-industries-llc/2305/5356831) $9
- ✅ [DigiKey: Vibrating Motor](https://www.digikey.com.au/en/products/detail/olimex-ltd/VIBRATING-MOTOR/21661954) $1
- ✅ [DigiKey: IRLML6344 N channel MOSFET](https://www.digikey.com.au/en/products/detail/infineon-technologies/IRLML6344TRPBF/2538152) $1

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
