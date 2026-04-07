# Wiring Guide — Wilderness QR Checkpoint Device

## ESP32-CAM Pinout Reference

The ESP32-CAM (AI-Thinker) has limited exposed GPIO pins. Many are shared between the camera, SD card, and flash LED. This guide covers only the external connections you need to make.

## Quick Reference — ESP32-CAM Pin Index

| ESP32-CAM Pin | GPIO | Device | Device Pin |
|---------------|------|--------|------------|
| IO0           | 0    | Push Button / OV2640 Camera | Leg 1 / XCLK (internal) |
| IO1           | N/A  | — (UART TX) | Not available — used for serial output |
| IO2           | 2    | SD Card (internal) | D0 |
| IO3           | N/A  | — (UART RX) | Not available — used for serial input |
| IO4           | N/A  | Flash LED (internal) | Not available — onboard flash LED + SD D1 |
| IO5           | N/A  | OV2640 Camera | Y2 (internal) |
| IO6           | N/A  | — (SPI Flash) | Not available — internal flash CLK |
| IO7           | N/A  | — (SPI Flash) | Not available — internal flash D0 |
| IO8           | N/A  | — (SPI Flash) | Not available — internal flash D1 |
| IO9           | N/A  | — (SPI Flash) | Not available — internal flash D2 |
| IO10          | N/A  | — (SPI Flash) | Not available — internal flash D3 |
| IO11          | N/A  | — (SPI Flash) | Not available — internal flash CMD |
| IO12          | 12   | KY-016 RGB LED | R (Red) |
| IO13          | 13   | KY-016 RGB LED | G (Green) |
| IO14          | 14   | DS3231 RTC / SD Card | SCL / SD_MMC CLK (shared) |
| IO15          | 15   | DS3231 RTC / SD Card | SDA / SD_MMC CMD (shared) |
| IO16          | N/A  | — (PSRAM) | Not available — used by PSRAM |
| IO17          | N/A  | — (PSRAM) | Not available — used by PSRAM |
| IO18          | N/A  | OV2640 Camera | Y3 (internal) |
| IO19          | N/A  | OV2640 Camera | Y4 (internal) |
| IO20          | N/A  | — | Not exposed on ESP32-CAM |
| IO21          | N/A  | OV2640 Camera | Y5 (internal) |
| IO22          | N/A  | OV2640 Camera | PCLK (internal) |
| IO23          | N/A  | OV2640 Camera | HREF (internal) |
| IO25          | N/A  | OV2640 Camera | VSYNC (internal) |
| IO26          | N/A  | OV2640 Camera | SIOD / SDA (internal) |
| IO27          | N/A  | OV2640 Camera | SIOC / SCL (internal) |
| IO28          | N/A  | — | Not exposed on ESP32 |
| IO29          | N/A  | — | Not exposed on ESP32 |
| IO30          | N/A  | — | Not exposed on ESP32 |
| IO31          | N/A  | — | Not exposed on ESP32 |
| IO32          | N/A  | OV2640 Camera | PWDN (internal) |
| IO33          | 33   | KY-016 RGB LED | B (Blue, inverted logic) |
| IO34          | N/A  | OV2640 Camera | Y8 (internal, input only) |
| IO35          | N/A  | OV2640 Camera | Y9 (internal, input only) |
| IO36          | N/A  | OV2640 Camera | Y6 (internal, input only) |
| IO37          | N/A  | — | Not exposed on ESP32-CAM |
| IO38          | N/A  | — | Not exposed on ESP32-CAM |
| IO39          | N/A  | OV2640 Camera | Y7 (internal, input only) |
| 5V            | —    | MiniUPS | 5V OUT |
| 3.3V          | —    | DS3231 RTC | VCC |
| GND           | —    | KY-016 RGB LED | GND (-) |
| GND           | —    | DS3231 RTC | GND |
| GND           | —    | Push Button | Leg 2 |
| GND           | —    | MiniUPS | GND |

## Connections

### KY-016 RGB LED Module

| KY-016 Pin | ESP32-CAM Pin | GPIO | Notes |
|------------|---------------|------|-------|
| R (Red)    | IO12          | 12   | Shared with SD_MMC. Detached during SD writes. |
| G (Green)  | IO13          | 13   | Shared with SD_MMC. Detached during SD writes. |
| B (Blue)   | IO33          | 33   | Onboard LED pin. Inverted logic (LOW = on). |
| GND (-)    | GND           | —    | Common ground. |

### DS3231 RTC Module

| DS3231 Pin | ESP32-CAM Pin | GPIO | Notes |
|------------|---------------|------|-------|
| SCL        | IO14          | 14   | Shared with SD_MMC CLK. Time-multiplexed in firmware. |
| SDA        | IO15          | 15   | Shared with SD_MMC CMD. Time-multiplexed in firmware. |
| VCC        | 3.3V            | —    | Use 3.3V to match ESP32 GPIO levels on I2C lines. |
| GND        | GND           | —    | Common ground. |

### Momentary Push Button (Wake)

| Button Pin | ESP32-CAM Pin | GPIO | Notes |
|------------|---------------|------|-------|
| One leg    | IO0           | 0    | Has internal pull-up. ext0 deep sleep wake source. |
| Other leg  | GND           | —    | Button press pulls GPIO 0 LOW to trigger wake. |

### MiniUPS Power Module

| MiniUPS Pin | ESP32-CAM Pin | Notes |
|-------------|---------------|-------|
| 5V OUT      | 5V            | Powers the ESP32-CAM and all peripherals. |
| GND         | GND           | Common ground. |
| BAT+/BAT-   | 18650 holder  | Connect to battery holder terminals. |
| Type-C IN   | —             | For charging the battery. |

## GPIO Summary

| GPIO | Function | Shared With | Available During SD Write |
|------|----------|-------------|---------------------------|
| 0    | Wake button | — | Yes |
| 2    | SD_MMC D0 (internal) | — | No (SD use) |
| 12   | KY-016 Red | SD_MMC | No — detached by firmware |
| 13   | KY-016 Green | SD_MMC | No — detached by firmware |
| 14   | DS3231 SCL (I2C) | SD_MMC CLK | No — used by SD |
| 15   | DS3231 SDA (I2C) | SD_MMC CMD | No — used by SD |
| 33   | KY-016 Blue | Onboard LED | Yes — only LED during saves |

## Pin Sharing Notes

- GPIO 12 and 13 are shared between the KY-016 LED and the SD card interface. The firmware automatically detaches the LED PWM channels before SD writes and reattaches them after. During SD operations, only the blue LED (GPIO 33) is active.
- GPIO 14 and 15 are shared between I2C (RTC) and SD_MMC. The firmware reads the RTC before or after SD operations, never simultaneously.
- GPIO 0 is the boot mode pin. It's safe to use as a button input after boot. During flashing, it may need to be held LOW — the ESP32-CAM-MB programmer handles this automatically.
- GPIO 33 uses inverted logic — the firmware handles this transparently. Writing LOW turns the blue LED on, HIGH turns it off.
