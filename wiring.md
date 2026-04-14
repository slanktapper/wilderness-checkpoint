# Wiring Diagram — Wilderness QR Checkpoint

## Overview

Two-board system: Arduino Nano (controller) + ESP32-CAM (camera/storage). The Nano controls power to the ESP32-CAM via a P-channel MOSFET. Boards communicate over UART with a voltage divider on the Nano TX line (5V → 3.3V).

## Power Supply

```
18650 Battery (3.7V)
    │
    ▼
MiniUPS Module (5V output)
    │
    ├──── Nano 5V pin (always on)
    │
    └──── MOSFET Source ──── ESP32-CAM 5V pin (switched)
```

The MiniUPS provides regulated 5V from the 18650 cell. The Nano is always powered. The ESP32-CAM is powered through the MOSFET, controlled by the Nano.

## MOSFET Power Switch (FQP27P06)

```
                    MiniUPS 5V
                        │
                     [Source]
                        │
    Nano D8 ───────[Gate]  FQP27P06 (P-channel)
                        │
                     [Drain]
                        │
                   ESP32-CAM 5V


    MiniUPS 5V ─── 10kΩ ─── Nano D8 (Gate)
```

- Gate LOW (Nano D8 = LOW) → MOSFET conducts → ESP32-CAM powered ON
- Gate HIGH (Nano D8 = HIGH) → MOSFET off → ESP32-CAM powered OFF
- 10kΩ pull-up resistor from gate to 5V rail ensures ESP32-CAM stays OFF when Nano is sleeping or resetting

## UART Connection (Nano ↔ ESP32-CAM)

```
Nano TX (D1, 5V) ──── 1kΩ ────┬──── ESP32-CAM RX (GPIO3)
                               │
                              2kΩ
                               │
                              GND

Vout = 5V × 2kΩ / (1kΩ + 2kΩ) = 3.33V


ESP32-CAM TX (GPIO1, 3.3V) ──────── Nano RX (D0)
```

- Nano TX → ESP32-CAM RX: voltage divider required (5V → 3.3V)
- ESP32-CAM TX → Nano RX: direct connection (3.3V exceeds Nano's logic HIGH threshold)
- Baud rate: 115200

Note: disconnect these UART wires when flashing either board via USB, since D0/D1 are shared with the Nano's USB serial and GPIO1/GPIO3 are the ESP32-CAM's USB serial.

## DS3231 RTC Module

```
DS3231        Nano
──────        ────
VCC    ────── 5V
GND    ────── GND
SDA    ────── A4
SCL    ────── A5
```

Standard I2C connection. The Nano uses its default I2C pins (A4/A5). No external pull-ups needed — the DS3231 module has them onboard.

## KY-016 RGB LED Module

```
KY-016        Nano
──────        ────
R      ────── D9  (PWM)
G      ────── D10 (PWM)
B      ────── D11 (PWM)
GND    ────── GND
```

Common cathode module with built-in current-limiting resistors. No external resistors needed. PWM pins allow brightness control.

## Wake Button

```
Nano D2 (INT0) ────── Button ────── GND
```

- Uses Nano's internal pull-up resistor (configured in firmware)
- Falling edge triggers INT0 interrupt to wake from deep sleep
- No external pull-up or debounce circuit needed (software debounce in firmware)

## 5-Position DIP Switch

```
DIP Switch        Nano
──────────        ────
SW1 (bit 0) ───── D3
SW2 (bit 1) ───── D4
SW3 (bit 2) ───── D5
SW4 (bit 3) ───── D6
SW5 (bit 4) ───── D7
Common      ───── GND
```

- Active LOW: ON position connects pin to GND (reads LOW)
- Nano uses internal pull-up resistors (configured in firmware)
- OFF = HIGH (1), ON = LOW (0) — firmware inverts to get the binary device ID
- All OFF = binary 00000 = Device ID 0 = invalid configuration

## MicroSD Card

The MicroSD card inserts into the ESP32-CAM's built-in slot. No external wiring needed. Must be FAT32 formatted.

## Complete Nano Pin Assignment

| Pin | Function | Connection |
|-----|----------|------------|
| D0 (RX) | UART RX | ← ESP32-CAM TX (GPIO1), direct 3.3V |
| D1 (TX) | UART TX | → 1kΩ → junction → ESP32-CAM RX (GPIO3); junction → 2kΩ → GND |
| D2 | Wake button (INT0) | → button → GND |
| D3 | DIP switch bit 0 | → SW1 → GND |
| D4 | DIP switch bit 1 | → SW2 → GND |
| D5 | DIP switch bit 2 | → SW3 → GND |
| D6 | DIP switch bit 3 | → SW4 → GND |
| D7 | DIP switch bit 4 | → SW5 → GND |
| D8 | MOSFET gate | → FQP27P06 gate; 10kΩ pull-up to 5V |
| D9 | LED Red | → KY-016 R pin |
| D10 | LED Green | → KY-016 G pin |
| D11 | LED Blue | → KY-016 B pin |
| A4 | I2C SDA | → DS3231 SDA |
| A5 | I2C SCL | → DS3231 SCL |
| 5V | Power | ← MiniUPS 5V out |
| GND | Ground | Common ground bus |

## Complete ESP32-CAM Pin Assignment

| Pin | Function | Connection |
|-----|----------|------------|
| GPIO1 (U0TXD) | UART TX | → Nano RX (D0), direct |
| GPIO3 (U0RXD) | UART RX | ← Nano TX (D1) via 1kΩ/2kΩ voltage divider |
| 5V | Power | ← MOSFET drain (switched 5V) |
| GND | Ground | Common ground bus |
| SD slot | MicroSD | Insert FAT32 card |
| Camera | OV2640 | Built-in ribbon cable (no wiring needed) |

All other ESP32-CAM GPIO pins are used internally by the camera and SD card interfaces — do not connect anything to them.

## Ground Bus

All GND pins must be connected together on a common ground bus:

- Nano GND
- ESP32-CAM GND
- DS3231 GND
- KY-016 GND
- DIP switch common
- Wake button (one terminal)
- Voltage divider 2kΩ bottom
- MiniUPS GND
- MOSFET source (if using separate ground reference)

## Schematic Summary

```
                         ┌─────────────────────────────────────┐
                         │          MiniUPS 5V Output          │
                         └──┬──────────────┬───────────────────┘
                            │              │
                            │         ┌────┴────┐
                            │         │  10kΩ   │
                            │         └────┬────┘
                            │              │
                         Nano 5V      Nano D8 ──── MOSFET Gate
                            │                       │
                            │              MOSFET Source ── 5V rail
                            │              MOSFET Drain ── ESP32-CAM 5V
                            │
              ┌─────────────┼─────────────────────────────────┐
              │         ARDUINO NANO                          │
              │                                               │
              │  D0 (RX) ◄──────────────── ESP32-CAM GPIO1   │
              │  D1 (TX) ──── 1kΩ ──┬───── ESP32-CAM GPIO3   │
              │                     2kΩ                       │
              │                     GND                       │
              │                                               │
              │  D2 ──── Button ──── GND                      │
              │  D3 ──── DIP SW1 ─── GND                     │
              │  D4 ──── DIP SW2 ─── GND                     │
              │  D5 ──── DIP SW3 ─── GND                     │
              │  D6 ──── DIP SW4 ─── GND                     │
              │  D7 ──── DIP SW5 ─── GND                     │
              │                                               │
              │  D9  ──── KY-016 R                            │
              │  D10 ──── KY-016 G                            │
              │  D11 ──── KY-016 B                            │
              │           KY-016 GND ── GND                   │
              │                                               │
              │  A4 (SDA) ──── DS3231 SDA                     │
              │  A5 (SCL) ──── DS3231 SCL                     │
              │                DS3231 VCC ── 5V               │
              │                DS3231 GND ── GND              │
              └───────────────────────────────────────────────┘
```
