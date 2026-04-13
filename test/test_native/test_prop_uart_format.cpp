// Feature: wilderness-qr-checkpoint, Property 9: UART Protocol Message Format
//
// **Validates: Requirements 11.1, 11.2, 11.3, 11.4**
//
// For any valid command/response produced by formatting functions:
// (a) terminated by '\n'
// (b) printable ASCII + newline only
// (c) starts with "CMD:" or "RSP:"
// (d) ≤ 64 bytes total

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

// Both nano/uart_protocol.h and esp32cam/uart_protocol.h define
// UART_MAX_MSG_LEN as static const, so including both in one TU
// causes a redefinition error. We include nano's header for
// formatSaveCommand() and UART_MAX_MSG_LEN (value 64, same in both).
#include "nano/uart_protocol.h"

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

// Check that every character is printable ASCII (0x20–0x7E) or newline
static bool allPrintableOrNewline(const char* str, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        char c = str[i];
        if (c == '\n') continue;
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

// ── Property Tests ──────────────────────────────────────────────

RC_GTEST_PROP(UartFormatProperty9, FormatSaveCommandMessageFormat,
              ()) {
    // Generate valid inputs
    auto userId    = *genValidUserId();
    auto deviceId  = *genValidDeviceId();
    auto timestamp = *genValidTimestamp();

    char buf[128] = {};
    uint8_t written = formatSaveCommand(buf, sizeof(buf),
                                        userId.c_str(),
                                        deviceId.c_str(),
                                        timestamp.c_str());

    // formatSaveCommand must succeed for valid inputs
    RC_ASSERT(written > 0);

    size_t len = std::strlen(buf);

    // (a) Terminated by '\n'
    RC_ASSERT(len > 0);
    RC_ASSERT(buf[len - 1] == '\n');

    // (b) Printable ASCII + newline only
    RC_ASSERT(allPrintableOrNewline(buf, len));

    // (c) Starts with "CMD:"
    RC_ASSERT(std::strncmp(buf, "CMD:", 4) == 0);

    // (d) Total length ≤ 64 bytes (the message content, not counting
    //     the null terminator — UART_MAX_MSG_LEN is 64)
    RC_ASSERT(len <= UART_MAX_MSG_LEN);
}
