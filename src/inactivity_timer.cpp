#include "inactivity_timer.h"

namespace {

unsigned long timeoutDuration = 60000;
unsigned long lastResetTime   = 0;

#ifdef ARDUINO

unsigned long now() {
  return millis();
}

#else

// Default native time source — returns 0 until overridden via setTimeSource()
static unsigned long defaultMillis() { return 0; }
static MillisFunc currentMillis = defaultMillis;

unsigned long now() {
  return currentMillis();
}

#endif

} // anonymous namespace

namespace InactivityTimer {

void init(unsigned long timeoutMs) {
  timeoutDuration = timeoutMs;
  lastResetTime   = now();
}

void reset() {
  lastResetTime = now();
}

bool isExpired() {
  return (now() - lastResetTime) >= timeoutDuration;
}

#ifndef ARDUINO

void setTimeSource(MillisFunc func) {
  currentMillis = func ? func : defaultMillis;
}

#endif

} // namespace InactivityTimer
