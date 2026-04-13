// Feature: wilderness-qr-checkpoint, Property 3: User ID Validation
//
// **Validates: Requirements 3.2, 3.3**
//
// `isValidUserId` returns true iff string is exactly 4 ASCII digits
// with numeric value 1–1000. All other strings shall be rejected.

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "esp32cam/uart_protocol.h"

#include <cstdio>
#include <string>

// ── Helpers ─────────────────────────────────────────────────────

// Generate a valid userId: 4-digit zero-padded string, value 1–1000
static rc::Gen<std::string> genValidUserId() {
    return rc::gen::map(rc::gen::inRange(1, 1001), [](int v) {
        char buf[5];
        std::snprintf(buf, sizeof(buf), "%04d", v);
        return std::string(buf);
    });
}

// Returns true if a string is a valid user ID (mirror of the spec logic,
// used only for filtering arbitrary strings in the rejection property).
static bool isValidUserIdSpec(const std::string& s) {
    if (s.size() != 4) return false;
    for (char c : s) {
        if (c < '0' || c > '9') return false;
    }
    int val = (s[0] - '0') * 1000
            + (s[1] - '0') * 100
            + (s[2] - '0') * 10
            + (s[3] - '0');
    return val >= 1 && val <= 1000;
}

// ── Property Tests ──────────────────────────────────────────────

// Property 3a: For any valid userId (4-digit zero-padded, value 1–1000),
// isValidUserId returns true.
RC_GTEST_PROP(UserIdValidationProperty3, ValidUserIdIsAccepted,
              ()) {
    auto userId = *genValidUserId();

    RC_ASSERT(isValidUserId(userId.c_str()) == true);
}

// Property 3b: For any arbitrary string that is NOT a valid userId,
// isValidUserId returns false.
RC_GTEST_PROP(UserIdValidationProperty3, InvalidUserIdIsRejected,
              ()) {
    auto input = *rc::gen::arbitrary<std::string>();

    // Discard strings that happen to be valid user IDs
    RC_PRE(!isValidUserIdSpec(input));

    RC_ASSERT(isValidUserId(input.c_str()) == false);
}
