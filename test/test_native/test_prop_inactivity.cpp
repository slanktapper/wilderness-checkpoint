// Feature: wilderness-qr-checkpoint, Property 2: Inactivity Timeout with Scan Reset
//
// **Validates: Requirements 2.2, 2.5**
//
// For any sequence of scan events with associated timestamps, the system
// shall trigger a shutdown command iff the elapsed time since the last
// successful scan (or since wake) exceeds 60 seconds. Each successful
// scan resets the timer to 60 seconds.

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include <cstdint>
#include <vector>
#include <algorithm>

// Declared in src/nano/main.cpp (outside #ifdef ARDUINO)
extern bool shouldShutdown(uint32_t lastEventTime, uint32_t now, uint32_t timeoutMs);

static const uint32_t TIMEOUT_60S = 60000; // 60 seconds in ms

// ── Property Tests ──────────────────────────────────────────────

// Property 2a: For any lastEventTime and now where elapsed >= timeoutMs,
// shouldShutdown returns true.
RC_GTEST_PROP(InactivityProperty2, ShutdownWhenElapsedGteTimeout,
              ()) {
    auto lastEventTime = *rc::gen::arbitrary<uint32_t>();
    // Generate an elapsed time >= timeoutMs
    auto elapsed = *rc::gen::inRange<uint32_t>(TIMEOUT_60S, UINT32_MAX / 2);
    uint32_t now = lastEventTime + elapsed;

    RC_ASSERT(shouldShutdown(lastEventTime, now, TIMEOUT_60S) == true);
}

// Property 2b: For any lastEventTime and now where elapsed < timeoutMs,
// shouldShutdown returns false.
RC_GTEST_PROP(InactivityProperty2, NoShutdownWhenElapsedLtTimeout,
              ()) {
    auto lastEventTime = *rc::gen::arbitrary<uint32_t>();
    // Generate an elapsed time strictly less than timeoutMs
    auto elapsed = *rc::gen::inRange<uint32_t>(0, TIMEOUT_60S);
    uint32_t now = lastEventTime + elapsed;

    RC_ASSERT(shouldShutdown(lastEventTime, now, TIMEOUT_60S) == false);
}

// Property 2c: uint32_t wrap-around is handled correctly.
// millis() wraps at ~49.7 days (UINT32_MAX). Unsigned subtraction
// naturally handles this: (now - lastEventTime) gives the correct
// elapsed time even when now < lastEventTime numerically.
RC_GTEST_PROP(InactivityProperty2, WraparoundHandledCorrectly,
              ()) {
    // Place lastEventTime near the top of uint32_t range
    auto lastEventTime = *rc::gen::inRange<uint32_t>(
        UINT32_MAX - 200000, UINT32_MAX);
    // Generate elapsed that wraps around (now < lastEventTime numerically)
    auto elapsed = *rc::gen::inRange<uint32_t>(1, 200000);
    uint32_t now = lastEventTime + elapsed; // wraps around naturally

    bool expected = elapsed >= TIMEOUT_60S;
    RC_ASSERT(shouldShutdown(lastEventTime, now, TIMEOUT_60S) == expected);
}

// Property 2d: Simulate a sequence of scan events. After each scan the
// timer resets. Shutdown only triggers after 60s of no scans.
RC_GTEST_PROP(InactivityProperty2, ScanSequenceResetsTimer,
              ()) {
    // Generate 1-20 scan events with inter-scan gaps < 60s
    auto scanCount = *rc::gen::inRange(1, 21);
    uint32_t lastEvent = *rc::gen::inRange<uint32_t>(0, UINT32_MAX / 2);

    for (int i = 0; i < scanCount; ++i) {
        // Each scan happens within the timeout window
        auto gap = *rc::gen::inRange<uint32_t>(0, TIMEOUT_60S);
        uint32_t checkTime = lastEvent + gap;

        // Should NOT shutdown — still within timeout
        RC_ASSERT(shouldShutdown(lastEvent, checkTime, TIMEOUT_60S) == false);

        // Scan occurs, resetting the timer
        lastEvent = checkTime;
    }

    // After the last scan, wait >= 60s — should trigger shutdown
    auto finalGap = *rc::gen::inRange<uint32_t>(TIMEOUT_60S, TIMEOUT_60S + 120000);
    uint32_t finalCheck = lastEvent + finalGap;
    RC_ASSERT(shouldShutdown(lastEvent, finalCheck, TIMEOUT_60S) == true);
}
