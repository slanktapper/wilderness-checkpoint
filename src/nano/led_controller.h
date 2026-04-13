#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#endif

// Pin assignments for KY-016 RGB LED
static const uint8_t PIN_LED_R = 9;
static const uint8_t PIN_LED_G = 10;
static const uint8_t PIN_LED_B = 11;

// LED color presets (R, G, B values 0-255)
struct LedColor {
    uint8_t r, g, b;
};

static const LedColor COLOR_OFF   = {0, 0, 0};
static const LedColor COLOR_RED   = {255, 0, 0};
static const LedColor COLOR_GREEN = {0, 255, 0};
static const LedColor COLOR_BLUE  = {0, 0, 255};
static const LedColor COLOR_WHITE = {255, 255, 255};

// Initialize LED pins as OUTPUT and turn off.
void ledInit();

// Set LED to a solid color.
void ledSolid(LedColor color);

// Turn LED off.
void ledOff();

// Non-blocking LED pattern update. Call from loop().
// Manages timed patterns (blink, accelerating blink, alternating).
void ledUpdate();

// Start accelerating white/green blink pattern over durationMs.
void ledStartCountdown(uint16_t durationMs);

// Start alternating blue/green blink pattern for durationMs.
void ledStartSuccessPattern(uint16_t durationMs);

// Start blinking red pattern (for invalid DIP config). Runs until reset.
void ledStartErrorBlink();

// Returns true if a timed pattern is currently active.
bool ledPatternActive();

#endif
