> ⚠️ OUTDATED — This document is from the single-board architecture. Will be refreshed after the two-board (ESP32-CAM + Arduino Nano) spec is complete.

# Flashing & Hardware Setup Guide

## Prerequisites

- ESP32-CAM (AI-Thinker) board
- ESP32-CAM-MB programmer board (USB-C)
- MicroSD card (FAT32 formatted, 1GB+ recommended)
- DS3231 RTC module with coin cell battery
- KY-016 RGB LED module
- Momentary push button
- MiniUPS power module + 3.7V lithium battery (for field deployment)

## 1. Wiring

| ESP32-CAM Pin | Connected To              |
|---------------|---------------------------|
| GPIO 0        | Push button → GND         |
| GPIO 12       | KY-016 Red pin            |
| GPIO 13       | KY-016 Green pin          |
| GPIO 33       | KY-016 Blue pin (inverted)|
| GPIO 14       | DS3231 SCL                |
| GPIO 15       | DS3231 SDA                |
| 5V            | DS3231 VCC, KY-016 VCC    |
| GND           | DS3231 GND, KY-016 GND, Button GND |

Insert the MicroSD card into the ESP32-CAM's built-in slot.

## 2. Flash the Firmware

Plug the ESP32-CAM into the ESP32-CAM-MB programmer board, connect via USB-C, then:

```bash
pio run -e esp32cam -t upload
```

If upload fails, hold the button on the ESP32-CAM-MB while pressing reset, then retry.

## 3. Monitor Serial Output

```bash
pio device monitor -b 115200
```

You should see boot messages like:
```
[BOOT] Power-on or reset
[BOOT] RTC initialized OK
[BOOT] SD card OK
[BOOT] Camera initialized OK
[BOOT] Device ready
```

If you see `[BOOT] SD card missing or unwritable`, check the MicroSD card is inserted and FAT32 formatted.

## 4. Set the DS3231 RTC Clock

The RTC needs to be set once. Create a temporary sketch or use the RTClib example:

```bash
pio lib install "adafruit/RTClib"
```

Upload this one-time sketch (replace `main.ino` temporarily, then restore):

```cpp
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  Wire.begin(15, 14);  // SDA=GPIO15, SCL=GPIO14
  
  if (!rtc.begin(&Wire)) {
    Serial.println("RTC not found");
    while (1);
  }
  
  // Set RTC to the compile time of this sketch
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  DateTime now = rtc.now();
  Serial.print("RTC set to: ");
  Serial.print(now.year()); Serial.print("-");
  Serial.print(now.month()); Serial.print("-");
  Serial.print(now.day()); Serial.print("T");
  Serial.print(now.hour()); Serial.print(":");
  Serial.print(now.minute()); Serial.print(":");
  Serial.println(now.second());
}

void loop() {}
```

After setting the clock, re-flash the main firmware:

```bash
pio run -e esp32cam -t upload
```

## 5. Change Device ID

Edit `include/config.h` and change:

```cpp
#define DEVICE_ID "01"
```

to the desired 2-digit ID (e.g., `"02"`, `"03"`, etc.). Re-flash after changing.

## 6. End-to-End Test Checklist

1. **Wake**: Press button → LED turns green
2. **QR scan**: Show a QR code containing "0001" → LED turns white (1s)
3. **Countdown**: White/green blink accelerates over 3s
4. **Capture**: LED solid white briefly
5. **Save**: LED solid blue for ~1.5s
6. **Success**: Blue/green alternating blink for 3s → back to green
7. **Invalid QR**: Show a QR with "ABCD" → LED red for 2s → back to green
8. **Inactivity sleep**: Wait 60s with no scans → LED turns off (deep sleep)
9. **Wake again**: Press button → LED green, device ready

## 7. Verify SD Card Contents

Remove the MicroSD card and check on a computer:

```
/DEVICE01/
  scan_log.csv
  DEVICE01_USER0001.jpg
```

- `scan_log.csv` should have header `user_id,device_id,timestamp` and one row per scan
- JPEG files should be ~30KB VGA photos
- Rescanning the same user overwrites the photo but appends a new CSV row

## 8. Field Deployment

1. Set the RTC clock (step 4)
2. Set the Device ID (step 5) and flash
3. Insert a fresh FAT32 MicroSD card
4. Connect MiniUPS + battery
5. Place in weatherproof enclosure
6. Verify green LED on button press
