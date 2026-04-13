#ifdef ARDUINO

#include "led_controller.h"

// --- Pattern types ---
enum class LedPattern : uint8_t {
    NONE,
    COUNTDOWN,       // Accelerating white/green blink
    SUCCESS,         // Alternating blue/green blink
    ERROR_BLINK      // Continuous blinking red (no timeout)
};

// --- Internal state ---
static LedPattern currentPattern = LedPattern::NONE;
static uint32_t patternStartMs   = 0;
static uint16_t patternDurationMs = 0;
static uint32_t lastToggleMs     = 0;
static bool     toggleState      = false;

// --- Helpers ---

static void applyColor(LedColor c) {
    analogWrite(PIN_LED_R, c.r);
    analogWrite(PIN_LED_G, c.g);
    analogWrite(PIN_LED_B, c.b);
}

// --- Public API ---

void ledInit() {
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);
    ledOff();
}

void ledSolid(LedColor color) {
    currentPattern = LedPattern::NONE;
    applyColor(color);
}

void ledOff() {
    currentPattern = LedPattern::NONE;
    applyColor(COLOR_OFF);
}

void ledStartCountdown(uint16_t durationMs) {
    currentPattern    = LedPattern::COUNTDOWN;
    patternStartMs    = millis();
    patternDurationMs = durationMs;
    lastToggleMs      = patternStartMs;
    toggleState       = false;
}

void ledStartSuccessPattern(uint16_t durationMs) {
    currentPattern    = LedPattern::SUCCESS;
    patternStartMs    = millis();
    patternDurationMs = durationMs;
    lastToggleMs      = patternStartMs;
    toggleState       = false;
}

void ledStartErrorBlink() {
    currentPattern    = LedPattern::ERROR_BLINK;
    patternStartMs    = millis();
    patternDurationMs = 0; // runs indefinitely
    lastToggleMs      = patternStartMs;
    toggleState       = false;
}

bool ledPatternActive() {
    return currentPattern != LedPattern::NONE;
}

void ledUpdate() {
    if (currentPattern == LedPattern::NONE) return;

    uint32_t now     = millis();
    uint32_t elapsed = now - patternStartMs;

    switch (currentPattern) {

    // Accelerating white/green blink.
    // Interval starts at 400ms and shrinks linearly to 80ms over durationMs.
    case LedPattern::COUNTDOWN: {
        if (elapsed >= patternDurationMs) {
            ledOff();
            return;
        }
        float progress    = (float)elapsed / (float)patternDurationMs; // 0..1
        uint16_t interval = (uint16_t)(400.0f - 320.0f * progress);   // 400→80 ms
        if (interval < 80) interval = 80;

        if (now - lastToggleMs >= interval) {
            lastToggleMs = now;
            toggleState  = !toggleState;
            applyColor(toggleState ? COLOR_WHITE : COLOR_GREEN);
        }
        break;
    }

    // Alternating blue/green blink at a fixed 250ms interval.
    case LedPattern::SUCCESS: {
        if (elapsed >= patternDurationMs) {
            ledOff();
            return;
        }
        if (now - lastToggleMs >= 250) {
            lastToggleMs = now;
            toggleState  = !toggleState;
            applyColor(toggleState ? COLOR_BLUE : COLOR_GREEN);
        }
        break;
    }

    // Continuous blinking red at 500ms interval (runs until reset).
    case LedPattern::ERROR_BLINK: {
        if (now - lastToggleMs >= 500) {
            lastToggleMs = now;
            toggleState  = !toggleState;
            applyColor(toggleState ? COLOR_RED : COLOR_OFF);
        }
        break;
    }

    default:
        break;
    }
}

#endif // ARDUINO
