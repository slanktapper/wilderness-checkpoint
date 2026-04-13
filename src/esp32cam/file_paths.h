#ifndef ESP_FILE_PATHS_H
#define ESP_FILE_PATHS_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#include <cstddef>
#endif

// Build photo file path: /DEVICEXX/DEVICEXX_USERYYYY.jpg
// Returns number of characters written (excluding null terminator),
// or 0 if buffer too small.
uint8_t buildPhotoPath(char* buf, uint8_t bufSize,
                       const char* deviceId, const char* userId);

// Build scan log path: /DEVICEXX/scan_log.csv
// Returns number of characters written (excluding null terminator),
// or 0 if buffer too small.
uint8_t buildLogPath(char* buf, uint8_t bufSize, const char* deviceId);

// Build device directory path: /DEVICEXX
// Returns number of characters written (excluding null terminator),
// or 0 if buffer too small.
uint8_t buildDirPath(char* buf, uint8_t bufSize, const char* deviceId);

// Format a CSV row: userId,deviceId,timestamp\n
// Returns number of characters written (excluding null terminator),
// or 0 if buffer too small.
uint8_t formatCsvRow(char* buf, uint8_t bufSize,
                     const char* userId, const char* deviceId,
                     const char* timestamp);

// Returns a pointer to the static CSV header string:
// "user_id,device_id,timestamp\n"
const char* csvHeader();

#endif // ESP_FILE_PATHS_H
