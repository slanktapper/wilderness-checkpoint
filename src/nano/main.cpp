// Wilderness QR Checkpoint — Nano Main State Machine
// Requirements: 1.1, 1.4, 1.5, 2.1, 2.2, 2.4, 2.5, 3.1, 4.1, 5.1,
//               7.2, 7.3, 8.1–8.8, 9.1–9.6, 10.6, 10.7

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#endif

// ── Pure logic (testable on native host) ────────────────────────

// Inactivity timeout check.
// Returns true if (now - lastEventTime) >= timeoutMs, handling wrap-around.
bool shouldShutdown(uint32_t lastEventTime, uint32_t now, uint32_t timeoutMs) {
    return (now - lastEventTime) >= timeoutMs;
}

// Simulate UART command retry logic as a pure function.
// outcomes[i] = true means attempt i succeeded, false means timeout.
// maxRetries = maximum additional retries (2 per spec, so 3 attempts total).
// Returns: 0-based index of first successful attempt, or -1 if all failed.
int simulateRetry(const bool* outcomes, uint8_t numOutcomes, uint8_t maxRetries) {
    uint8_t totalAttempts = 1 + maxRetries; // initial + retries
    for (uint8_t i = 0; i < totalAttempts && i < numOutcomes; ++i) {
        if (outcomes[i]) {
            return (int)i;
        }
    }
    return -1;
}

// ── Hardware-dependent (Arduino only) ───────────────────────────

#ifdef ARDUINO

#include <Arduino.h>
#include "uart_protocol.h"
#include "led_controller.h"
#include "power_manager.h"
#include "rtc_manager.h"
#include "config_reader.h"

// ── Constants ───────────────────────────────────────────────────

static const uint32_t INACTIVITY_TIMEOUT_MS  = 60000; // 60 seconds
static const uint16_t QR_DECODED_DISPLAY_MS  = 1000;  // White LED 1s
static const uint16_t COUNTDOWN_DURATION_MS  = 3000;  // Countdown pattern 3s
static const uint16_t SUCCESS_PATTERN_MS     = 3000;  // Success pattern 3s
static const uint16_t ERROR_DISPLAY_MS       = 3000;  // Red LED 3s on error
static const uint16_t INVALID_QR_DISPLAY_MS  = 2000;  // Red LED 2s on invalid QR
static const uint16_t SHUTDOWN_WAIT_MS       = 5000;  // Wait after SHUTDOWN_READY

// ── State Enum ──────────────────────────────────────────────────

enum class NanoState : uint8_t {
    DEEP_SLEEP,
    BOOTING,
    WAIT_ESP_INIT,
    READY,
    SCANNING,
    QR_DECODED,
    COUNTDOWN,
    CAPTURING,
    SAVING,
    SCAN_COMPLETE,
    SHUTDOWN,
    ERROR
};

// ── Global State ────────────────────────────────────────────────

static NanoState state = NanoState::DEEP_SLEEP;

static uint8_t  deviceId = 0;
static char     deviceIdStr[3] = {0};  // "XX\0"
static char     userId[16] = {0};      // From QR scan
static bool     rtcOk = false;

// Timers
static uint32_t lastEventTime = 0;     // For inactivity timeout
static uint32_t stateEntryTime = 0;    // For timed states (QR_DECODED, etc.)

// ── Helpers ─────────────────────────────────────────────────────

static void enterState(NanoState newState) {
    state = newState;
    stateEntryTime = millis();
}

// ── setup() ─────────────────────────────────────────────────────

void setup() {
    // 1. Initialize power (MOSFET HIGH = ESP off, button pull-up)
    powerInit();

    // 2. Initialize LED
    ledInit();

    // 3. Initialize DIP switch pins and read device ID
    configInit();
    deviceId = configReadDeviceId();

    // 4. Validate device ID — halt with blinking red if 0
    if (deviceId == 0) {
        ledStartErrorBlink();
        // Halt forever — never proceed
        while (true) {
            ledUpdate();
        }
    }

    // Format device ID for later use in SAVE command
    configFormatDeviceId(deviceId, deviceIdStr);

    // 5. Power on ESP32-CAM
    espPowerOn();

    // 6. Initialize RTC (continue with fallback if it fails)
    rtcOk = rtcInit();
    if (!rtcOk) {
        // Req 7.2: solid red 3s on RTC failure, then continue
        ledSolid(COLOR_RED);
        delay(ERROR_DISPLAY_MS);
        ledOff();
    }

    // 7. Initialize UART
    uartInit();

    // 8. Wait for ESP32-CAM init response
    enterState(NanoState::WAIT_ESP_INIT);
    lastEventTime = millis();
}

// ── loop() ──────────────────────────────────────────────────────

void loop() {
    uint32_t now = millis();

    // Always update LED patterns
    ledUpdate();

    switch (state) {

    // ── WAIT_ESP_INIT ───────────────────────────────────────────
    case NanoState::WAIT_ESP_INIT: {
        // Wait for RSP:INIT_OK from ESP32-CAM (sent via sendCommand with retries)
        ParsedResponse resp = sendCommand("CMD:STATUS");
        if (resp.status == EspResponse::INIT_OK) {
            // Req 10.6, 8.1: solid green = ready
            ledSolid(COLOR_GREEN);
            lastEventTime = millis();
            enterState(NanoState::READY);
        } else if (resp.status == EspResponse::SD_ERR ||
                   resp.status == EspResponse::CAM_ERR) {
            // Req 10.7: solid red, error state
            ledSolid(COLOR_RED);
            enterState(NanoState::ERROR);
        } else if (resp.status == EspResponse::TIMEOUT) {
            // Timeout after retries — error
            ledSolid(COLOR_RED);
            enterState(NanoState::ERROR);
        }
        // UNKNOWN or other: keep waiting (will retry on next loop)
        break;
    }

    // ── READY ───────────────────────────────────────────────────
    case NanoState::READY: {
        // Check inactivity timeout
        if (shouldShutdown(lastEventTime, now, INACTIVITY_TIMEOUT_MS)) {
            // Req 2.2: send shutdown command
            ParsedResponse resp = sendCommand("CMD:SHUTDOWN");
            if (resp.status == EspResponse::SHUTDOWN_READY) {
                enterState(NanoState::SHUTDOWN);
            } else {
                // Even on timeout, proceed to shutdown
                enterState(NanoState::SHUTDOWN);
            }
            break;
        }

        // Send scan command
        ParsedResponse resp = sendCommand("CMD:SCAN");
        if (resp.status == EspResponse::TIMEOUT) {
            // UART timeout — show red, stay ready
            ledSolid(COLOR_RED);
            stateEntryTime = millis();
            // Brief error display, then back to green
            delay(ERROR_DISPLAY_MS);
            ledSolid(COLOR_GREEN);
            break;
        }
        enterState(NanoState::SCANNING);
        break;
    }

    // ── SCANNING ────────────────────────────────────────────────
    case NanoState::SCANNING: {
        // Read response from ESP32-CAM (scan is ongoing on ESP side)
        // The ESP sends RSP:QR:YYYY when found, or RSP:QR_INVALID
        ParsedResponse resp = sendCommand("CMD:POLL");
        if (resp.status == EspResponse::QR_FOUND) {
            // Store userId for SAVE command
            strncpy(userId, resp.data, sizeof(userId) - 1);
            userId[sizeof(userId) - 1] = '\0';
            // Req 8.2: white LED 1s
            ledSolid(COLOR_WHITE);
            enterState(NanoState::QR_DECODED);
        } else if (resp.status == EspResponse::QR_INVALID) {
            // Req 8.3: red 2s, then back to ready (green)
            ledSolid(COLOR_RED);
            delay(INVALID_QR_DISPLAY_MS);
            ledSolid(COLOR_GREEN);
            enterState(NanoState::READY);
        } else if (resp.status == EspResponse::CAM_ERR) {
            ledSolid(COLOR_RED);
            enterState(NanoState::ERROR);
        }
        // TIMEOUT or other: keep polling
        break;
    }

    // ── QR_DECODED ──────────────────────────────────────────────
    case NanoState::QR_DECODED: {
        // Req 8.2: white LED for 1s, then start countdown
        if (now - stateEntryTime >= QR_DECODED_DISPLAY_MS) {
            // Req 8.4: start countdown pattern (3s accelerating blink)
            ledStartCountdown(COUNTDOWN_DURATION_MS);
            enterState(NanoState::COUNTDOWN);
        }
        break;
    }

    // ── COUNTDOWN ───────────────────────────────────────────────
    case NanoState::COUNTDOWN: {
        // Wait for countdown pattern to finish
        if (!ledPatternActive()) {
            // Send capture command
            ParsedResponse resp = sendCommand("CMD:CAPTURE");
            if (resp.status == EspResponse::PHOTO_READY) {
                // Req 8.5: solid blue while saving
                ledSolid(COLOR_BLUE);
                enterState(NanoState::CAPTURING);
            } else if (resp.status == EspResponse::CAM_ERR) {
                // Req 8.7: red 3s on camera error
                ledSolid(COLOR_RED);
                delay(ERROR_DISPLAY_MS);
                ledSolid(COLOR_GREEN);
                enterState(NanoState::READY);
            } else if (resp.status == EspResponse::TIMEOUT) {
                ledSolid(COLOR_RED);
                delay(ERROR_DISPLAY_MS);
                ledSolid(COLOR_GREEN);
                enterState(NanoState::READY);
            }
            break;
        }
        break;
    }

    // ── CAPTURING ───────────────────────────────────────────────
    case NanoState::CAPTURING: {
        // Photo is ready in ESP memory. Build and send SAVE command.
        // Req 7.3: read RTC timestamp just before saving
        char timestamp[20];
        if (rtcOk) {
            if (!rtcGetTimestamp(timestamp, sizeof(timestamp))) {
                // RTC read failed — use fallback
                strncpy(timestamp, rtcFallbackTimestamp(), sizeof(timestamp) - 1);
                timestamp[sizeof(timestamp) - 1] = '\0';
            }
        } else {
            strncpy(timestamp, rtcFallbackTimestamp(), sizeof(timestamp) - 1);
            timestamp[sizeof(timestamp) - 1] = '\0';
        }

        // Build CMD:SAVE:uid:did:ts
        char saveCmd[UART_MAX_MSG_LEN];
        uint8_t len = formatSaveCommand(saveCmd, sizeof(saveCmd),
                                         userId, deviceIdStr, timestamp);
        if (len == 0) {
            // Buffer error — shouldn't happen, go to error
            ledSolid(COLOR_RED);
            delay(ERROR_DISPLAY_MS);
            ledSolid(COLOR_GREEN);
            enterState(NanoState::READY);
            break;
        }

        // Remove trailing newline for sendCommand (it adds its own)
        if (len > 0 && saveCmd[len - 1] == '\n') {
            saveCmd[len - 1] = '\0';
        }

        // Req 8.5: solid blue while saving
        ledSolid(COLOR_BLUE);

        ParsedResponse resp = sendCommand(saveCmd);
        if (resp.status == EspResponse::SAVE_OK) {
            enterState(NanoState::SAVING);
        } else if (resp.status == EspResponse::STORAGE_ERR) {
            // Req 8.7: red 3s on storage error, back to ready
            ledSolid(COLOR_RED);
            delay(ERROR_DISPLAY_MS);
            ledSolid(COLOR_GREEN);
            enterState(NanoState::READY);
        } else if (resp.status == EspResponse::TIMEOUT) {
            ledSolid(COLOR_RED);
            delay(ERROR_DISPLAY_MS);
            ledSolid(COLOR_GREEN);
            enterState(NanoState::READY);
        }
        break;
    }

    // ── SAVING ──────────────────────────────────────────────────
    case NanoState::SAVING: {
        // Save was successful — transition to SCAN_COMPLETE
        // Req 8.6: success pattern (blue/green alternating 3s)
        ledStartSuccessPattern(SUCCESS_PATTERN_MS);
        enterState(NanoState::SCAN_COMPLETE);
        break;
    }

    // ── SCAN_COMPLETE ───────────────────────────────────────────
    case NanoState::SCAN_COMPLETE: {
        // Wait for success pattern to finish
        if (!ledPatternActive()) {
            // Req 2.5: reset inactivity timer on successful scan
            lastEventTime = millis();
            // Req 8.1: back to solid green
            ledSolid(COLOR_GREEN);
            enterState(NanoState::READY);
        }
        break;
    }

    // ── SHUTDOWN ────────────────────────────────────────────────
    case NanoState::SHUTDOWN: {
        // Req 2.4: wait 5s, cut power, enter deep sleep
        if (now - stateEntryTime >= SHUTDOWN_WAIT_MS) {
            // Req 8.8: LED off before sleep
            ledOff();
            espPowerOff();
            enterState(NanoState::DEEP_SLEEP);
            enterDeepSleep();
            // --- Execution resumes here after wake button press ---
            // Re-run setup sequence
            setup();
        }
        break;
    }

    // ── ERROR ───────────────────────────────────────────────────
    case NanoState::ERROR: {
        // Req 10.7: solid red, wait for button press to retry
        // Check if button is pressed (LOW = pressed, pull-up)
        if (digitalRead(PIN_WAKE_BUTTON) == LOW) {
            delay(50); // debounce
            if (digitalRead(PIN_WAKE_BUTTON) == LOW) {
                // Power cycle ESP and re-init
                espPowerOff();
                delay(500);
                ledOff();
                // Re-run full setup
                setup();
            }
        }
        break;
    }

    // ── DEEP_SLEEP ──────────────────────────────────────────────
    case NanoState::DEEP_SLEEP: {
        // Should not reach here during normal operation.
        // enterDeepSleep() is called from SHUTDOWN state.
        // If we somehow end up here, go to sleep.
        ledOff();
        espPowerOff();
        enterDeepSleep();
        setup();
        break;
    }

    // ── BOOTING (handled in setup) ──────────────────────────────
    case NanoState::BOOTING:
    default:
        break;
    }
}

#endif // ARDUINO
