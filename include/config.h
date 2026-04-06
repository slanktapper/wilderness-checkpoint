#ifndef CONFIG_H
#define CONFIG_H

// ─── Device Identity ────────────────────────────────────────────────────────
#define DEVICE_ID        "01"          // 2-digit zero-padded device identifier

// ─── Capacity ───────────────────────────────────────────────────────────────
#define MAX_USERS        1000          // Maximum unique users per event

// ─── Photo Settings ─────────────────────────────────────────────────────────
#define PHOTO_QUALITY    12            // JPEG quality (10-63, lower = better)
#define VGA_WIDTH        640           // Photo width in pixels
#define VGA_HEIGHT       480           // Photo height in pixels
#define MAX_PHOTO_SIZE   50000         // 50KB max per photo

// ─── Timing Constants (milliseconds) ────────────────────────────────────────
#define INACTIVITY_MS    60000         // 60s inactivity timeout before sleep
#define COUNTDOWN_MS     3000          // 3-second countdown before capture
#define BLINK_START_MS   500           // Initial blink interval (slow)
#define BLINK_END_MS     100           // Final blink interval (fast)
#define ACCEPTED_MS      1000          // White solid duration after QR accept
#define CAPTURE_FLASH_MS 200           // White solid during capture
#define SAVE_MIN_MS      1500          // Minimum blue LED during save
#define SUCCESS_MS       3000          // Blue/green alternating duration
#define SUCCESS_BLINK_MS 100           // Blue/green alternating interval
#define ERROR_MS         5000          // Red solid for SD errors
#define INVALID_MS       2000          // Red solid for invalid QR

// ─── GPIO Pin Assignments ───────────────────────────────────────────────────
#define PIN_BUTTON       0             // Wake button (ext0 deep sleep source)
#define PIN_SD_D0        2             // SD_MMC data line (1-bit mode)
#define PIN_LED_RED      12            // KY-016 Red (shared with SD_MMC)
#define PIN_LED_GREEN    13            // KY-016 Green (shared with SD_MMC)
#define PIN_LED_BLUE     33            // KY-016 Blue (onboard LED, inverted logic)
#define PIN_I2C_SCL      14            // I2C SCL for DS3231 RTC / SD_MMC CLK
#define PIN_I2C_SDA      15            // I2C SDA for DS3231 RTC / SD_MMC CMD

#endif // CONFIG_H
