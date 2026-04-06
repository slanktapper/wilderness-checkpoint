/**
 * Property-based tests for LED Controller module (Property 2).
 *
 * Property 2: Countdown blink interval is monotonically decreasing
 * For any two elapsed times t1 and t2 within the countdown period [0, 3000ms]
 * where t1 < t2, the computed blink interval at t1 should be greater than or
 * equal to the blink interval at t2. The interval at t=0 should be 500ms and
 * at t=3000ms should be 100ms.
 *
 * **Validates: Requirements 3.1**
 */

#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "led_controller.h"

/**
 * Property 2a: Monotonically decreasing — for t1 < t2 in [0, 3000],
 * computeBlinkInterval(t1) >= computeBlinkInterval(t2).
 *
 * **Validates: Requirements 3.1**
 */
RC_GTEST_PROP(BlinkIntervalProperty,
              MonotonicallyDecreasing,
              ()) {
    // Generate two distinct elapsed times in [0, 3000]
    const auto t1 = *rc::gen::inRange<unsigned long>(0, 3001);
    const auto t2 = *rc::gen::inRange<unsigned long>(0, 3001);

    // We need t1 < t2; discard if not
    RC_PRE(t1 < t2);

    const auto interval1 = LED::computeBlinkInterval(t1);
    const auto interval2 = LED::computeBlinkInterval(t2);

    RC_ASSERT(interval1 >= interval2);
}

/**
 * Property 2b: Boundary at t=0 — interval is exactly 500ms.
 *
 * **Validates: Requirements 3.1**
 */
RC_GTEST_PROP(BlinkIntervalProperty,
              BoundaryAtZero,
              ()) {
    const auto interval = LED::computeBlinkInterval(0);
    RC_ASSERT(interval == 500);
}

/**
 * Property 2c: Boundary at t=3000 — interval is exactly 100ms.
 *
 * **Validates: Requirements 3.1**
 */
RC_GTEST_PROP(BlinkIntervalProperty,
              BoundaryAtTotal,
              ()) {
    const auto interval = LED::computeBlinkInterval(3000);
    RC_ASSERT(interval == 100);
}

/**
 * Property 2d: Always in range [100, 500] for any elapsed in [0, 3000].
 *
 * **Validates: Requirements 3.1**
 */
RC_GTEST_PROP(BlinkIntervalProperty,
              AlwaysInRange,
              ()) {
    const auto elapsed = *rc::gen::inRange<unsigned long>(0, 3001);

    const auto interval = LED::computeBlinkInterval(elapsed);

    RC_ASSERT(interval >= 100);
    RC_ASSERT(interval <= 500);
}
