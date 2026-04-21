# Sentinel Box

Element 14 Design Challenge
- [https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance](
  https://community.element14.com/challenges-projects/design-challenges/smart-security-and-surveillance)

# SentinelBox: Smart Secure Storage for Families

SentinelBox — Intelligent Device Lockbox Powered by MAX32630FTHR

## Blog

[https://saramic.github.io/sentinel-box/](
https://saramic.github.io/sentinel-box/)


## Project Summary

SentinelBox is a smart secure storage system designed to control access to
devices such as gaming consoles, tablets, or phones. The system demonstrates
how the **MAX32630FTHR** platform can implement an intelligent access control
system that combines identity verification, time-based policies, and behavioral
authentication.

Two **MAX32630FTHR** boards manage authentication and actuation. A stepper
motor FeatherWing physically locks the box, while the ICLED display shows
system status and alerts. The Ethernet FeatherWing provides secure logging and
parental monitoring features.

SentinelBox demonstrates how embedded AI can support healthy device usage
policies and secure storage in households or schools.

## Problem Statement

Many families struggle with controlling device access for children or shared
environments. Simple locks or software restrictions can be bypassed.

SentinelBox demonstrates a physical and intelligent solution that integrates
authentication, scheduling, and monitoring into a secure storage system.

## Key Features

* Time-based unlock policies
* Voice authentication
* Authorized phone presence detection
* Manual override for guardians
* Usage logs via Ethernet

## Setup and build

> **NOTE:** requires the download and installation of **LPSDK** (Low Power ARM
> Micro SDK) which is the legacy SDK with support for MAX32360 (new MSDK starts
> support from MAX32690)
>
>   - https://www.analog.com/en/products/max32630.html
>   - [Low Power ARM Micro SDK (Mac) 1.2.0](https://www.analog.com/en/resources/evaluation-hardware-and-software/embedded-development-software/software-download.html?swpart=SFW0001660A)
>     - login
>     - **`ARMCortexToolhchain.dmg`**

```sh
mise

# default PlatfromIO project - Mbed blink
# blinks 500ms ON/OFF
mise run build
mise run upload
mise run upload

# LPSDK based blink
# blinks RED🔴/GREEN🟢/BLUE🔵 LED based on code by @arvindsa
mise run clean:in_blink_LPSDK
mise run build:in_blink_LPSDK
mise run upload:in_blink_LPSDK

# Rust 🦀 based blink
# blinks 120ms ON/OFF
mise run clean:in_blink_rust
mise run build:in_blink_rust
mise run upload:in_blink_rust
```

### OpenOCD - build and install

OpenOCD can be built and installed form source via
a fork. When programming in Rust this means there
is no need to install LPSD.

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

# you can now remove the directory where openocd was built
rm -rf openocd
```
