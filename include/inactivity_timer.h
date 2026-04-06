#ifndef INACTIVITY_TIMER_H
#define INACTIVITY_TIMER_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
// Provide millis() type for native builds
typedef unsigned long (*MillisFunc)();
#endif

namespace InactivityTimer {

/// Initialize the timer with a timeout duration (default 60000ms).
void init(unsigned long timeoutMs = 60000);

/// Reset the timer countdown (call after each scan event).
void reset();

/// Returns true if the timeout has elapsed since the last reset.
bool isExpired();

#ifndef ARDUINO
/// Inject a custom time source for native testing.
/// The provided function replaces millis() calls internally.
void setTimeSource(MillisFunc func);
#endif

} // namespace InactivityTimer

#endif // INACTIVITY_TIMER_H
