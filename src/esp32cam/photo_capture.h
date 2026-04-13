#ifndef PHOTO_CAPTURE_H
#define PHOTO_CAPTURE_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#include <cstddef>
#endif

#ifdef ARDUINO

// Initialize camera in VGA JPEG mode for photo capture.
// Configures OV2640 with AI-Thinker pin mapping.
// Returns true on success.
bool photoCaptureInit();

// Capture a single JPEG photo. Returns pointer to JPEG buffer
// and sets jpegSize. Caller must call photoRelease() after use.
// Returns nullptr on failure.
const uint8_t* photoCaptureOne(size_t* jpegSize);

// Release the captured photo buffer.
void photoRelease();

// Deinitialize camera to release GPIO pins (required before SD access).
void photoCaptureDeinit();

#endif // ARDUINO

#endif // PHOTO_CAPTURE_H
