# Submission

I ended up adding 4 submissions

1. **VolkWatch** - AI-Powered Wildlife vs Human Intruder Perimeter Sentry
2. **WolfWatch RF**  — TinyML Spectrum Sentinel for Smart Security
3. **HomeWolf** - Multi-Factor Smart Guardian Lock
4. **SentinelBox:** Smart Secure Storage for Families

**WolfWatch RF** is probably what I wanted to build, an RF fingerprinting
detector of devices like drones. As I also want to do the Arduino UNO Q
challenge, I have decided to go simnple here and just do the **SentinelBox**. A
simple remote control box for digital abstience in families.

# VolkWatch — AI-Powered Wildlife vs Human Intruder Perimeter Sentry

## Project Description:

VolkWatch is an AI-enabled smart perimeter sentinel designed for remote outdoor
environments such as campsites, scout jamborees, conservation zones, and rural
properties. Its purpose is to distinguish between natural wildlife activity and
genuine human intrusions using on-device machine learning models running at the
edge on the Analog Devices MAX32630FTHR platform. Traditional motion sensors
often generate nuisance alarms from animals or environmental noise, reducing
trust in security alerts; VolkWatch solves this by combining multi-sensor data
(acoustic signatures, motion patterns, vibration features, and radio spectrum
analysis) to make real-time, confidence-based classifications without relying
on cloud connectivity.

Leveraging TinyML techniques and TensorFlow Lite Micro, VolkWatch extracts
spectral and temporal features from ambient sound and motion in real time,
feeding them into a compact neural network that classifies events as Human,
Animal, or Noise/Environment. The system wakes from low-power sleep only when
significant events are detected, maximizing battery life for extended remote
deployment. Upon classification of a human intrusion, the node triggers local
alerts (LED, buzzer) and can optionally transmit secure notifications via
low-power wireless (e.g., LoRa or BLE), making it suitable even in
low-connectivity environments.

This project will fully utilize features of the MAX32630FTHR kit — including
its low-power Cortex-M4F MCU, audio sampling, sensor inputs, and display
modules — to demonstrate an effective AI-at-the-edge security prototype. The
build log will cover requirements analysis, sensor integration, embedded TinyML
model training and optimization, hardware prototyping, firmware design, and
real-world testing under outdoor conditions.

# WolfWatch RF  — TinyML Spectrum Sentinel for Smart Security

## Project Description:

WolfWatch RF is a passive smart-security prototype that uses the
**MAX32630FTHR** as an edge-AI controller to monitor radio-spectrum activity
and classify security-relevant RF signatures in real time. Instead of relying
only on cameras or motion sensors, WolfWatch RF treats the local spectrum as
another source of situational awareness and looks for patterns such as
radiosonde telemetry, drone-like 2.4 GHz activity, weather-radar-like pulse
trains, and the presence of digital/encrypted traffic in monitored bands. The
system is receive-only and does not attempt to decode protected communications;
its goal is to turn invisible RF activity into practical local alerts.

The hardware architecture is intentionally embedded and low power. An
**SX1276**-based module scans sub-GHz channels over SPI, an
**SX1280/SX1281**-based module monitors 2.4 GHz burst activity over SPI, and an
**AD8318** RF detector measures power in selected filtered bands from MHz
through microwave frequencies. The **MAX32630FTHR** collects those
measurements, extracts compact features such as channel occupancy, burst
timing, pulse repetition, and energy histograms, and then runs a lightweight
TinyML classifier to label events as known activity classes or unknown
anomalies. The onboard Bluetooth and microSD on the MAX32630FTHR make it
suitable for local alerts, logging, and mobile notifications without needing a
Linux host.

This project fits the smart security and surveillance theme because it provides
passive RF situational awareness for campsites, field operations, temporary
events, and remote sites. WolfWatch RF will demonstrate how the
**MAX32630FTHR** can be used not just as a general microcontroller board, but
as a compact edge-intelligence node for real-world signal classification,
anomaly detection, and low-power security monitoring. The finished prototype
will show that “smart surveillance” can include RF awareness as well as
traditional sensors, reducing blind spots and creating a more intelligent
security picture. (edited) 

# HomeWolf : Multi-Factor Smart Guardian Lock

HomeWolf — Multi-Factor AI Guardian Lock Using MAX32630FTHR

## Project Summary

HomeWolf is a multi-factor intelligent access control system built around the
**MAX32630FTHR** platform that protects a home entry point or secured container
(such as a lockbox for devices or valuables). The system combines biometric
authentication, device presence detection, and behavioral signals to determine
whether access should be granted. When the correct combination of factors is
detected, the system drives a stepper motor via the Adafruit DC Motor + Stepper
FeatherWing to physically unlock a door latch or enclosure.

Two **MAX32630FTHR&& boards are used cooperatively: one acts as the sensor and
identity node, and the other as the secure actuator and network controller. The
Würth ICLED FeatherWing display provides real-time system status, while the
Particle Ethernet FeatherWing enables secure network connectivity for logging,
remote alerts, and configuration.

The goal of HomeWolf is to demonstrate how embedded AI and multi-sensor fusion
can provide stronger security than traditional single-factor locks.

## Problem Statement

Traditional locks rely on single authentication factors such as keys or PIN
codes, which can be easily lost, stolen, or shared. Modern smart locks often
rely solely on smartphone presence or cloud authentication.

**HomeWolf** demonstrates a layered, privacy-preserving edge-AI approach where
authentication decisions are made locally by the **MAX32630FTHR** hardware
using multiple independent signals.

## Core Security Factors

* Possible authentication signals include:
* Voice recognition / passphrase
* Face or visual recognition
* Phone proximity / Bluetooth presence
* Motion or gesture pattern
* Custom pass-gesture sequence

Access is granted only when a valid combination of signals is detected.

# SentinelBox: Smart Secure Storage for Families

SentinelBox — Intelligent Device Lockbox Powered by MAX32630FTHR

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
