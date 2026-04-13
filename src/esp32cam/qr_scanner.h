#ifndef QR_SCANNER_H
#define QR_SCANNER_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#endif

// QR scan result classification
enum class QrResult : uint8_t {
    FOUND_VALID,    // Valid user_id decoded
    FOUND_INVALID,  // QR decoded but not a valid user_id
    NO_QR,          // No QR code in frame
    CAM_ERROR       // Camera capture failed
};

struct QrScanResult {
    QrResult result;
    char userId[5];  // Populated only when result == FOUND_VALID
};

#ifdef ARDUINO

// Initialize camera in QVGA grayscale mode for QR scanning.
// Configures OV2640 with AI-Thinker pin mapping.
// Returns true on success.
bool qrScannerInit();

// Capture one frame and attempt QR decode using quirc.
// Non-blocking per frame; caller loops until QR found or stop.
QrScanResult qrScanOneFrame();

// Deinitialize camera to release GPIO pins (required before SD access).
void qrScannerDeinit();

#endif // ARDUINO

#endif // QR_SCANNER_H
