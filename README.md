# RC Combat Vehicle — Srijan Death Race 2026

Embedded firmware for a competition RC combat vehicle built by **Team ApeX** (Jadavpur University) for **Srijan Death Race 2026**. The vehicle takes throttle/steering input from an RC receiver and drives a dual-motor H-bridge setup with real-time interrupt-driven control, dead-zone filtering, ramped acceleration, and a signal-loss fail-safe.

## Overview

- **Platform:** Arduino Uno
- **Language:** Embedded C
- **Team:** Team ApeX (4 members)
- **My Role:** Chief Embedded Systems Architect — led embedded firmware design end-to-end

## Features

- **Interrupt-driven PWM decoding** — reads throttle and steering channels from the RC receiver in real time without blocking the main loop
- **Closed-loop motor control** — dead-zone filtering to ignore receiver jitter, plus ramped acceleration to prevent sudden torque spikes
- **BTS7960 H-bridge driving** — high-current motor control tuned to prevent stalls and flips during combat maneuvers
- **Signal-loss fail-safe** — automatically stops the vehicle if no valid RC signal is received for 500ms
- **Exponential steering response** — smoother, more precise low-speed handling while retaining full steering authority at high speed

## Hardware

| Component | Details |
|---|---|
| Microcontroller | Arduino Uno |
| Motor Driver | BTS7960 H-bridge |
| RC Receiver | PWM output, throttle + steering channels |
| Power | 3S1P 2200mAh Li-Po pack |
| Chassis | Custom combat-vehicle chassis (Team ApeX build) |

## How It Works

1. RC receiver PWM signals are captured via hardware interrupts on the Arduino Uno.
2. Raw pulse widths are filtered through a dead-zone to remove receiver noise near center/neutral.
3. Throttle output is ramped rather than applied instantly, preventing abrupt acceleration that could cause stalls or flips.
4. Steering input is passed through an exponential curve for finer control near center while preserving full lock at the extremes.
5. A watchdog timer continuously checks for valid signal pulses — if none are received within 500ms, the vehicle cuts motor output and stops.

## Repository Structure

```
.
├── Death Race Manuals/   # Reference manuals and datasheets
├── Death Race PPT.pdf    # Project presentation
├── rc_car_code.cpp       # Firmware: interrupt-driven PWM decoding, motor control, fail-safe logic
└── README.md
```

## Getting Started

1. Clone the repository and open `rc_car_code.cpp` in the Arduino IDE.
2. Select **Arduino Uno** as the target board.
3. Connect the RC receiver PWM lines and BTS7960 control pins as per the wiring/setup notes in `Death Race Manuals/`.
4. Flash the firmware and power the vehicle.
5. Verify the fail-safe by disconnecting the RC transmitter — motors should stop within 500ms.

## Result

Built and competed at **Srijan Death Race 2026**, an autonomous/RC combat vehicle competition at Jadavpur University.

## Author

**Jinnad Ul Raihan Chowdhury** — Chief Embedded Systems Architect, Team ApeX
[GitHub](https://github.com/jinnad287) · [LinkedIn](https://linkedin.com/in/jinnad287)
