#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#include <cstdio>
#endif

// ── Pure logic (compiles on native host and Arduino) ────────────

// Format date/time components as ISO 8601 string: "YYYY-MM-DDTHH:MM:SS"
// Buffer must be at least 20 bytes (19 chars + null terminator).
// Returns number of characters written (19 on success), or 0 if buffer too small.
uint8_t formatTimestamp(char* buf, uint8_t bufSize,
                        uint16_t year, uint8_t month, uint8_t day,
                        uint8_t hour, uint8_t minute, uint8_t second);

// Returns a pointer to the fallback timestamp used when RTC is unavailable.
// Value: "0000-00-00T00:00:00"
const char* rtcFallbackTimestamp();

#ifdef ARDUINO
// Initialize DS3231 RTC over I2C (A4/SDA, A5/SCL).
// Returns true if RTC responds, false otherwise.
bool rtcInit();

// Read current time from DS3231 and format as ISO 8601 string.
// Writes to buffer (must be at least 20 bytes).
// Returns true on success, false if RTC read fails.
bool rtcGetTimestamp(char* buffer, uint8_t bufferSize);
#endif

#endif // RTC_MANAGER_H
