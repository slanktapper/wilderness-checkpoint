#include "config_reader.h"

// ── Pure logic (compiles on native host and Arduino) ────────────

uint8_t configReadDeviceId(uint8_t pinValues) {
    // Mask to 5 bits and return directly.
    // The caller has already inverted the active-LOW signals.
    return pinValues & 0x1F;
}

void configFormatDeviceId(uint8_t deviceId, char* buffer) {
    if (buffer == nullptr) {
        return;
    }
    // 2-digit zero-padded decimal string
    buffer[0] = '0' + (deviceId / 10);
    buffer[1] = '0' + (deviceId % 10);
    buffer[2] = '\0';
}

// ── Hardware-dependent (Arduino only) ───────────────────────────

#ifdef ARDUINO

void configInit() {
    for (uint8_t i = 0; i < DIP_PIN_COUNT; ++i) {
        pinMode(DIP_PINS[i], INPUT_PULLUP);
    }
}

uint8_t configReadDeviceId() {
    uint8_t raw = 0;
    for (uint8_t i = 0; i < DIP_PIN_COUNT; ++i) {
        if (digitalRead(DIP_PINS[i]) == LOW) {
            raw |= (1 << i);
        }
    }
    // raw is already inverted (LOW = ON = 1), pass to pure logic overload
    return configReadDeviceId(raw);
}

#endif // ARDUINO
