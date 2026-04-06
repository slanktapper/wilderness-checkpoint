#include "led_controller.h"
#include "config.h"

#ifdef ARDUINO

// ─── ESP32 LEDC Implementation ─────────────────────────────────────────────

namespace {

// LEDC channel assignments
static const uint8_t CH_RED   = 0;
static const uint8_t CH_GREEN = 1;
static const uint8_t CH_BLUE  = 2;

// PWM configuration
static const double   PWM_FREQ       = 5000;  // 5 kHz
static const uint8_t  PWM_RESOLUTION = 8;     // 8-bit (0–255)

// Pin state
static uint8_t pinRed   = 0;
static uint8_t pinGreen = 0;
static uint8_t pinBlue  = 0;
static bool    sharedDetached = false;

// Blink state
enum class BlinkMode { NONE, WHITE_GREEN, BLUE_GREEN };
static BlinkMode      blinkMode     = BlinkMode::NONE;
static unsigned long  blinkInterval = 0;
static unsigned long  lastToggleMs  = 0;
static bool           blinkPhaseA   = true;  // true = first color, false = second

} // anonymous namespace

namespace LED {

void init(uint8_t pinR, uint8_t pinG, uint8_t pinB) {
  pinRed   = pinR;
  pinGreen = pinG;
  pinBlue  = pinB;

  // Configure LEDC channels
  ledcSetup(CH_RED,   PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(CH_GREEN, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(CH_BLUE,  PWM_FREQ, PWM_RESOLUTION);

  // Attach pins to channels
  ledcAttachPin(pinRed,   CH_RED);
  ledcAttachPin(pinGreen, CH_GREEN);
  ledcAttachPin(pinBlue,  CH_BLUE);

  sharedDetached = false;
  off();
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
  if (!sharedDetached) {
    ledcWrite(CH_RED,   r);
    ledcWrite(CH_GREEN, g);
  }
  // Blue on GPIO 33 uses inverted logic: 255 = off, 0 = full on
  ledcWrite(CH_BLUE, 255 - b);
}

void off() {
  if (!sharedDetached) {
    ledcWrite(CH_RED,   0);
    ledcWrite(CH_GREEN, 0);
  }
  // Inverted: 255 = off
  ledcWrite(CH_BLUE, 255);
}

void green() {
  setColor(0, 255, 0);
}

void red() {
  setColor(255, 0, 0);
}

void blue() {
  // Safe to call even when shared pins are detached — only uses GPIO 33
  if (!sharedDetached) {
    ledcWrite(CH_RED,   0);
    ledcWrite(CH_GREEN, 0);
  }
  ledcWrite(CH_BLUE, 0);  // Inverted: 0 = full on
}

void white() {
  setColor(255, 255, 255);
}

void blinkWhiteGreen(unsigned long intervalMs) {
  blinkMode     = BlinkMode::WHITE_GREEN;
  blinkInterval = intervalMs;
  lastToggleMs  = millis();
  blinkPhaseA   = true;
  white();  // Start on white
}

void blinkBlueGreen(unsigned long intervalMs) {
  blinkMode     = BlinkMode::BLUE_GREEN;
  blinkInterval = intervalMs;
  lastToggleMs  = millis();
  blinkPhaseA   = true;
  blue();  // Start on blue
}

void detachSharedPins() {
  // Turn off red/green before detaching
  ledcWrite(CH_RED,   0);
  ledcWrite(CH_GREEN, 0);
  ledcDetachPin(pinRed);
  ledcDetachPin(pinGreen);
  sharedDetached = true;
}

void reattachSharedPins() {
  ledcAttachPin(pinRed,   CH_RED);
  ledcAttachPin(pinGreen, CH_GREEN);
  sharedDetached = false;
}

void update() {
  if (blinkMode == BlinkMode::NONE || blinkInterval == 0) return;

  unsigned long now = millis();
  if (now - lastToggleMs >= blinkInterval) {
    lastToggleMs = now;
    blinkPhaseA  = !blinkPhaseA;

    switch (blinkMode) {
      case BlinkMode::WHITE_GREEN:
        if (blinkPhaseA) white(); else green();
        break;
      case BlinkMode::BLUE_GREEN:
        if (blinkPhaseA) blue(); else green();
        break;
      default:
        break;
    }
  }
}

} // namespace LED

#else

// ─── Native (no-op) Stubs ──────────────────────────────────────────────────

namespace LED {

void init(uint8_t /*pinR*/, uint8_t /*pinG*/, uint8_t /*pinB*/) {}
void setColor(uint8_t /*r*/, uint8_t /*g*/, uint8_t /*b*/) {}
void off() {}
void green() {}
void red() {}
void blue() {}
void white() {}
void blinkWhiteGreen(unsigned long /*intervalMs*/) {}
void blinkBlueGreen(unsigned long /*intervalMs*/) {}
void detachSharedPins() {}
void reattachSharedPins() {}
void update() {}

} // namespace LED

#endif

// ─── Pure Functions (available in all builds) ───────────────────────────────

namespace LED {

unsigned long computeBlinkInterval(unsigned long elapsed,
                                   unsigned long totalMs,
                                   unsigned long startMs,
                                   unsigned long endMs) {
  // Clamp elapsed to [0, totalMs]
  if (elapsed > totalMs) elapsed = totalMs;

  // Linear interpolation: startMs → endMs as elapsed goes 0 → totalMs
  // interval = startMs - (startMs - endMs) * elapsed / totalMs
  unsigned long range = startMs - endMs;
  unsigned long interval = startMs - (range * elapsed / totalMs);
  return interval;
}

} // namespace LED
