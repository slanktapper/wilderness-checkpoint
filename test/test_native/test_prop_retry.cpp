// Feature: wilderness-qr-checkpoint, Property 1: UART Command Retry on Timeout
//
// **Validates: Requirements 1.5**
//
// For any UART command sent by the Nano, if the ESP32-CAM does not respond
// within 500ms, the Nano shall retry up to 2 additional times (3 attempts
// total) before reporting an error. If any attempt succeeds, the successful
// response is returned.

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include <cstdint>
#include <vector>
#include <algorithm>

// Declared in src/nano/main.cpp (outside #ifdef ARDUINO)
extern int simulateRetry(const bool* outcomes, uint8_t numOutcomes, uint8_t maxRetries);

static const uint8_t MAX_RETRIES = 2; // per spec: 2 additional retries

// Helper: convert vector<uint8_t> (0/1) to vector<bool-as-bool> array
static std::vector<bool> toBoolVec(const std::vector<uint8_t>& v) {
    std::vector<bool> result(v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        result[i] = (v[i] != 0);
    }
    return result;
}

// Helper: copy bool values into a plain bool array for passing to simulateRetry
static void toBoolArray(const std::vector<uint8_t>& v, bool* arr, size_t len) {
    for (size_t i = 0; i < len && i < v.size(); ++i) {
        arr[i] = (v[i] != 0);
    }
}

// ── Property Tests ──────────────────────────────────────────────

// Property 1a: Total attempts never exceed 3 (1 initial + 2 retries).
// For any outcome sequence, simulateRetry inspects at most 3 entries.
RC_GTEST_PROP(RetryProperty1, TotalAttemptsAtMost3,
              ()) {
    // Generate a sequence of 1-10 outcomes (0 = timeout, 1 = success)
    auto count = *rc::gen::inRange(1, 11);
    auto raw = *rc::gen::container<std::vector<uint8_t>>(
        count, rc::gen::inRange<uint8_t>(0, 2));

    bool arr[10];
    toBoolArray(raw, arr, (size_t)count);

    int result = simulateRetry(arr, (uint8_t)count, MAX_RETRIES);

    // If a success was returned, it must be at index 0, 1, or 2
    if (result >= 0) {
        RC_ASSERT(result <= 2);
        // The outcome at that index must be true
        RC_ASSERT(arr[result] == true);
    }
    // If -1 returned, the first 3 (or fewer) entries must all be false
    if (result == -1) {
        uint8_t checkCount = std::min((uint8_t)count, (uint8_t)3);
        for (uint8_t i = 0; i < checkCount; ++i) {
            RC_ASSERT(arr[i] == false);
        }
    }
}

// Property 1b: If any of the first 3 attempts succeeds, the first success
// index is returned.
RC_GTEST_PROP(RetryProperty1, FirstSuccessReturned,
              ()) {
    // Generate exactly 3 outcomes
    auto raw = *rc::gen::container<std::vector<uint8_t>>(
        3, rc::gen::inRange<uint8_t>(0, 2));
    // Ensure at least one success
    auto successIdx = *rc::gen::inRange<size_t>(0, 3);
    raw[successIdx] = 1;

    bool arr[3];
    toBoolArray(raw, arr, 3);

    int result = simulateRetry(arr, 3, MAX_RETRIES);

    // Result must be the index of the first true value
    RC_ASSERT(result >= 0);
    RC_ASSERT(arr[result] == true);
    // All earlier entries must be false
    for (int i = 0; i < result; ++i) {
        RC_ASSERT(arr[i] == false);
    }
}

// Property 1c: If all 3 attempts fail, -1 is returned.
RC_GTEST_PROP(RetryProperty1, AllFailReturnsNegOne,
              ()) {
    // Generate extra outcomes beyond the 3 that are checked
    auto extraCount = *rc::gen::inRange<size_t>(0, 8);
    size_t totalLen = 3 + extraCount;
    bool arr[20];
    // First 3 are always false (timeout)
    arr[0] = false;
    arr[1] = false;
    arr[2] = false;
    // Extra outcomes can be anything — they should be ignored
    for (size_t i = 3; i < totalLen; ++i) {
        arr[i] = (*rc::gen::inRange<uint8_t>(0, 2) != 0);
    }

    int result = simulateRetry(arr, (uint8_t)totalLen, MAX_RETRIES);
    RC_ASSERT(result == -1);
}

// Property 1d: Outcomes beyond index 2 are never consulted.
// If the first 3 fail, result is -1 regardless of later outcomes.
RC_GTEST_PROP(RetryProperty1, OutcomesBeyondThreeIgnored,
              ()) {
    auto totalLen = *rc::gen::inRange<uint8_t>(4, 20);
    bool arr[20];
    // First 3 are false
    arr[0] = false;
    arr[1] = false;
    arr[2] = false;
    // Set all later ones to true
    for (uint8_t i = 3; i < totalLen; ++i) {
        arr[i] = true;
    }

    int result = simulateRetry(arr, totalLen, MAX_RETRIES);
    // Must still return -1 since only first 3 are checked
    RC_ASSERT(result == -1);
}
