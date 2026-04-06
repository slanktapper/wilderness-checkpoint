# Implementation Plan: Wilderness QR Checkpoint Device

## Overview

Incremental implementation of the ESP32-CAM wilderness checkpoint firmware using Arduino framework (C/C++). Tasks build sequentially: core utility modules first, then hardware interface modules, then the state machine that wires everything together. Host-based tests use Google Test + RapidCheck. Requirements 11–12 (Phone App) are out of scope.

## Tasks

- [x] 1. Set up project structure and configuration
  - Create PlatformIO project structure with `src/`, `include/`, `test/` directories
  - Create `include/config.h` with all compile-time constants (DEVICE_ID, MAX_USERS, PHOTO_QUALITY, timing constants, GPIO pin assignments)
  - Create `include/device_state.h` with `DeviceState` enum and `DeviceContext` struct
  - Configure `platformio.ini` for ESP32-CAM (AI-Thinker) board with Arduino framework
  - Configure a `native` test environment in `platformio.ini` for host-based Google Test + RapidCheck
  - _Requirements: 9.1, 9.2, 9.3, 10.1_

- [ ] 2. Implement Inactivity Timer module
  - [x] 2.1 Implement `InactivityTimer` namespace in `include/inactivity_timer.h` and `src/inactivity_timer.cpp`
    - `init(unsigned long timeoutMs)`, `reset()`, `isExpired()` using `millis()` comparison
    - Default timeout 60000ms
    - _Requirements: 6.1, 6.2_

  - [x] 2.2 Write property test for Inactivity Timer (Property 6)
    - **Property 6: Inactivity timer correctness**
    - Generate random timeout durations and elapsed times; assert `isExpired()` returns false when elapsed < timeout and true when elapsed >= timeout; assert `reset()` restarts the countdown
    - **Validates: Requirements 6.1, 6.2**

- [ ] 3. Implement QR Scanner module (validation logic)
  - [x] 3.1 Implement `isValidUserId()` in `include/qr_scanner.h` and `src/qr_scanner.cpp`
    - Validate 4-digit zero-padded string, digits only, range 0001–1000
    - Define `ScanResult` struct with `found`, `userId`, `valid` fields
    - _Requirements: 2.2, 2.3_

  - [x] 3.2 Write property test for User_ID validation (Property 1)
    - **Property 1: User_ID validation accepts only correctly formatted IDs**
    - Generate arbitrary strings; assert `isValidUserId()` returns true iff string is exactly 4 digits representing 0001–1000
    - **Validates: Requirements 2.2**

  - [x] 3.3 Write unit tests for User_ID validation edge cases
    - Test known-good IDs ("0001", "0500", "1000") and known-bad inputs ("", "0000", "1001", "abcd", "00001", "1", "999", " 042")
    - _Requirements: 2.2, 2.3_

- [ ] 4. Implement Storage Manager module
  - [x] 4.1 Implement file path construction helpers in `include/storage_manager.h` and `src/storage_manager.cpp`
    - `buildPhotoPath(deviceId, userId)` returns `/DEVICE{XX}/DEVICE{XX}_USER{XXXX}.jpg`
    - `buildLogPath(deviceId)` returns `/DEVICE{XX}/scan_log.csv`
    - `formatCsvRow(userId, deviceId, timestamp)` returns formatted CSV string
    - `parseCsvRow(csvLine)` parses a CSV row back into fields
    - `ensureDirectory(deviceId)` creates `/DEVICEXX/` if needed
    - _Requirements: 3.4, 3.5, 4.1, 4.2, 9.1, 13.1, 13.2_

  - [x] 4.2 Write property test for file path construction (Property 3)
    - **Property 3: File path construction is deterministic and correctly formatted**
    - Generate valid Device_IDs and User_IDs; assert path matches expected pattern; assert two calls with same inputs produce identical results
    - **Validates: Requirements 3.4, 3.5, 9.1, 13.1**

  - [x] 4.3 Write property test for CSV round-trip (Property 4)
    - **Property 4: CSV scan log record round-trip**
    - Generate valid User_IDs, Device_IDs, and ISO 8601 timestamps; format as CSV; parse back; assert equality
    - **Validates: Requirements 4.1, 4.2, 13.2**

  - [x] 4.4 Write property test for scan log append (Property 5)
    - **Property 5: Scan log append preserves all previous records**
    - Generate a list of N scan records (1–100); append all to an in-memory log; assert log contains exactly N data rows plus header; assert each row matches input
    - **Validates: Requirements 4.3**

  - [x] 4.5 Implement SD card I/O functions
    - `init()` / `deinit()` for SD_MMC in 1-bit mode
    - `isReady()` to check SD present and writable
    - `savePhoto(deviceId, userId, data, length)` to write/overwrite JPEG file
    - `appendScanLog(deviceId, userId, timestamp)` to append CSV row (write header if file is new)
    - _Requirements: 3.3, 3.5, 4.1, 4.2, 4.3, 4.4, 7.3, 13.1, 13.2, 13.3_

- [x] 5. Checkpoint — Ensure all host-based tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 6. Implement LED Controller module
  - [x] 6.1 Implement LED Controller in `include/led_controller.h` and `src/led_controller.cpp`
    - `init()` configuring LEDC PWM channels 0–2 at 5kHz, 8-bit resolution for GPIO 12 (red), GPIO 13 (green), GPIO 33 (blue, inverted logic)
    - `setColor(r, g, b)`, `off()`, `green()`, `red()`, `blue()`, `white()`
    - `detachSharedPins()` / `reattachSharedPins()` for GPIO 12/13 handoff during SD operations
    - _Requirements: 1.2, 2.3, 2.4, 3.2, 3.6, 5.1, 5.2, 6.3, 7.3_

  - [x] 6.2 Implement non-blocking blink patterns and countdown interval calculation
    - `blinkWhiteGreen(intervalMs)` and `blinkBlueGreen(intervalMs)` with `update()` driven by `millis()`
    - `computeBlinkInterval(elapsed, totalMs, startMs, endMs)` — linear interpolation from 500ms to 100ms over 3s
    - _Requirements: 3.1, 5.1_

  - [x] 6.3 Write property test for countdown blink interval (Property 2)
    - **Property 2: Countdown blink interval is monotonically decreasing**
    - Generate pairs of elapsed times (t1, t2) in [0, 3000] where t1 < t2; assert `computeBlinkInterval(t1) >= computeBlinkInterval(t2)`; assert interval at t=0 is 500ms and at t=3000ms is 100ms
    - **Validates: Requirements 3.1**

- [x] 7. Implement RTC Manager module
  - Implement `RTC` namespace in `include/rtc_manager.h` and `src/rtc_manager.cpp`
  - `init()` — initialize I2C on GPIO 14 (SCL) / GPIO 15 (SDA), check DS3231 presence
  - `getTimestamp()` — return ISO 8601 string, or fallback `"0000-00-00T00:00:00"` if RTC unresponsive
  - `isAvailable()` — true if RTC responds on I2C
  - Uses RTClib (Adafruit) for DS3231 communication
  - _Requirements: 8.1, 8.2_

- [x] 8. Implement Photo Capture module
  - Implement `PhotoCapture` namespace in `include/photo_capture.h` and `src/photo_capture.cpp`
  - `init()` — configure OV2640 for VGA (640x480) JPEG, quality tuned to keep files ≤50KB
  - `deinit()` — release camera resources via `esp_camera_deinit()`
  - `capture()` — grab single frame, return `CaptureResult` with JPEG buffer and length
  - Define `CaptureResult` struct with `success`, `data`, `length`
  - _Requirements: 3.2, 3.3, 9.2, 9.3_

- [x] 9. Implement QR Scanner camera integration
  - Implement `QRScanner::init()`, `QRScanner::deinit()`, and `QRScanner::scan()` in `src/qr_scanner.cpp`
  - `init()` — configure OV2640 for lower-resolution grayscale frames suitable for quirc
  - `scan()` — grab frame, convert to grayscale, decode with quirc library, return `ScanResult`
  - `deinit()` — release camera
  - _Requirements: 2.1, 2.2_

- [x] 10. Checkpoint — Ensure all modules compile for ESP32-CAM target
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 11. Implement main state machine
  - [x] 11.1 Implement `setup()` in `src/main.ino`
    - Determine wake cause (ext0 button on GPIO 0)
    - Initialize I2C / RTC, check SD card health, initialize camera, initialize LED, set LED green
    - If SD missing/unwritable → ERROR state with solid red LED
    - Initialize `DeviceContext` struct
    - _Requirements: 1.1, 1.2, 7.3_

  - [x] 11.2 Implement `loop()` state machine — READY and QR_SCANNING states
    - READY: call `QRScanner::scan()` each iteration
    - On valid QR → set `currentUserId`, transition to QR_ACCEPTED
    - On invalid QR → transition to QR_INVALID (red 2s, then back to READY)
    - Check `InactivityTimer::isExpired()` → transition to SLEEP_PENDING
    - _Requirements: 2.1, 2.2, 2.3, 2.4, 6.1, 6.2_

  - [x] 11.3 Implement `loop()` state machine — QR_ACCEPTED, COUNTDOWN, and CAPTURE states
    - QR_ACCEPTED: solid white 1s, then transition to COUNTDOWN
    - COUNTDOWN: call `computeBlinkInterval()` with elapsed time, drive `blinkWhiteGreen()`, after 3s transition to CAPTURE with solid white
    - CAPTURE: call `PhotoCapture::capture()`, transition to SAVING
    - _Requirements: 2.4, 3.1, 3.2_

  - [x] 11.4 Implement `loop()` state machine — SAVING and SUCCESS states
    - SAVING: deinit camera, `LED::detachSharedPins()`, `LED::blue()`, init SD, call `Storage::savePhoto()` and `Storage::appendScanLog()`, enforce minimum 1.5s blue LED, deinit SD, `LED::reattachSharedPins()`, reinit camera
    - On save failure → ERROR state (red 5s)
    - SUCCESS: `blinkBlueGreen(100ms)` for 3s, then back to READY
    - Reset inactivity timer on scan completion
    - _Requirements: 3.3, 3.4, 3.5, 3.6, 4.1, 4.2, 4.3, 4.4, 5.1, 5.2, 6.1_

  - [x] 11.5 Implement `loop()` state machine — ERROR and SLEEP_PENDING states
    - ERROR: solid red LED for configured duration, then transition to READY (or stay in ERROR if SD missing at boot)
    - SLEEP_PENDING: turn off LED, deinit peripherals, enter ESP32 deep sleep with ext0 wake on GPIO 0
    - _Requirements: 1.3, 4.4, 6.2, 6.3, 7.3_

- [x] 12. Final checkpoint — Ensure full firmware compiles and all tests pass
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Requirements 11 and 12 (Phone App) are out of scope for firmware implementation
- Property tests use RapidCheck integrated with Google Test, running on the `native` PlatformIO environment
- Hardware-dependent behavior (camera capture, SD I/O, deep sleep, LED output) is verified on-device; host tests cover pure logic only
- GPIO pin sharing between SD_MMC and LED/I2C is the primary hardware constraint — task 11.4 (SAVING state) is the critical integration point
