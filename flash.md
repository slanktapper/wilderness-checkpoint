# Flashing & Hardware Setup Guide

## Prerequisites

- Arduino Nano (ATMEGA328P) — main controller
- ESP32-CAM (AI-Thinker) + ESP32-CAM-MB programmer board (USB-C)
- DS3231 RTC module with CR2032 coin cell battery
- KY-016 RGB LED module (common cathode)
- FQP27P06 P-channel MOSFET
- 5-position DIP switch
- Momentary push button
- Resistors: 1kΩ, 2kΩ, 10kΩ
- MicroSD card (FAT32 formatted, 1GB+)
- MiniUPS module + 18650 battery (for field deployment)
- Dupont jumper wires

See `wiring.md` for the full wiring diagram.

## 1. Set the DIP Switch (Device ID)

Before powering on, set the 5-position DIP switch to the desired Device ID (1–31). The switches are active LOW (ON = 0), and the Nano inverts them in software.

| Device ID | SW1 | SW2 | SW3 | SW4 | SW5 |
|-----------|-----|-----|-----|-----|-----|
| 01        | ON  | OFF | OFF | OFF | OFF |
| 02        | OFF | ON  | OFF | OFF | OFF |
| 03        | ON  | ON  | OFF | OFF | OFF |
| 10        | OFF | ON  | OFF | ON  | OFF |
| 16        | OFF | OFF | OFF | OFF | ON  |
| 31        | ON  | ON  | ON  | ON  | ON  |

All switches OFF = invalid (blinking red LED, device halts).

## 2. Flash the Arduino Nano

Connect the Nano via USB, then:

```bash
pio run -e nano -t upload
```

## 3. Flash the ESP32-CAM

Plug the ESP32-CAM into the ESP32-CAM-MB programmer board, connect via USB-C, then:

```bash
pio run -e esp32cam -t upload
```

If upload fails, hold the IO0 button on the ESP32-CAM-MB while pressing reset, then retry.

After flashing, remove the ESP32-CAM from the programmer board — it will be powered by the Nano via the MOSFET in the final assembly.

## 4. Set the DS3231 RTC Clock

The RTC needs to be set once. Upload this one-time sketch to the Nano (replace `src/nano/main.cpp` temporarily, then restore):

```cpp
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  Wire.begin();  // A4=SDA, A5=SCL (Nano defaults)

  if (!rtc.begin()) {
    Serial.println("RTC not found");
    while (1);
  }

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  DateTime now = rtc.now();
  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  Serial.print("RTC set to: ");
  Serial.println(buf);
}

void loop() {}
```

After setting the clock, re-flash the main Nano firmware:

```bash
pio run -e nano -t upload
```

The DS3231 retains time via its CR2032 backup battery.

## 5. Monitor Serial Output

To debug the Nano side (connect USB to Nano):

```bash
pio device monitor -e nano -b 115200
```

To debug the ESP32-CAM side (connect via programmer):

```bash
pio device monitor -e esp32cam -b 115200
```

Note: you can't monitor both simultaneously over USB since they share UART lines D0/D1 (Nano) and GPIO1/GPIO3 (ESP32-CAM). Disconnect the UART cross-wires when using USB serial for debugging.

## 6. End-to-End Test Checklist

1. Insert a FAT32 MicroSD card into the ESP32-CAM
2. Set DIP switches to a valid Device ID (e.g., 01)
3. Power on via MiniUPS or USB
4. Press wake button → LED turns solid green (system ready)
5. Show a QR code containing `0001` → LED turns white (1s)
6. Countdown: white/green blink accelerates over 3s
7. Photo capture: LED solid blue
8. Save complete: blue/green alternating blink for 3s → back to green
9. Show a QR with `ABCD` → LED red for 2s → back to green
10. Wait 60s with no scans → LED turns off (deep sleep)
11. Press wake button → LED green, device ready again

## 7. Verify SD Card Contents

Remove the MicroSD card and check on a computer:

```
/DEVICE01/
  scan_log.csv
  DEVICE01_USER0001.jpg
```

- `scan_log.csv` should have header `user_id,device_id,timestamp` and one row per scan
- JPEG files should be ~30KB VGA photos (640x480)
- Rescanning the same user overwrites the photo but appends a new CSV row

## 8. LED Status Reference

| LED State | Meaning |
|-----------|---------|
| Solid green | Ready to scan |
| Solid white (1s) | Valid QR code decoded |
| White/green accelerating blink (3s) | Photo countdown |
| Solid blue | Saving to SD card |
| Blue/green alternating blink (3s) | Save successful |
| Solid red (2s) | Invalid QR code |
| Solid red (3s) | Error (UART timeout, storage, camera) |
| Blinking red (continuous) | Invalid DIP switch config (all OFF) |
| Off | Deep sleep |

## 9. Field Deployment

1. Set the RTC clock (step 4) — only needed once
2. Set the DIP switch to the desired Device ID (step 1)
3. Flash both boards (steps 2–3)
4. Insert a fresh FAT32 MicroSD card into the ESP32-CAM
5. Wire everything per `wiring.md`
6. Connect MiniUPS + 18650 battery
7. Place in weatherproof enclosure (camera lens needs clear line of sight)
8. Press wake button — verify solid green LED

## 10. Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| Blinking red on boot | DIP switches all OFF (ID = 0) | Set at least one switch ON, reset |
| Solid red on boot | ESP32-CAM SD or camera error | Check MicroSD is inserted and FAT32, check camera ribbon cable |
| No LED at all | Nano not powered or MOSFET wiring issue | Check power connections, verify D8 wiring to MOSFET gate |
| LED green but no scan | UART wiring issue between boards | Check voltage divider on Nano TX→ESP RX, check ESP TX→Nano RX |
| RTC shows 0000-00-00 | RTC not set or not connected | Run the RTC set sketch (step 4), check I2C wiring A4/A5 |
