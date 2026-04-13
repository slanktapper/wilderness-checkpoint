// Feature: wilderness-qr-checkpoint, Property 6: File Path Construction
//
// **Validates: Requirements 5.2, 6.2, 6.3, 9.5**
//
// For any device_id 1–31 and user_id 0001–1000:
// (a) Photo path matches /DEVICEXX/DEVICEXX_USERYYYY.jpg
// (b) Log path matches /DEVICEXX/scan_log.csv
// (c) Dir path matches /DEVICEXX
// (d) Photo and log paths start with the dir path (all within /DEVICEXX/)

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "esp32cam/file_paths.h"

#include <cstring>
#include <string>
#include <cstdio>
#include <regex>

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

// ── Property Tests ──────────────────────────────────────────────

RC_GTEST_PROP(FilePathProperty6, FilePathConstruction,
              ()) {
    auto deviceId = *genValidDeviceId();
    auto userId   = *genValidUserId();

    char photoBuf[64] = {};
    char logBuf[64]   = {};
    char dirBuf[64]   = {};

    uint8_t photoLen = buildPhotoPath(photoBuf, sizeof(photoBuf),
                                      deviceId.c_str(), userId.c_str());
    uint8_t logLen   = buildLogPath(logBuf, sizeof(logBuf),
                                    deviceId.c_str());
    uint8_t dirLen   = buildDirPath(dirBuf, sizeof(dirBuf),
                                    deviceId.c_str());

    // All functions must succeed for valid inputs
    RC_ASSERT(photoLen > 0);
    RC_ASSERT(logLen > 0);
    RC_ASSERT(dirLen > 0);

    std::string photoPath(photoBuf);
    std::string logPath(logBuf);
    std::string dirPath(dirBuf);

    // (a) Photo path matches /DEVICEXX/DEVICEXX_USERYYYY.jpg
    std::string expectedPhoto = "/DEVICE" + deviceId + "/DEVICE" + deviceId + "_USER" + userId + ".jpg";
    RC_ASSERT(photoPath == expectedPhoto);

    // Verify photo path matches the regex pattern
    std::regex photoRegex(R"(^/DEVICE\d{2}/DEVICE\d{2}_USER\d{4}\.jpg$)");
    RC_ASSERT(std::regex_match(photoPath, photoRegex));

    // (b) Log path matches /DEVICEXX/scan_log.csv
    std::string expectedLog = "/DEVICE" + deviceId + "/scan_log.csv";
    RC_ASSERT(logPath == expectedLog);

    // Verify log path matches the regex pattern
    std::regex logRegex(R"(^/DEVICE\d{2}/scan_log\.csv$)");
    RC_ASSERT(std::regex_match(logPath, logRegex));

    // (c) Dir path matches /DEVICEXX
    std::string expectedDir = "/DEVICE" + deviceId;
    RC_ASSERT(dirPath == expectedDir);

    // Verify dir path matches the regex pattern
    std::regex dirRegex(R"(^/DEVICE\d{2}$)");
    RC_ASSERT(std::regex_match(dirPath, dirRegex));

    // (d) Photo and log paths start with the dir path (all within /DEVICEXX/)
    RC_ASSERT(photoPath.substr(0, dirPath.size()) == dirPath);
    RC_ASSERT(photoPath[dirPath.size()] == '/');

    RC_ASSERT(logPath.substr(0, dirPath.size()) == dirPath);
    RC_ASSERT(logPath[dirPath.size()] == '/');
}
