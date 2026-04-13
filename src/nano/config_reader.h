#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#endif

// DIP switch pins (D3-D7), active LOW with internal pull-ups
static const uint8_t DIP_PINS[] = {3, 4, 5, 6, 7};
static const uint8_t DIP_PIN_COUNT = 5;

// ── Pure logic (compiles on native host and Arduino) ────────────

// Read device ID from a pre-inverted 5-bit pin value (0-31).
// Returns the value directly as the device ID.
// Value 0 means all switches OFF = invalid configuration.
// Valid range: 1-31.
uint8_t configReadDeviceId(uint8_t pinValues);

// Format device ID as a 2-digit zero-padded string.
// Buffer must be at least 3 bytes (e.g., "07\0").
void configFormatDeviceId(uint8_t deviceId, char* buffer);

#ifdef ARDUINO
// Initialize DIP switch pins with internal pull-ups.
void configInit();

// Read DIP switches, invert (active LOW), and return device ID (0-31).
// Returns 0 if all switches are OFF (invalid configuration).
uint8_t configReadDeviceId();
#endif

#endif // CONFIG_READER_H
