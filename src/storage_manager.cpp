#include "storage_manager.h"

#ifdef ARDUINO
#include <SD_MMC.h>
#include <FS.h>
#endif

namespace Storage {

// ─── SD Card I/O ────────────────────────────────────────────────────────────

#ifdef ARDUINO

bool init() {
  // Mount SD_MMC in 1-bit mode (second arg = true)
  return SD_MMC.begin("/sdcard", true);
}

void deinit() {
  SD_MMC.end();
}

bool isReady() {
  uint8_t cardType = SD_MMC.cardType();
  return cardType != CARD_NONE && cardType != CARD_UNKNOWN;
}

bool savePhoto(const char* deviceId, const char* userId,
               const uint8_t* data, size_t length) {
  if (!ensureDirectory(deviceId)) {
    return false;
  }

  String path = buildPhotoPath(deviceId, userId);
  File file = SD_MMC.open(path.c_str(), FILE_WRITE);
  if (!file) {
    return false;
  }

  size_t written = file.write(data, length);
  file.close();
  return written == length;
}

bool appendScanLog(const char* deviceId, const char* userId,
                   const char* timestamp) {
  if (!ensureDirectory(deviceId)) {
    return false;
  }

  String path = buildLogPath(deviceId);
  bool fileExists = SD_MMC.exists(path.c_str());

  File file = SD_MMC.open(path.c_str(), FILE_APPEND);
  if (!file) {
    return false;
  }

  // Write CSV header if this is a new file
  if (!fileExists) {
    file.println("user_id,device_id,timestamp");
  }

  String row = formatCsvRow(userId, deviceId, timestamp);
  file.println(row.c_str());
  file.close();
  return true;
}

bool ensureDirectory(const char* deviceId) {
  String dirPath = String("/DEVICE") + deviceId;
  if (SD_MMC.exists(dirPath.c_str())) {
    return true;
  }
  return SD_MMC.mkdir(dirPath.c_str());
}

#else
// ─── Native stubs (no SD hardware) ─────────────────────────────────────────

bool init() {
  return false;
}

void deinit() {
}

bool isReady() {
  return false;
}

bool savePhoto(const char* /*deviceId*/, const char* /*userId*/,
               const uint8_t* /*data*/, size_t /*length*/) {
  return false;
}

bool appendScanLog(const char* /*deviceId*/, const char* /*userId*/,
                   const char* /*timestamp*/) {
  return false;
}

bool ensureDirectory(const char* /*deviceId*/) {
  return false;
}

#endif // ARDUINO

// ─── Pure Logic Helpers ─────────────────────────────────────────────────────

String buildPhotoPath(const char* deviceId, const char* userId) {
  // /DEVICE{XX}/DEVICE{XX}_USER{XXXX}.jpg
  return String("/DEVICE") + deviceId +
         "/DEVICE" + deviceId +
         "_USER" + userId + ".jpg";
}

String buildLogPath(const char* deviceId) {
  // /DEVICE{XX}/scan_log.csv
  return String("/DEVICE") + deviceId + "/scan_log.csv";
}

String formatCsvRow(const char* userId, const char* deviceId,
                    const char* timestamp) {
  // user_id,device_id,timestamp (no trailing newline)
  return String(userId) + "," + deviceId + "," + timestamp;
}

CsvRecord parseCsvRow(const String& csvLine) {
  CsvRecord record;

#ifdef ARDUINO
  // Arduino String uses indexOf() and substring()
  int first = csvLine.indexOf(',');
  if (first < 0) {
    record.userId = csvLine;
    return record;
  }

  int second = csvLine.indexOf(',', first + 1);
  if (second < 0) {
    record.userId   = csvLine.substring(0, first);
    record.deviceId = csvLine.substring(first + 1);
    return record;
  }

  record.userId   = csvLine.substring(0, first);
  record.deviceId = csvLine.substring(first + 1, second);
  record.timestamp = csvLine.substring(second + 1);
#else
  // std::string uses find() and substr()
  size_t first = csvLine.find(',');
  if (first == String::npos) {
    record.userId = csvLine;
    return record;
  }

  size_t second = csvLine.find(',', first + 1);
  if (second == String::npos) {
    record.userId   = csvLine.substr(0, first);
    record.deviceId = csvLine.substr(first + 1);
    return record;
  }

  record.userId   = csvLine.substr(0, first);
  record.deviceId = csvLine.substr(first + 1, second - first - 1);
  record.timestamp = csvLine.substr(second + 1);
#endif

  return record;
}

} // namespace Storage
