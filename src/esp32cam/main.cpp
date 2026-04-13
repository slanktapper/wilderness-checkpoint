// ESP32-CAM main — command loop for Wilderness QR Checkpoint
// Handles: boot init, QR scanning, photo capture, SD save, shutdown
// Requirements: 1.2, 2.3, 3.1, 3.4, 4.1, 4.2, 5.1, 5.7, 10.1–10.5, 11.5

#include <Arduino.h>
#include "uart_protocol.h"
#include "qr_scanner.h"
#include "photo_capture.h"
#include "sd_storage.h"
#include "file_paths.h"

// ── Static state ────────────────────────────────────────────────

// Photo buffer held between CMD:CAPTURE and CMD:SAVE
static const uint8_t* s_jpegData = nullptr;
static size_t          s_jpegSize = 0;

// UART read buffer
static char s_cmdBuf[UART_MAX_MSG_LEN];

// ── Forward declarations ────────────────────────────────────────
static void handleScan();
static void handleCapture();
static void handleSave(const ParsedCommand& cmd);

// ── setup() ─────────────────────────────────────────────────────
// Boot sequence: SD verify → SD deinit → camera init → RSP:INIT_OK
// On failure at any stage, send the appropriate error and halt.

void setup() {
    uartInit();

    // 10.1: Verify SD card is present and writable, then release GPIOs
    if (!sdVerify()) {
        sendResponse("RSP:SD_ERR");
        // Halt — Nano will handle error state
        while (true) { delay(1000); }
    }
    // sdVerify already unmounts internally, but call sdDeinit for safety
    sdDeinit();

    // 10.2: Initialize camera in QR scanning mode (QVGA grayscale)
    if (!qrScannerInit()) {
        sendResponse("RSP:CAM_ERR");
        while (true) { delay(1000); }
    }

    // 10.3: Both subsystems OK — tell Nano we're ready
    sendResponse("RSP:INIT_OK");
}

// ── loop() ──────────────────────────────────────────────────────
// Read UART command, dispatch based on parseCommand() result.

void loop() {
    // Block until a command arrives (generous timeout — Nano drives timing)
    if (!readCommand(s_cmdBuf, sizeof(s_cmdBuf), 5000)) {
        return; // No command yet, keep waiting
    }

    ParsedCommand cmd = parseCommand(s_cmdBuf);

    switch (cmd.command) {

    // ── CMD:SCAN ────────────────────────────────────────────────
    // 3.1, 3.4: Continuously scan frames until QR found or CMD:STOP
    case NanoCommand::SCAN:
        handleScan();
        break;

    // ── CMD:CAPTURE ─────────────────────────────────────────────
    // 4.1, 4.2: Deinit QR camera, init photo camera, capture one frame
    case NanoCommand::CAPTURE:
        handleCapture();
        break;

    // ── CMD:SAVE ────────────────────────────────────────────────
    // 5.1, 5.7: Deinit photo camera, mount SD, save photo + log,
    //           deinit SD, reinit QR camera
    case NanoCommand::SAVE:
        handleSave(cmd);
        break;

    // ── CMD:SHUTDOWN ────────────────────────────────────────────
    // 2.3: Flush and signal ready for power cut
    case NanoCommand::SHUTDOWN:
        Serial.flush();
        sendResponse("RSP:SHUTDOWN_READY");
        // Stay in tight loop — Nano will cut power
        while (true) { delay(1000); }
        break;

    // ── CMD:STOP (outside scan context — ignore) ────────────────
    case NanoCommand::STOP:
        // STOP only meaningful during scan; if received here, ignore
        break;

    // ── Unknown / malformed ─────────────────────────────────────
    // 11.5: Respond with parse error
    case NanoCommand::UNKNOWN:
    default:
        sendResponse("RSP:ERR:PARSE");
        break;
    }
}


// ── Handler: CMD:SCAN ───────────────────────────────────────────
// Continuously capture frames and attempt QR decode.
// Between frames, check for incoming CMD:STOP to abort.
// On valid QR: send RSP:QR:YYYY and return.
// On invalid QR: send RSP:QR_INVALID and return.
// On camera error: send RSP:CAM_ERR and return.

static void handleScan() {
    for (;;) {
        QrScanResult res = qrScanOneFrame();

        switch (res.result) {
        case QrResult::FOUND_VALID:
            // 3.2: Send decoded user ID to Nano
            {
                char rsp[20]; // "RSP:QR:YYYY" = 11 chars + null
                snprintf(rsp, sizeof(rsp), "RSP:QR:%s", res.userId);
                sendResponse(rsp);
            }
            return;

        case QrResult::FOUND_INVALID:
            // 3.3: QR decoded but content not a valid user ID
            sendResponse("RSP:QR_INVALID");
            return;

        case QrResult::CAM_ERROR:
            sendResponse("RSP:CAM_ERR");
            return;

        case QrResult::NO_QR:
            // 3.4: No QR in this frame — check for CMD:STOP, then continue
            if (Serial.available()) {
                char stopBuf[UART_MAX_MSG_LEN];
                if (readCommand(stopBuf, sizeof(stopBuf), 0)) {
                    ParsedCommand inner = parseCommand(stopBuf);
                    if (inner.command == NanoCommand::STOP) {
                        return; // Abort scan loop
                    }
                }
            }
            break; // Continue scanning next frame
        }
    }
}

// ── Handler: CMD:CAPTURE ────────────────────────────────────────
// Deinit QR camera, init photo camera, capture one JPEG frame.
// Hold the buffer pointer for the subsequent CMD:SAVE.

static void handleCapture() {
    // Release any previously held photo buffer
    if (s_jpegData != nullptr) {
        photoRelease();
        s_jpegData = nullptr;
        s_jpegSize = 0;
    }

    // Camera and SD share GPIOs — deinit QR camera first
    qrScannerDeinit();

    // 4.1: Reinitialize camera in VGA JPEG mode
    if (!photoCaptureInit()) {
        sendResponse("RSP:CAM_ERR");
        return;
    }

    // 4.2: Capture a single photo, hold in memory
    s_jpegData = photoCaptureOne(&s_jpegSize);
    if (s_jpegData == nullptr) {
        sendResponse("RSP:CAM_ERR");
        return;
    }

    sendResponse("RSP:PHOTO_READY");
}

// ── Handler: CMD:SAVE ───────────────────────────────────────────
// Deinit photo camera, mount SD, save photo + append log,
// deinit SD, reinit QR camera for next scan cycle.

static void handleSave(const ParsedCommand& cmd) {
    // Deinit photo camera to free GPIOs for SD
    photoCaptureDeinit();

    // Mount SD and create device directory
    if (!sdInit(cmd.deviceId)) {
        // Release photo buffer before reinit
        if (s_jpegData != nullptr) {
            s_jpegData = nullptr;
            s_jpegSize = 0;
        }
        sendResponse("RSP:STORAGE_ERR");
        // Attempt to reinit camera for recovery
        qrScannerInit();
        return;
    }

    // Save JPEG photo (overwrites if exists per Req 4.4)
    bool photoOk = false;
    if (s_jpegData != nullptr && s_jpegSize > 0) {
        photoOk = sdSavePhoto(cmd.deviceId, cmd.userId,
                              s_jpegData, s_jpegSize);
    }

    // Release photo buffer — no longer needed after save
    s_jpegData = nullptr;
    s_jpegSize = 0;

    if (!photoOk) {
        sdDeinit();
        sendResponse("RSP:STORAGE_ERR");
        qrScannerInit();
        return;
    }

    // Append scan log row (Req 5.1, 5.3)
    if (!sdAppendLog(cmd.deviceId, cmd.userId, cmd.timestamp)) {
        sdDeinit();
        sendResponse("RSP:STORAGE_ERR");
        qrScannerInit();
        return;
    }

    // 5.7: Unmount SD, reinit camera in QR mode for next scan
    sdDeinit();

    if (!qrScannerInit()) {
        // Camera reinit failed — report but save was successful
        sendResponse("RSP:SAVE_OK");
        return;
    }

    sendResponse("RSP:SAVE_OK");
}
