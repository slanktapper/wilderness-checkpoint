# Sourcing List — Wilderness QR Checkpoint Device

## Per Device

| Component | Description | Qty |
|-----------|-------------|-----|
| ESP32-CAM-MB Kit | AI-Thinker ESP32-CAM + USB-C programmer board | 1 |
| DS3231 RTC Module | Real-time clock with CR2032 coin cell backup | 1 |
| KY-016 RGB LED Module | Common cathode, built-in resistors, 3-pin color + GND | 1 |
| Momentary Push Button | Normally open, panel mount, for wake trigger | 1 |
| MiniUPS Module | Type-C UPS power management, 3.3V/5V output, charge + discharge | 1 |
| 18650 Lithium Battery | 3.7V, 3000mAh recommended, protected cell | 1 |
| 18650 Battery Holder | Single cell, solder tabs or wire leads | 1 |
| MicroSD Card | FAT32, 1GB+ (4GB is plenty), Class 10 | 1 |
| Weatherproof Enclosure | IP65+, large enough for all components, clear window for camera | 1 |
| Dupont Jumper Wires | Female-to-female, assorted lengths | ~10 |
| CR2032 Coin Cell | Backup battery for DS3231 (usually included with module) | 1 |

## Development Only (shared across devices)

| Component | Description | Qty |
|-----------|-------------|-----|
| USB-C Cable | For flashing via ESP32-CAM-MB programmer | 1 |
| MicroSD Card Reader | USB adapter for reading SD card on computer | 1 |
| Breadboard | For prototyping wiring before final assembly | 1 |

## Notes

- The ESP32-CAM-MB kit includes both the ESP32-CAM board and the USB-C programmer. The programmer is only needed during development — only the ESP32-CAM board is deployed.
- A 3000mAh 18650 cell provides comfortable margin for an 8-hour event with up to 1000 scans.
- The MicroSD card only needs ~30MB for 1000 photos + CSV log. A 4GB card is more than enough.
- The DS3231 module usually ships with a CR2032 already installed.
- For the enclosure, ensure the camera lens has a clear line of sight — either a transparent window or a cutout with a lens cover.

## Links
| Component | Ordered | Required | Cost Each | Cost Total | Link |
|-----------|---------|----------|-----------|------------|------|
| ESP32-CAM | 10 | 10 | $9.85 | $98.50 | [AliExpress](https://www.aliexpress.com/item/1005010579171124.html) |
| KY-016 RGB LED Module | 10 | 10 | $0.99 | $9.90 | [AliExpress](https://www.aliexpress.com/item/1005008186943628.html) |
| MiniUPS Module | 10 | 10 | $3.44 | $34.40 | [AliExpress](https://www.aliexpress.com/item/1005009305823865.html) |
| DS3231 RTC Module | 10 | 10 | $4.34 | $43.40 | [AliExpress](https://www.aliexpress.com/item/1005010686237643.html) |
| 18650 Lithium Battery | 10 | 10 | $2.28 | $22.80 | [AliExpress](https://www.aliexpress.com/item/1005011923380377.html) |
| 18650 Battery Holder | 10 | 10 | $0.96 | $9.60 | [AliExpress](https://www.aliexpress.com/item/1005009467208586.html) |
| MicroSD Card | 10 | 10 | $7.25 | $72.50 | [Amazon](https://www.amazon.ca/Cloudisk-10Pack-Memory-Adapter-TF-Black-10/dp/B0D2TCM9Z4/) |
| Weatherproof Enclosure | 2 | 10 | $16.30 | $163.00 | [Amazon](https://www.amazon.ca/Zulkit-Waterproof-Electrical-Electronic-100X68X50mm/dp/B0G2KXX5Z4) |
| Momentary Switch | 10 | 10 | $2.12 | $21.20 | [AliExpress](https://www.aliexpress.com/item/1005008777117179.html) |
| **Costs** | | | **$47.53** | **$475.30** | |
