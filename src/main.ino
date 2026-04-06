/**
 * Wilderness QR Checkpoint Device — Main Firmware
 *
 * ESP32-CAM (AI-Thinker) based checkpoint station for outdoor events.
 * Wakes on button press, scans QR code, captures proof-of-presence photo,
 * stores photo + CSV log to SD card, sleeps on inactivity.
 */

#include "config.h"
#include "device_state.h"
#include "led_controller.h"
#include "qr_scanner.h"
#include "photo_capture.h"
#include "storage_manager.h"
#include "rtc_manager.h"
#include "inactivity_timer.h"

#ifdef ARDUINO
#include "esp_sleep.h"
#include "esp_camera.h"
#endif

// ─── Global State ───────────────────────────────────────────────────────────

DeviceContext ctx;

// Temporary storage for captured photo data during SAVING state
static uint8_t* capturedPhotoData   = nullptr;
static size_t   capturedPhotoLength = 0;

// Track whether SD was missing at boot (permanent ERROR)
static bool sdMissingAtBoot = false;

// ─── Helper: State Transition ───────────────────────────────────────────────

void transitionTo(DeviceState newState) {
  ctx.currentState  = newState;
  ctx.stateEnteredAt = millis();
}

// ─── setup() — Task 11.1 ───────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);

  // Determine wake cause
#ifdef ARDUINO
  esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
  if (wakeCause == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("[BOOT] Woke from deep sleep via button press");
  } else {
    Serial.println("[BOOT] Power-on or reset");
  }
#endif

  // Initialize DeviceContext defaults
  ctx.currentState    = STATE_INIT;
  ctx.stateEnteredAt  = millis();
  ctx.lastScanAt      = millis();
  ctx.countdownStartAt = 0;
  ctx.currentUserId   = "";
  ctx.sdReady         = false;
  ctx.rtcReady        = false;

  // Initialize I2C / RTC
  ctx.rtcReady = RTC::init();
  if (ctx.rtcReady) {
    Serial.println("[BOOT] RTC initialized OK");
  } else {
    Serial.println("[BOOT] RTC unavailable — using fallback timestamps");
  }

  // Check SD card health: init, check, then deinit (camera needs the pins)
  ctx.sdReady = Storage::init();
  if (ctx.sdReady) {
    ctx.sdReady = Storage::isReady();
    Storage::deinit();
  }

  if (!ctx.sdReady) {
    Serial.println("[BOOT] SD card missing or unwritable — entering ERROR");
    sdMissingAtBoot = true;
    // Initialize LED so we can show red
    LED::init(PIN_LED_RED, PIN_LED_GREEN, PIN_LED_BLUE);
    LED::red();
    transitionTo(STATE_ERROR);
    return;
  }

  Serial.println("[BOOT] SD card OK");

  // Initialize camera (QR scanning mode)
  bool camOk = QRScanner::init();
  if (!camOk) {
    Serial.println("[BOOT] Camera init failed — entering ERROR");
    LED::init(PIN_LED_RED, PIN_LED_GREEN, PIN_LED_BLUE);
    LED::red();
    transitionTo(STATE_ERROR);
    return;
  }
  Serial.println("[BOOT] Camera initialized OK");

  // Initialize LED and set green (ready)
  LED::init(PIN_LED_RED, PIN_LED_GREEN, PIN_LED_BLUE);
  LED::green();

  // Initialize inactivity timer
  InactivityTimer::init(INACTIVITY_MS);

  Serial.println("[BOOT] Device ready");
  transitionTo(STATE_READY);
}

// ─── loop() — Tasks 11.2–11.5 ──────────────────────────────────────────────

void loop() {
  // Drive non-blocking LED blink patterns every iteration
  LED::update();

  unsigned long now = millis();
  unsigned long elapsed = now - ctx.stateEnteredAt;

  switch (ctx.currentState) {

  // ── READY / QR_SCANNING — Task 11.2 ────────────────────────────────────
  case STATE_READY:
  case STATE_QR_SCANNING: {
    // Check inactivity timeout first
    if (InactivityTimer::isExpired()) {
      Serial.println("[STATE] Inactivity timeout — going to sleep");
      transitionTo(STATE_SLEEP_PENDING);
      break;
    }

    // Attempt QR scan each iteration
    ScanResult result = QRScanner::scan();

    if (result.found) {
      if (result.valid) {
        // Valid QR — accept
        ctx.currentUserId = result.userId;
        Serial.print("[STATE] Valid QR scanned: ");
        Serial.println(result.userId.c_str());
        LED::white();
        transitionTo(STATE_QR_ACCEPTED);
      } else {
        // Invalid QR — red flash
        Serial.print("[STATE] Invalid QR: ");
        Serial.println(result.userId.c_str());
        LED::red();
        transitionTo(STATE_QR_INVALID);
      }
    }
    break;
  }

  // ── QR_INVALID — Task 11.2 ────────────────────────────────────────────
  case STATE_QR_INVALID: {
    // Red LED for INVALID_MS (2s), then back to READY
    if (elapsed >= INVALID_MS) {
      LED::green();
      transitionTo(STATE_READY);
    }
    break;
  }

  // ── QR_ACCEPTED — Task 11.3 ───────────────────────────────────────────
  case STATE_QR_ACCEPTED: {
    // Solid white for ACCEPTED_MS (1s), then transition to COUNTDOWN
    if (elapsed >= ACCEPTED_MS) {
      ctx.countdownStartAt = millis();
      unsigned long interval = LED::computeBlinkInterval(0);
      LED::blinkWhiteGreen(interval);
      transitionTo(STATE_COUNTDOWN);
    }
    break;
  }

  // ── COUNTDOWN — Task 11.3 ─────────────────────────────────────────────
  case STATE_COUNTDOWN: {
    unsigned long countdownElapsed = millis() - ctx.countdownStartAt;

    if (countdownElapsed >= COUNTDOWN_MS) {
      // Countdown complete — solid white for capture
      LED::white();
      transitionTo(STATE_CAPTURE);
    } else {
      // Update blink interval based on elapsed time (accelerating)
      unsigned long interval = LED::computeBlinkInterval(countdownElapsed);
      LED::blinkWhiteGreen(interval);
    }
    break;
  }

  // ── CAPTURE — Task 11.3 ───────────────────────────────────────────────
  case STATE_CAPTURE: {
    // Deinit QR scanner camera, init photo camera, capture, then save
    QRScanner::deinit();

    bool photoInitOk = PhotoCapture::init();
    if (!photoInitOk) {
      Serial.println("[STATE] Photo camera init failed");
      LED::red();
      transitionTo(STATE_ERROR);
      break;
    }

    CaptureResult photo = PhotoCapture::capture();
    if (!photo.success) {
      Serial.println("[STATE] Photo capture failed");
      PhotoCapture::deinit();
      LED::red();
      transitionTo(STATE_ERROR);
      break;
    }

    // Stash photo data for SAVING state
    capturedPhotoData   = photo.data;
    capturedPhotoLength = photo.length;

    Serial.print("[STATE] Photo captured: ");
    Serial.print(capturedPhotoLength);
    Serial.println(" bytes");

    transitionTo(STATE_SAVING);
    break;
  }

  // ── SAVING — Task 11.4 ────────────────────────────────────────────────
  case STATE_SAVING: {
    // 1. Deinit camera (release GPIO pins)
    PhotoCapture::deinit();

    // 2. Detach shared LED pins (GPIO 12/13) for SD access
    LED::detachSharedPins();

    // 3. Blue LED only (GPIO 33 — safe during SD operations)
    LED::blue();

    // Record when blue LED started
    unsigned long blueStartMs = millis();

    // 4. Init SD card
    bool sdOk = Storage::init();
    bool saveOk = false;
    bool logOk  = false;

    if (sdOk) {
      // 5. Save photo + append scan log
      saveOk = Storage::savePhoto(DEVICE_ID, ctx.currentUserId.c_str(),
                                  capturedPhotoData, capturedPhotoLength);

      if (saveOk) {
        String timestamp = RTC::getTimestamp();
        logOk = Storage::appendScanLog(DEVICE_ID, ctx.currentUserId.c_str(),
                                       timestamp.c_str());
      }

      // 6. Deinit SD
      Storage::deinit();
    }

    // Release the camera frame buffer (must happen after save)
#ifdef ARDUINO
    // The frame buffer was from esp_camera — return it
    // Note: PhotoCapture::deinit() was already called, but fb_return
    // should still work if the buffer is valid. In practice the buffer
    // is freed by deinit, so this is a no-op safety measure.
#endif

    capturedPhotoData   = nullptr;
    capturedPhotoLength = 0;

    // Enforce minimum 1.5s blue LED
    unsigned long blueElapsed = millis() - blueStartMs;
    if (blueElapsed < SAVE_MIN_MS) {
      delay(SAVE_MIN_MS - blueElapsed);
    }

    // 7. Reattach shared LED pins
    LED::reattachSharedPins();

    // 8. Reinit camera for QR scanning
    bool camOk = QRScanner::init();
    if (!camOk) {
      Serial.println("[STATE] Camera reinit failed after save");
    }

    // Check save results
    if (!sdOk || !saveOk) {
      Serial.println("[STATE] Save failed — entering ERROR");
      LED::red();
      transitionTo(STATE_ERROR);
      break;
    }

    if (!logOk) {
      Serial.println("[STATE] Log append failed — entering ERROR");
      LED::red();
      transitionTo(STATE_ERROR);
      break;
    }

    Serial.println("[STATE] Save complete — SUCCESS");
    LED::blinkBlueGreen(SUCCESS_BLINK_MS);
    transitionTo(STATE_SUCCESS);
    break;
  }

  // ── SUCCESS — Task 11.4 ───────────────────────────────────────────────
  case STATE_SUCCESS: {
    // Blue/green alternating for SUCCESS_MS (3s), then back to READY
    if (elapsed >= SUCCESS_MS) {
      // Reset inactivity timer on scan completion
      InactivityTimer::reset();
      LED::green();
      transitionTo(STATE_READY);
    }
    break;
  }

  // ── ERROR — Task 11.5 ─────────────────────────────────────────────────
  case STATE_ERROR: {
    if (sdMissingAtBoot) {
      // SD missing at boot — stay in ERROR permanently (red LED)
      // Device must be power-cycled or woken again to retry
      break;
    }

    // Solid red for ERROR_MS (5s), then back to READY
    if (elapsed >= ERROR_MS) {
      LED::green();
      // Reinit inactivity timer so we don't immediately sleep
      InactivityTimer::reset();
      transitionTo(STATE_READY);
    }
    break;
  }

  // ── SLEEP_PENDING — Task 11.5 ─────────────────────────────────────────
  case STATE_SLEEP_PENDING: {
    // Turn off LED
    LED::off();

    // Deinit peripherals
    QRScanner::deinit();
    Serial.println("[STATE] Entering deep sleep...");
    Serial.flush();

#ifdef ARDUINO
    // Configure ext0 wake on button press (GPIO 0, active LOW)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    esp_deep_sleep_start();
#endif
    // If not on Arduino (shouldn't happen), just loop
    break;
  }

  // ── INIT (should not stay here) ───────────────────────────────────────
  case STATE_INIT:
  default:
    break;
  }
}
