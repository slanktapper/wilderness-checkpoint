/**
 * Property-based tests for InactivityTimer module (Property 6).
 *
 * Property 6: Inactivity timer correctness
 * For any timeout duration T and elapsed time E: after a reset, isExpired()
 * returns false when E < T and true when E >= T. Calling reset() at any point
 * restarts the countdown.
 *
 * **Validates: Requirements 6.1, 6.2**
 */

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "inactivity_timer.h"

// Shared simulated clock value used by the injected time source.
static unsigned long g_fakeMillis = 0;

static unsigned long fakeMillis() {
    return g_fakeMillis;
}

class InactivityTimerPropertyTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_fakeMillis = 0;
        InactivityTimer::setTimeSource(fakeMillis);
    }
};

/**
 * Property 6a: isExpired() returns false when elapsed < timeout.
 *
 * **Validates: Requirements 6.1, 6.2**
 */
RC_GTEST_FIXTURE_PROP(InactivityTimerPropertyTest,
                       NotExpiredBeforeTimeout,
                       ()) {
    // Generate a timeout in [1, 120000] ms (up to 2 minutes)
    const auto timeout = *rc::gen::inRange<unsigned long>(1, 120001);

    // Generate an elapsed time strictly less than timeout
    const auto elapsed = *rc::gen::inRange<unsigned long>(0, timeout);

    g_fakeMillis = 0;
    InactivityTimer::init(timeout);

    // Advance clock by elapsed (which is < timeout)
    g_fakeMillis = elapsed;

    RC_ASSERT(!InactivityTimer::isExpired());
}

/**
 * Property 6b: isExpired() returns true when elapsed >= timeout.
 *
 * **Validates: Requirements 6.1, 6.2**
 */
RC_GTEST_FIXTURE_PROP(InactivityTimerPropertyTest,
                       ExpiredAtOrAfterTimeout,
                       ()) {
    // Generate a timeout in [1, 120000] ms
    const auto timeout = *rc::gen::inRange<unsigned long>(1, 120001);

    // Generate an overshoot in [0, 60000] ms so elapsed = timeout + overshoot
    const auto overshoot = *rc::gen::inRange<unsigned long>(0, 60001);

    g_fakeMillis = 0;
    InactivityTimer::init(timeout);

    // Advance clock to at least the timeout
    g_fakeMillis = timeout + overshoot;

    RC_ASSERT(InactivityTimer::isExpired());
}

/**
 * Property 6c: reset() restarts the countdown — timer is not expired for
 * the next T milliseconds after a reset.
 *
 * **Validates: Requirements 6.1, 6.2**
 */
RC_GTEST_FIXTURE_PROP(InactivityTimerPropertyTest,
                       ResetRestartsCountdown,
                       ()) {
    // Generate a timeout in [1, 120000] ms
    const auto timeout = *rc::gen::inRange<unsigned long>(1, 120001);

    // Generate a time at which we perform the reset (could be before or after
    // the original expiry)
    const auto resetTime = *rc::gen::inRange<unsigned long>(0, 200001);

    // Generate an elapsed time after reset that is strictly less than timeout
    const auto elapsedAfterReset = *rc::gen::inRange<unsigned long>(0, timeout);

    g_fakeMillis = 0;
    InactivityTimer::init(timeout);

    // Advance to resetTime and call reset()
    g_fakeMillis = resetTime;
    InactivityTimer::reset();

    // Advance by elapsedAfterReset (still < timeout from the reset point)
    g_fakeMillis = resetTime + elapsedAfterReset;

    RC_ASSERT(!InactivityTimer::isExpired());
}

/**
 * Property 6d: After reset(), isExpired() returns true once the full timeout
 * elapses from the reset point.
 *
 * **Validates: Requirements 6.1, 6.2**
 */
RC_GTEST_FIXTURE_PROP(InactivityTimerPropertyTest,
                       ResetThenExpireAfterTimeout,
                       ()) {
    // Generate a timeout in [1, 120000] ms
    const auto timeout = *rc::gen::inRange<unsigned long>(1, 120001);

    // Generate a time at which we perform the reset
    const auto resetTime = *rc::gen::inRange<unsigned long>(0, 200001);

    // Generate an overshoot >= 0 so elapsed from reset >= timeout
    const auto overshoot = *rc::gen::inRange<unsigned long>(0, 60001);

    g_fakeMillis = 0;
    InactivityTimer::init(timeout);

    // Advance to resetTime and call reset()
    g_fakeMillis = resetTime;
    InactivityTimer::reset();

    // Advance by timeout + overshoot from the reset point
    g_fakeMillis = resetTime + timeout + overshoot;

    RC_ASSERT(InactivityTimer::isExpired());
}
