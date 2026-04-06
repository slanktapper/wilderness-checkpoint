#ifndef PHOTO_CAPTURE_H
#define PHOTO_CAPTURE_H

#include <cstddef>
#include <cstdint>

/// Result of a single photo capture attempt.
struct CaptureResult {
  bool success;        // true if a frame was captured successfully
  uint8_t* data;       // JPEG buffer (owned by esp_camera — do NOT free manually)
  size_t length;       // JPEG size in bytes
};

namespace PhotoCapture {

/// Initialize OV2640 camera for VGA (640x480) JPEG capture.
/// Camera is configured with AI-Thinker ESP32-CAM pin mapping.
/// JPEG quality is set via PHOTO_QUALITY from config.h to keep files ≤50KB.
/// @return true if camera initialized successfully
bool init();

/// Release camera resources via esp_camera_deinit().
void deinit();

/// Grab a single VGA JPEG frame.
/// The caller is responsible for calling esp_camera_fb_return() on the
/// frame buffer when done with CaptureResult.data.
/// @return CaptureResult with success flag, JPEG buffer pointer, and length
CaptureResult capture();

} // namespace PhotoCapture

#endif // PHOTO_CAPTURE_H
