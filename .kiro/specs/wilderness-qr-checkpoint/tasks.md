# Implementation Plan: Wilderness QR Checkpoint

## Overview

Two-board PlatformIO project (Arduino Nano + ESP32-CAM) with a native test environment for host-based property and unit testing of pure logic modules. Implementation proceeds bottom-up: project scaffolding → shared pure-logic modules (testable on host) → Nano firmware → ESP32-CAM firmware → integration wiring.

## Tasks

- [x] 1. Project scaffolding and build configuration
  - [x] 1.1 Create `platformio.ini` with three environments: `nano` (ATmega328P), `esp32cam` (AI-Thinker ESP32-CAM), and `native` (host-based Google Test + RapidCheck)
    - `nano` env: board = nanoatmega328, framework = arduino, lib_deps = adafruit/RTClib, monitor_speed = 115200
    - `esp32cam` env: board = esp32cam, framework = arduino, lib_deps = adafruit/RTClib, board_build.partitions = huge_app.csv
    - `native` env: platform = native, test_build_src = yes, build_src_filter = exclude nano/main.cpp and esp32cam/main.cpp, lib_deps = google/googletest, emil-e/rapidcheck, build_flags include rapidcheck gtest extras include path and `-Wl,-subsystem,console` for Windows/MinGW
    - _Requirements: 1.1, 1.2_
  - [x] 1.2 Create source directory structure: `src/nano/`, `src/esp32cam/`, and stub `main.cpp` files for each board
    - Nano `main.cpp`: empty `setup()` and `loop()` with includes
    - ESP32-CAM `main.cpp`: empty `setup()` and `loop()` with includes
    - _Requirements: 1.1, 1.2_
  - [x] 1.3 Verify `lib/quirc_lib/` has the quirc source and a valid `library.json`; re-clone from https://github.com/dlbeer/quirc if missing
    - `library.json` must specify name, version, and `"frameworks": "*"` so PlatformIO picks it up
    - Ensure `src_filter` in library.json includes only `lib/*.c` and `lib/*.h` (exclude demo/tests)
    - _Requirements: 3.1_
  - [x] 1.4 Create `test/test_placeholder.cpp` with a Google Test `main()` entry point and one trivial passing test
    - This file provides `main()` for the native test runner
    - Verify the native test environment compiles and runs: `pio test -e native --verbose`
    - _Requirements: (testing infrastructure)_

- [x] 2. Checkpoint — Verify all three PlatformIO environments compile
  - Ensure `pio run -e nano`, `pio run -e esp32cam`, and `pio test -e native` all succeed. Ask the user if questions arise.

- [x] 3. Implement shared UART protocol module (pure logic, testable on host)
  - [x] 3.1 Create `src/nano/uart_protocol.h` and `src/nano/uart_protocol.cpp` — Nano-side UART formatting and parsing
    - `parseResponse(const char* line)` → `ParsedResponse` struct with `EspResponse` enum and optional data payload
    - `formatSaveCommand(char* buf, uint8_t bufSize, const char* userId, const char* deviceId, const char* timestamp)` — builds `CMD:SAVE:uid:did:ts` string
    - Use `#ifdef ARDUINO` guards: under Arduino, `uartInit()` calls `Serial.begin(115200)`, `sendCommand()` writes to `Serial`; under native, these are stubs or excluded
    - _Requirements: 1.4, 1.5, 11.1, 11.2, 11.4_
  - [x] 3.2 Create `src/esp32cam/uart_protocol.h` and `src/esp32cam/uart_protocol.cpp` — ESP32-CAM-side UART formatting and parsing
    - `parseCommand(const char* line)` → `ParsedCommand` struct with `NanoCommand` enum and userId/deviceId/timestamp fields
    - `isValidUserId(const char* userId)` — validates 4-digit string in range 0001–1000
    - `sendResponse(const char* response)` — formats and sends response line
    - Use `#ifdef ARDUINO` guards for hardware Serial calls
    - _Requirements: 3.2, 3.3, 11.3, 11.4, 11.5_
  - [x] 3.3 Write property test: UART Protocol Message Format (Property 9)
    - **Property 9: UART Protocol Message Format**
    - For any valid command/response produced by formatting functions: (a) terminated by `\n`, (b) printable ASCII + newline only, (c) starts with `CMD:` or `RSP:`, (d) ≤64 bytes total
    - Use RapidCheck generators for valid userId, deviceId, timestamp strings
    - **Validates: Requirements 11.1, 11.2, 11.3, 11.4**
  - [x] 3.4 Write property test: Malformed Command Rejection (Property 10)
    - **Property 10: Malformed Command Rejection**
    - For any string not matching a valid command format, `parseCommand()` returns UNKNOWN
    - Use RapidCheck `gen::arbitrary<std::string>()` filtered to exclude valid command patterns
    - **Validates: Requirements 11.5**
  - [x] 3.5 Write property test: User ID Validation (Property 3)
    - **Property 3: User ID Validation**
    - `isValidUserId` returns true iff string is exactly 4 ASCII digits with numeric value 1–1000
    - Generate arbitrary strings and valid user IDs; verify acceptance/rejection
    - **Validates: Requirements 3.2, 3.3**

- [x] 4. Implement User ID validation, file path construction, and CSV formatting (pure logic, testable on host)
  - [x] 4.1 Add file path helper functions (can live in `src/esp32cam/sd_storage.h/.cpp` or a separate `src/esp32cam/file_paths.h/.cpp`)
    - `buildPhotoPath(char* buf, uint8_t bufSize, const char* deviceId, const char* userId)` → `/DEVICEXX/DEVICEXX_USERYYYY.jpg`
    - `buildLogPath(char* buf, uint8_t bufSize, const char* deviceId)` → `/DEVICEXX/scan_log.csv`
    - `buildDirPath(char* buf, uint8_t bufSize, const char* deviceId)` → `/DEVICEXX`
    - Pure string functions, no hardware dependency
    - _Requirements: 5.2, 6.2, 6.3_
  - [x] 4.2 Add CSV row formatting function
    - `formatCsvRow(char* buf, uint8_t bufSize, const char* userId, const char* deviceId, const char* timestamp)` → `"0042,07,2025-06-15T14:30:22\n"`
    - `csvHeader()` → `"user_id,device_id,timestamp\n"`
    - Pure string functions, no hardware dependency
    - _Requirements: 5.1, 5.3, 5.4_
  - [x] 4.3 Write property test: File Path Construction (Property 6)
    - **Property 6: File Path Construction**
    - For any device_id 1–31 and user_id 0001–1000, photo path matches `/DEVICEXX/DEVICEXX_USERYYYY.jpg`, log path matches `/DEVICEXX/scan_log.csv`, all paths within `/DEVICEXX/`
    - **Validates: Requirements 5.2, 6.2, 6.3, 9.5**
  - [x] 4.4 Write property test: CSV Row Correctness (Property 5)
    - **Property 5: CSV Row Correctness**
    - For any valid save parameters, the formatted CSV row contains exact userId, deviceId, timestamp in correct column order with no truncation
    - **Validates: Requirements 5.1, 5.3, 5.4**

- [x] 5. Implement DIP switch config reader and timestamp formatting (pure logic, testable on host)
  - [x] 5.1 Create `src/nano/config_reader.h` and `src/nano/config_reader.cpp`
    - `configReadDeviceId(uint8_t pinValues)` — pure logic overload that takes a 5-bit value (already inverted) and returns device ID 0–31 (0 = invalid)
    - `configFormatDeviceId(uint8_t deviceId, char* buffer)` — formats as 2-digit zero-padded string
    - Hardware-dependent `configInit()` and `configReadDeviceId()` (no-arg, reads pins) guarded with `#ifdef ARDUINO`
    - _Requirements: 9.1, 9.2, 9.3, 9.5_
  - [x] 5.2 Create `src/nano/rtc_manager.h` and `src/nano/rtc_manager.cpp`
    - `formatTimestamp(char* buf, uint8_t bufSize, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)` — pure logic ISO 8601 formatter
    - `rtcFallbackTimestamp()` → `"0000-00-00T00:00:00"`
    - Hardware-dependent `rtcInit()` and `rtcGetTimestamp()` guarded with `#ifdef ARDUINO`
    - _Requirements: 7.1, 7.2, 7.3_
  - [x] 5.3 Write property test: DIP Switch to Device ID Conversion (Property 8)
    - **Property 8: DIP Switch to Device ID Conversion**
    - For any 5-bit input (0–31), device_id equals the numeric value; 0 is invalid, 1–31 are valid
    - **Validates: Requirements 9.2**
  - [x] 5.4 Write property test: ISO 8601 Timestamp Formatting (Property 7)
    - **Property 7: ISO 8601 Timestamp Formatting**
    - For any valid DateTime (year 2000–2099, valid month/day/hour/minute/second), output is exactly 19 chars matching `YYYY-MM-DDTHH:MM:SS` with zero-padded fields and literal `T`
    - **Validates: Requirements 7.3**

- [x] 6. Checkpoint — Verify all pure-logic modules compile and property tests pass
  - Run `pio test -e native --verbose`. Ensure all tests pass, ask the user if questions arise.

- [x] 7. Implement Nano LED controller (hardware-dependent)
  - [x] 7.1 Create `src/nano/led_controller.h` and `src/nano/led_controller.cpp`
    - `ledInit()` — configure pins D9/D10/D11 as OUTPUT, turn off
    - `ledSolid(LedColor)` — set solid color via analogWrite
    - `ledOff()` — all channels to 0
    - `ledUpdate()` — non-blocking pattern state machine (call from `loop()`)
    - `ledStartCountdown(uint16_t durationMs)` — accelerating white/green blink
    - `ledStartSuccessPattern(uint16_t durationMs)` — alternating blue/green blink
    - `ledStartErrorBlink()` — blinking red for invalid DIP config
    - `ledPatternActive()` — returns true if timed pattern running
    - _Requirements: 8.1, 8.2, 8.3, 8.4, 8.5, 8.6, 8.7, 8.8_

- [x] 8. Implement Nano power manager (hardware-dependent)
  - [x] 8.1 Create `src/nano/power_manager.h` and `src/nano/power_manager.cpp`
    - `powerInit()` — D8 OUTPUT HIGH (ESP off), D2 INPUT_PULLUP
    - `espPowerOn()` — D8 LOW
    - `espPowerOff()` — D8 HIGH
    - `enterDeepSleep()` — set sleep mode SLEEP_MODE_PWR_DOWN, attach INT0 interrupt, sleep_cpu
    - `attachWakeInterrupt()` — falling edge on D2
    - _Requirements: 2.1, 2.4, 2.6, 2.8_

- [x] 9. Implement Nano main state machine
  - [x] 9.1 Implement `src/nano/main.cpp` — full Nano state machine
    - States: DEEP_SLEEP, BOOTING, WAIT_ESP_INIT, READY, SCANNING, QR_DECODED, COUNTDOWN, CAPTURING, SAVING, SCAN_COMPLETE, SHUTDOWN, ERROR
    - `setup()`: powerInit, configInit, read DIP, validate device ID (halt with blinking red if 0), espPowerOn, rtcInit, uartInit, ledSolid green on INIT_OK
    - `loop()`: state machine switch, inactivity timer check, ledUpdate call
    - Send UART commands at each state transition per the protocol sequence diagram
    - Handle all error responses with LED red feedback and state transitions per error handling table
    - _Requirements: 1.1, 1.4, 1.5, 2.1, 2.2, 2.4, 2.5, 3.1, 4.1, 5.1, 7.2, 7.3, 8.1–8.8, 9.1–9.6, 10.6, 10.7_
  - [x] 9.2 Write property test: Inactivity Timeout with Scan Reset (Property 2)
    - **Property 2: Inactivity Timeout with Scan Reset**
    - Extract inactivity timer logic into a pure function: `shouldShutdown(uint32_t lastEventTime, uint32_t now, uint32_t timeoutMs)` → bool
    - For any sequence of scan timestamps, shutdown triggers iff elapsed > 60s since last event
    - **Validates: Requirements 2.2, 2.5**
  - [x] 9.3 Write property test: UART Command Retry on Timeout (Property 1)
    - **Property 1: UART Command Retry on Timeout**
    - Extract retry logic into a pure function testable on host: given a sequence of attempt outcomes (success/timeout), verify retry count ≤ 2 additional attempts, and first success is returned
    - **Validates: Requirements 1.5**

- [x] 10. Checkpoint — Verify Nano firmware compiles
  - Run `pio run -e nano`. Ensure it compiles cleanly, ask the user if questions arise.

- [x] 11. Implement ESP32-CAM QR scanner module (hardware-dependent)
  - [x] 11.1 Create `src/esp32cam/qr_scanner.h` and `src/esp32cam/qr_scanner.cpp`
    - `qrScannerInit()` — configure OV2640 for QVGA grayscale (PIXFORMAT_GRAYSCALE, FRAMESIZE_QVGA), return success/failure
    - `qrScanOneFrame()` — capture frame with `esp_camera_fb_get()`, run quirc identify+decode, return `QrScanResult` with userId if valid
    - `qrScannerDeinit()` — `esp_camera_deinit()` to release GPIO for SD
    - Use quirc API: `quirc_new`, `quirc_resize`, `quirc_begin`, memcpy frame, `quirc_end`, `quirc_count`, `quirc_extract`, `quirc_decode`
    - Call `isValidUserId()` on decoded data to classify FOUND_VALID vs FOUND_INVALID
    - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5_

- [x] 12. Implement ESP32-CAM photo capture module (hardware-dependent)
  - [x] 12.1 Create `src/esp32cam/photo_capture.h` and `src/esp32cam/photo_capture.cpp`
    - `photoCaptureInit()` — configure OV2640 for VGA JPEG (PIXFORMAT_JPEG, FRAMESIZE_VGA, quality 12)
    - `photoCaptureOne(size_t* jpegSize)` — `esp_camera_fb_get()`, return buffer pointer and size
    - `photoRelease()` — `esp_camera_fb_return()`
    - `photoCaptureDeinit()` — `esp_camera_deinit()`
    - _Requirements: 4.1, 4.2, 4.3, 4.5_

- [x] 13. Implement ESP32-CAM SD storage module (hardware-dependent)
  - [x] 13.1 Create `src/esp32cam/sd_storage.h` and `src/esp32cam/sd_storage.cpp`
    - `sdVerify()` — mount SD_MMC, check writable, unmount; return success/failure
    - `sdInit(const char* deviceId)` — mount SD_MMC, create `/DEVICEXX/` directory if needed
    - `sdSavePhoto(deviceId, userId, jpegData, jpegSize)` — write JPEG to `/DEVICEXX/DEVICEXX_USERYYYY.jpg` (overwrite if exists)
    - `sdAppendLog(deviceId, userId, timestamp)` — open `/DEVICEXX/scan_log.csv` in append mode, write header if new file, append CSV row using `formatCsvRow()`
    - `sdDeinit()` — unmount SD_MMC
    - Use `buildPhotoPath()`, `buildLogPath()`, `buildDirPath()` from file_paths module
    - _Requirements: 4.4, 5.1, 5.2, 5.3, 5.5, 5.6, 5.7, 5.8, 6.1, 6.2, 6.3_
  - [x] 13.2 Write property test: Rescan Overwrites Photo but Appends Log (Property 4)
    - **Property 4: Rescan Overwrites Photo but Appends Log**
    - Test the pure logic: for N saves of the same userId, `buildPhotoPath` always produces the same single path (overwrite semantics), while `formatCsvRow` produces N distinct rows (append semantics). Verify path uniqueness per userId and row count == N.
    - **Validates: Requirements 4.4, 5.5**

- [x] 14. Implement ESP32-CAM main command dispatcher
  - [x] 14.1 Implement `src/esp32cam/main.cpp` — full ESP32-CAM command loop
    - `setup()`: sdVerify → sdDeinit → qrScannerInit → send RSP:INIT_OK (or error responses on failure)
    - `loop()`: read UART command, dispatch based on `parseCommand()` result
    - CMD:SCAN → loop `qrScanOneFrame()` until QR found or CMD:STOP received, send RSP:QR:YYYY or RSP:QR_INVALID
    - CMD:CAPTURE → `qrScannerDeinit()`, `photoCaptureInit()`, `photoCaptureOne()`, send RSP:PHOTO_READY or RSP:CAM_ERR
    - CMD:SAVE → `photoCaptureDeinit()`, `sdInit(deviceId)`, `sdSavePhoto()`, `sdAppendLog()`, `sdDeinit()`, reinit camera, send RSP:SAVE_OK or RSP:STORAGE_ERR
    - CMD:SHUTDOWN → flush, send RSP:SHUTDOWN_READY
    - UNKNOWN → send RSP:ERR:PARSE
    - _Requirements: 1.2, 2.3, 3.1, 3.4, 4.1, 4.2, 5.1, 5.7, 10.1, 10.2, 10.3, 10.4, 10.5, 11.5_

- [x] 15. Checkpoint — Verify ESP32-CAM firmware compiles
  - Run `pio run -e esp32cam`. Ensure it compiles cleanly, ask the user if questions arise.

- [x] 16. Wire Nano ↔ ESP32-CAM communication and final integration
  - [x] 16.1 Verify UART protocol round-trip between Nano and ESP32-CAM code paths
    - Ensure Nano's `formatSaveCommand()` output is correctly parsed by ESP32-CAM's `parseCommand()`
    - Ensure ESP32-CAM's `sendResponse()` output is correctly parsed by Nano's `parseResponse()`
    - Verify the Device ID is sent from Nano to ESP32-CAM during boot (as part of the save command flow)
    - _Requirements: 1.3, 1.4, 9.4, 11.1–11.4_
  - [x] 16.2 Write unit tests for save command round-trip
    - Format a CMD:SAVE on Nano side, parse on ESP32-CAM side, verify all fields match
    - Test boundary values: userId "0001"/"1000", deviceId "01"/"31", max-length timestamp
    - _Requirements: 1.4, 11.2, 11.3_

- [x] 17. Final checkpoint — Ensure all environments compile and all native tests pass
  - Run `pio run -e nano`, `pio run -e esp32cam`, and `pio test -e native --verbose`. Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- Checkpoints ensure incremental validation
- Property tests validate universal correctness properties from the design document
- Unit tests validate specific examples and edge cases
- All pure-logic modules use `#ifdef ARDUINO` guards so the same source compiles for both target boards and the native test host
- The native test environment excludes `main.cpp` files from both board targets via `build_src_filter`
- Hardware-dependent modules (camera, SD, MOSFET, LED, RTC hardware access, sleep) are only compiled for their target board environments
