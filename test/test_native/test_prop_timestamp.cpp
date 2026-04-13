// Feature: wilderness-qr-checkpoint, Property 7: ISO 8601 Timestamp Formatting
//
// **Validates: Requirements 7.3**
//
// For any valid DateTime (year 2000–2099, valid month/day/hour/minute/second),
// the formatted timestamp string shall match the pattern YYYY-MM-DDTHH:MM:SS
// with exactly 19 characters, zero-padded fields, and the literal T separator.

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "nano/rtc_manager.h"

#include <cstring>
#include <cstdlib>
#include <string>

// ── Property Tests ──────────────────────────────────────────────

RC_GTEST_PROP(TimestampProperty7, Iso8601FormatCorrectness,
              ()) {
    // Generate random valid date/time components
    auto year   = *rc::gen::inRange(2000, 2100);   // 2000–2099
    auto month  = *rc::gen::inRange(1, 13);         // 1–12
    auto day    = *rc::gen::inRange(1, 29);          // 1–28 (safe for all months)
    auto hour   = *rc::gen::inRange(0, 24);          // 0–23
    auto minute = *rc::gen::inRange(0, 60);          // 0–59
    auto second = *rc::gen::inRange(0, 60);          // 0–59

    char buf[20] = {};
    uint8_t written = formatTimestamp(buf, sizeof(buf),
                                      static_cast<uint16_t>(year),
                                      static_cast<uint8_t>(month),
                                      static_cast<uint8_t>(day),
                                      static_cast<uint8_t>(hour),
                                      static_cast<uint8_t>(minute),
                                      static_cast<uint8_t>(second));

    // Must succeed (return 19)
    RC_ASSERT(written == 19);

    // Output is exactly 19 characters
    RC_ASSERT(std::strlen(buf) == 19);

    // Verify separator positions: '-' at 4, '-' at 7, 'T' at 10, ':' at 13, ':' at 16
    RC_ASSERT(buf[4]  == '-');
    RC_ASSERT(buf[7]  == '-');
    RC_ASSERT(buf[10] == 'T');
    RC_ASSERT(buf[13] == ':');
    RC_ASSERT(buf[16] == ':');

    // All other positions must be ASCII digits
    int digitPositions[] = {0, 1, 2, 3, 5, 6, 8, 9, 11, 12, 14, 15, 17, 18};
    for (int pos : digitPositions) {
        RC_ASSERT(buf[pos] >= '0' && buf[pos] <= '9');
    }

    // Parse fields back and verify they match input values
    char yearStr[5]   = {buf[0], buf[1], buf[2], buf[3], '\0'};
    char monthStr[3]  = {buf[5], buf[6], '\0'};
    char dayStr[3]    = {buf[8], buf[9], '\0'};
    char hourStr[3]   = {buf[11], buf[12], '\0'};
    char minuteStr[3] = {buf[14], buf[15], '\0'};
    char secondStr[3] = {buf[17], buf[18], '\0'};

    RC_ASSERT(std::atoi(yearStr)   == year);
    RC_ASSERT(std::atoi(monthStr)  == month);
    RC_ASSERT(std::atoi(dayStr)    == day);
    RC_ASSERT(std::atoi(hourStr)   == hour);
    RC_ASSERT(std::atoi(minuteStr) == minute);
    RC_ASSERT(std::atoi(secondStr) == second);
}
