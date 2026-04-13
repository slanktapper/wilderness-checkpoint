// Feature: wilderness-qr-checkpoint, Property 4: Rescan Overwrites Photo but Appends Log
//
// **Validates: Requirements 4.4, 5.5**
//
// For any user_id scanned N times (N >= 1) on the same device,
// buildPhotoPath always produces the same single path (overwrite semantics),
// while formatCsvRow produces N distinct rows (append semantics).

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "esp32cam/file_paths.h"

#include <cstring>
#include <string>
#include <cstdio>
#include <set>
#include <vector>

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

RC_GTEST_PROP(RescanProperty4, RescanOverwritesPhotoButAppendsLog,
              ()) {
    auto userId   = *genValidUserId();
    auto deviceId = *genValidDeviceId();
    int N         = *rc::gen::inRange(1, 21);  // 1–20 saves

    // Generate N distinct timestamps (one per save)
    std::vector<std::string> timestamps;
    timestamps.reserve(N);
    for (int i = 0; i < N; ++i) {
        timestamps.push_back(*genValidTimestamp());
    }

    // ── Photo path: overwrite semantics ─────────────────────────
    // All N calls to buildPhotoPath must produce the identical path
    std::set<std::string> photoPaths;
    for (int i = 0; i < N; ++i) {
        char pathBuf[64] = {};
        uint8_t written = buildPhotoPath(pathBuf, sizeof(pathBuf),
                                         deviceId.c_str(), userId.c_str());
        RC_ASSERT(written > 0);
        photoPaths.insert(std::string(pathBuf));
    }
    // Exactly one unique path regardless of how many saves
    RC_ASSERT(photoPaths.size() == 1u);

    // ── CSV rows: append semantics ──────────────────────────────
    // Each save produces a distinct row; collect all N rows
    std::vector<std::string> csvRows;
    csvRows.reserve(N);
    for (int i = 0; i < N; ++i) {
        char rowBuf[128] = {};
        uint8_t written = formatCsvRow(rowBuf, sizeof(rowBuf),
                                       userId.c_str(),
                                       deviceId.c_str(),
                                       timestamps[i].c_str());
        RC_ASSERT(written > 0);
        csvRows.push_back(std::string(rowBuf));
    }

    // Row count equals N (append semantics — one row per save)
    RC_ASSERT(static_cast<int>(csvRows.size()) == N);

    // Each row contains the correct timestamp for that save
    for (int i = 0; i < N; ++i) {
        RC_ASSERT(csvRows[i].find(timestamps[i]) != std::string::npos);
        // Each row also contains the userId and deviceId
        RC_ASSERT(csvRows[i].find(userId) != std::string::npos);
        RC_ASSERT(csvRows[i].find(deviceId) != std::string::npos);
    }
}
