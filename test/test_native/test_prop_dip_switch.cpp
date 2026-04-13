// Feature: wilderness-qr-checkpoint, Property 8: DIP Switch to Device ID Conversion
//
// **Validates: Requirements 9.2**
//
// For any 5-bit input value (0–31), device_id equals the numeric value.
// Value 0 (all switches OFF) is invalid. Values 1–31 are valid device IDs.
// Values > 31 are masked to 5 bits.

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "nano/config_reader.h"

#include <cstring>
#include <cstdio>
#include <string>

// ── Property Tests ──────────────────────────────────────────────

// Property 8a: For any 5-bit input (0-31), configReadDeviceId returns
// the input value directly.
RC_GTEST_PROP(DipSwitchProperty8, FiveBitInputEqualsDeviceId,
              ()) {
    auto pinValues = *rc::gen::inRange(0, 32);

    uint8_t deviceId = configReadDeviceId(static_cast<uint8_t>(pinValues));

    RC_ASSERT(deviceId == static_cast<uint8_t>(pinValues));
}

// Property 8b: Value 0 is treated as invalid (returned as 0),
// values 1-31 are valid device IDs.
RC_GTEST_PROP(DipSwitchProperty8, ZeroIsInvalidNonzeroIsValid,
              ()) {
    auto pinValues = *rc::gen::inRange(0, 32);

    uint8_t deviceId = configReadDeviceId(static_cast<uint8_t>(pinValues));

    if (pinValues == 0) {
        // 0 means all switches OFF = invalid
        RC_ASSERT(deviceId == 0);
    } else {
        // 1-31 are valid device IDs
        RC_ASSERT(deviceId >= 1);
        RC_ASSERT(deviceId <= 31);
    }
}

// Property 8c: Values > 31 are masked to 5 bits.
RC_GTEST_PROP(DipSwitchProperty8, ValuesAbove31AreMaskedTo5Bits,
              ()) {
    auto pinValues = *rc::gen::inRange(32, 256);

    uint8_t deviceId = configReadDeviceId(static_cast<uint8_t>(pinValues));

    RC_ASSERT(deviceId == (pinValues & 0x1F));
}

// ── configFormatDeviceId Property Tests ─────────────────────────

// For any device_id 1-31, the formatted string is exactly 2 digits,
// zero-padded.
RC_GTEST_PROP(DipSwitchProperty8, FormatDeviceIdIsTwoDigitZeroPadded,
              ()) {
    auto deviceId = *rc::gen::inRange(1, 32);

    char buffer[4] = {};
    configFormatDeviceId(static_cast<uint8_t>(deviceId), buffer);

    // Must be exactly 2 characters long
    RC_ASSERT(std::strlen(buffer) == 2);

    // Both characters must be ASCII digits
    RC_ASSERT(buffer[0] >= '0' && buffer[0] <= '9');
    RC_ASSERT(buffer[1] >= '0' && buffer[1] <= '9');

    // The numeric value must match the input
    int parsed = (buffer[0] - '0') * 10 + (buffer[1] - '0');
    RC_ASSERT(parsed == deviceId);
}
