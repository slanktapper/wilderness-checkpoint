#include "file_paths.h"

#include <string.h>
#include <stdio.h>

// ── Pure logic (compiles on native host and Arduino) ────────────

uint8_t buildPhotoPath(char* buf, uint8_t bufSize,
                       const char* deviceId, const char* userId) {
    if (buf == nullptr || deviceId == nullptr || userId == nullptr) {
        return 0;
    }

    // Format: /DEVICEXX/DEVICEXX_USERYYYY.jpg
    // Example: /DEVICE07/DEVICE07_USER0042.jpg  (31 chars + null)
    int written = snprintf(buf, bufSize,
                           "/DEVICE%s/DEVICE%s_USER%s.jpg",
                           deviceId, deviceId, userId);

    if (written < 0 || written >= bufSize) {
        if (bufSize > 0) {
            buf[0] = '\0';
        }
        return 0;
    }

    return (uint8_t)written;
}

uint8_t buildLogPath(char* buf, uint8_t bufSize, const char* deviceId) {
    if (buf == nullptr || deviceId == nullptr) {
        return 0;
    }

    // Format: /DEVICEXX/scan_log.csv
    // Example: /DEVICE07/scan_log.csv  (22 chars + null)
    int written = snprintf(buf, bufSize,
                           "/DEVICE%s/scan_log.csv",
                           deviceId);

    if (written < 0 || written >= bufSize) {
        if (bufSize > 0) {
            buf[0] = '\0';
        }
        return 0;
    }

    return (uint8_t)written;
}

uint8_t buildDirPath(char* buf, uint8_t bufSize, const char* deviceId) {
    if (buf == nullptr || deviceId == nullptr) {
        return 0;
    }

    // Format: /DEVICEXX
    // Example: /DEVICE07  (9 chars + null)
    int written = snprintf(buf, bufSize,
                           "/DEVICE%s",
                           deviceId);

    if (written < 0 || written >= bufSize) {
        if (bufSize > 0) {
            buf[0] = '\0';
        }
        return 0;
    }

    return (uint8_t)written;
}

uint8_t formatCsvRow(char* buf, uint8_t bufSize,
                     const char* userId, const char* deviceId,
                     const char* timestamp) {
    if (buf == nullptr || userId == nullptr ||
        deviceId == nullptr || timestamp == nullptr) {
        return 0;
    }

    // Format: userId,deviceId,timestamp\n
    // Example: 0042,07,2025-06-15T14:30:22\n
    int written = snprintf(buf, bufSize, "%s,%s,%s\n",
                           userId, deviceId, timestamp);

    if (written < 0 || written >= bufSize) {
        if (bufSize > 0) {
            buf[0] = '\0';
        }
        return 0;
    }

    return (uint8_t)written;
}

const char* csvHeader() {
    return "user_id,device_id,timestamp\n";
}
