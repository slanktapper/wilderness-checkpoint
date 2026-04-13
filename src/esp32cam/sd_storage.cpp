#ifdef ARDUINO

#include "sd_storage.h"
#include "file_paths.h"

#include <FS.h>
#include <SD_MMC.h>

// ── Path buffer sizes ───────────────────────────────────────────
// Longest path: /DEVICE31/DEVICE31_USER1000.jpg = 31 chars + null
static const uint8_t PATH_BUF_SIZE = 48;
// Longest CSV row: 1000,31,2099-12-31T23:59:59\n = ~30 chars + null
static const uint8_t ROW_BUF_SIZE = 64;

// ── Public API ──────────────────────────────────────────────────

bool sdVerify() {
    if (!SD_MMC.begin()) {
        return false;
    }

    // Check card is present and has space
    uint64_t totalBytes = SD_MMC.totalBytes();
    if (totalBytes == 0) {
        SD_MMC.end();
        return false;
    }

    SD_MMC.end();
    return true;
}

bool sdInit(const char* deviceId) {
    if (deviceId == nullptr) {
        return false;
    }

    if (!SD_MMC.begin()) {
        return false;
    }

    // Create device directory if it doesn't exist
    char dirPath[PATH_BUF_SIZE];
    if (buildDirPath(dirPath, sizeof(dirPath), deviceId) == 0) {
        SD_MMC.end();
        return false;
    }

    if (!SD_MMC.exists(dirPath)) {
        if (!SD_MMC.mkdir(dirPath)) {
            SD_MMC.end();
            return false;
        }
    }

    return true;
}

bool sdSavePhoto(const char* deviceId, const char* userId,
                 const uint8_t* jpegData, size_t jpegSize) {
    if (deviceId == nullptr || userId == nullptr ||
        jpegData == nullptr || jpegSize == 0) {
        return false;
    }

    char photoPath[PATH_BUF_SIZE];
    if (buildPhotoPath(photoPath, sizeof(photoPath), deviceId, userId) == 0) {
        return false;
    }

    // FILE_WRITE overwrites existing file
    File file = SD_MMC.open(photoPath, FILE_WRITE);
    if (!file) {
        return false;
    }

    size_t written = file.write(jpegData, jpegSize);
    file.close();

    return (written == jpegSize);
}

bool sdAppendLog(const char* deviceId, const char* userId,
                 const char* timestamp) {
    if (deviceId == nullptr || userId == nullptr || timestamp == nullptr) {
        return false;
    }

    char logPath[PATH_BUF_SIZE];
    if (buildLogPath(logPath, sizeof(logPath), deviceId) == 0) {
        return false;
    }

    // If file doesn't exist, write header first
    bool needsHeader = !SD_MMC.exists(logPath);

    File file = SD_MMC.open(logPath, FILE_APPEND);
    if (!file) {
        return false;
    }

    if (needsHeader) {
        const char* header = csvHeader();
        size_t headerLen = strlen(header);
        if (file.write((const uint8_t*)header, headerLen) != headerLen) {
            file.close();
            return false;
        }
    }

    // Format and write the CSV row
    char row[ROW_BUF_SIZE];
    uint8_t rowLen = formatCsvRow(row, sizeof(row),
                                  userId, deviceId, timestamp);
    if (rowLen == 0) {
        file.close();
        return false;
    }

    size_t written = file.write((const uint8_t*)row, rowLen);
    file.close();

    return (written == rowLen);
}

void sdDeinit() {
    SD_MMC.end();
}

#endif // ARDUINO
