#ifndef QR_SCANNER_H
#define QR_SCANNER_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <string>
typedef std::string String;
#endif

/// Result of a single QR scan attempt.
struct ScanResult {
  bool found;       // true if a QR code was detected in the frame
  String userId;    // Raw decoded string from the QR code
  bool valid;       // true if userId matches the expected 4-digit format
};

namespace QRScanner {

/// Initialize OV2640 camera for QR scanning (stub — implemented in Task 9).
bool init();

/// Release camera resources (stub — implemented in Task 9).
void deinit();

/// Grab frame and attempt QR decode (stub — implemented in Task 9).
ScanResult scan();

/// Validate that id is a 4-digit zero-padded string in range 0001–1000.
bool isValidUserId(const String& id);

} // namespace QRScanner

#endif // QR_SCANNER_H
