#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#endif

namespace LED {

/// Initialize LEDC PWM channels for the RGB LED.
/// @param pinR  GPIO pin for red   (e.g. 12)
/// @param pinG  GPIO pin for green (e.g. 13)
/// @param pinB  GPIO pin for blue  (e.g. 33, inverted logic)
void init(uint8_t pinR, uint8_t pinG, uint8_t pinB);

/// Set an arbitrary RGB color (0–255 per channel).
/// Blue channel is automatically inverted for GPIO 33.
void setColor(uint8_t r, uint8_t g, uint8_t b);

/// Turn all channels off.
void off();

/// Solid green — device ready.
void green();

/// Solid red — error / invalid QR.
void red();

/// Solid blue — saving (GPIO 33 only, safe during SD operations).
void blue();

/// Solid white — QR accepted / capture flash.
void white();

/// Non-blocking alternating white/green blink (stub — Task 6.2).
void blinkWhiteGreen(unsigned long intervalMs);

/// Non-blocking alternating blue/green blink (stub — Task 6.2).
void blinkBlueGreen(unsigned long intervalMs);

/// Detach LEDC from GPIO 12/13 before SD card access.
void detachSharedPins();

/// Reattach LEDC to GPIO 12/13 after SD card access.
void reattachSharedPins();

/// Call each loop iteration for non-blocking blink patterns (stub — Task 6.2).
void update();

/// Compute blink interval via linear interpolation.
/// At elapsed=0 returns startMs (500), at elapsed=totalMs returns endMs (100).
/// Clamped to [endMs, startMs].
unsigned long computeBlinkInterval(unsigned long elapsed,
                                   unsigned long totalMs = 3000,
                                   unsigned long startMs = 500,
                                   unsigned long endMs = 100);

} // namespace LED

#endif // LED_CONTROLLER_H
