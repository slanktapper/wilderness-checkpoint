#ifndef SD_STORAGE_H
#define SD_STORAGE_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#include <cstddef>
#endif

#ifdef ARDUINO

// Verify SD card is present and writable (used during boot check).
// Mounts, checks, then unmounts.
// Returns true if SD is OK.
bool sdVerify();

// Initialize SD card and create device directory if needed.
// deviceId: 2-digit zero-padded string (e.g., "01").
// Returns true on success.
bool sdInit(const char* deviceId);

// Save a JPEG photo to /DEVICEXX/DEVICEXX_USERYYYY.jpg.
// Overwrites if file exists.
// Returns true on success.
bool sdSavePhoto(const char* deviceId, const char* userId,
                 const uint8_t* jpegData, size_t jpegSize);

// Append a row to /DEVICEXX/scan_log.csv.
// Creates file with header if it doesn't exist.
// Returns true on success.
bool sdAppendLog(const char* deviceId, const char* userId,
                 const char* timestamp);

// Unmount SD card (required before camera reinit).
void sdDeinit();

#endif // ARDUINO

#endif // SD_STORAGE_H
