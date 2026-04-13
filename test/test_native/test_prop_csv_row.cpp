// Feature: wilderness-qr-checkpoint, Property 5: CSV Row Correctness
//
// **Validates: Requirements 5.1, 5.3, 5.4**
//
// For any valid save parameters (userId, deviceId, timestamp),
// the formatted CSV row contains the exact userId, deviceId, and
// timestamp values in correct column order with no truncation.

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "esp32cam/file_paths.h"

#include <cstring>
#include <string>
#include <cstdio>

// ── Helpers ─────────────────────────────────────────────────────

// Generate a valid userId string: 4-digit zero-padded, value 0001–1000
static rc::Gen<std::string> genValidUserId() {
    return rc::gen::map(rc::gen::inRange(1, 1001), [](int v) {
        char buf[5];
        std::snprintf(buf, sizeof(buf), "%04d", v);
        return std::string(buf);
    });
}

// Generate a valid deviceId string: 2-digit zero-padded, value 01–31
static rc::Gen<std::string> genValidDeviceId() {
    return rc::gen::map(rc::gen::inRange(1, 32), [](int v) {
        char buf[3];
        std::snprintf(buf, sizeof(buf), "%02d", v);
        return std::string(buf);
    });
}

// Generate a valid ISO 8601 timestamp: YYYY-MM-DDTHH:MM:SS
static rc::Gen<std::string> genValidTimestamp() {
    return rc::gen::apply(
        [](int year, int month, int day, int hour, int minute, int second) {
            char buf[20];
            std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
                          year, month, day, hour, minute, second);
            return std::string(buf);
        },
        rc::gen::inRange(2000, 2100),  // year
        rc::gen::inRange(1, 13),       // month
        rc::gen::inRange(1, 29),       // day (safe for all months)
        rc::gen::inRange(0, 24),       // hour
        rc::gen::inRange(0, 60),       // minute
        rc::gen::inRange(0, 60)        // second
    );
}

// ── Property Tests ──────────────────────────────────────────────

RC_GTEST_PROP(CsvRowProperty5, CsvRowCorrectness,
              ()) {
    auto userId    = *genValidUserId();
    auto deviceId  = *genValidDeviceId();
    auto timestamp = *genValidTimestamp();

    char buf[128] = {};
    uint8_t written = formatCsvRow(buf, sizeof(buf),
                                   userId.c_str(),
                                   deviceId.c_str(),
                                   timestamp.c_str());

    // formatCsvRow must succeed for valid inputs
    RC_ASSERT(written > 0);

    std::string row(buf);

    // Row ends with '\n'
    RC_ASSERT(!row.empty());
    RC_ASSERT(row.back() == '\n');

    // Strip trailing newline for column parsing
    std::string content = row.substr(0, row.size() - 1);

    // Split by commas — expect exactly 3 columns
    size_t comma1 = content.find(',');
    RC_ASSERT(comma1 != std::string::npos);

    size_t comma2 = content.find(',', comma1 + 1);
    RC_ASSERT(comma2 != std::string::npos);

    // No additional commas (exactly 3 fields)
    RC_ASSERT(content.find(',', comma2 + 1) == std::string::npos);

    std::string col1 = content.substr(0, comma1);
    std::string col2 = content.substr(comma1 + 1, comma2 - comma1 - 1);
    std::string col3 = content.substr(comma2 + 1);

    // Correct column order: userId, deviceId, timestamp
    RC_ASSERT(col1 == userId);
    RC_ASSERT(col2 == deviceId);
    RC_ASSERT(col3 == timestamp);

    // No truncation: written length matches expected
    // expected = userId(4) + ',' + deviceId(2) + ',' + timestamp(19) + '\n' = 27
    size_t expectedLen = userId.size() + 1 + deviceId.size() + 1 + timestamp.size() + 1;
    RC_ASSERT(written == expectedLen);
    RC_ASSERT(std::strlen(buf) == expectedLen);
}
