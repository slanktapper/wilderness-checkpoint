#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <string>
#include <cstdint>
typedef std::string String;
#endif

namespace Storage {

// ─── CSV Record ─────────────────────────────────────────────────────────────

struct CsvRecord {
  String userId;
  String deviceId;
  String timestamp;
};

// ─── SD Card I/O (stubs — implemented in Task 4.5) ─────────────────────────

bool init();
void deinit();
bool isReady();
bool savePhoto(const char* deviceId, const char* userId,
               const uint8_t* data, size_t length);
bool appendScanLog(const char* deviceId, const char* userId,
                   const char* timestamp);
bool ensureDirectory(const char* deviceId);

// ─── Pure Logic Helpers ─────────────────────────────────────────────────────

/// Build photo file path: /DEVICE{XX}/DEVICE{XX}_USER{XXXX}.jpg
String buildPhotoPath(const char* deviceId, const char* userId);

/// Build scan log file path: /DEVICE{XX}/scan_log.csv
String buildLogPath(const char* deviceId);

/// Format a CSV row: user_id,device_id,timestamp (no trailing newline)
String formatCsvRow(const char* userId, const char* deviceId,
                    const char* timestamp);

/// Parse a CSV row back into its fields
CsvRecord parseCsvRow(const String& csvLine);

} // namespace Storage

#endif // STORAGE_MANAGER_H
