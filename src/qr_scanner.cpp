#include "qr_scanner.h"

#ifdef ARDUINO

#include "config.h"
#include "esp_camera.h"
#include "quirc.h"

// ─── AI-Thinker ESP32-CAM Pin Definitions ───────────────────────────────────

#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM       5

#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

// ─── ESP32 / OV2640 + quirc Implementation ──────────────────────────────────

namespace QRScanner {

bool init() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_4;
  config.ledc_timer   = LEDC_TIMER_1;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;  // 20MHz
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size   = FRAMESIZE_QVGA;  // 320x240 — fast for QR decoding
  config.jpeg_quality = 12;
  config.fb_count     = 1;
  config.grab_mode    = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);
  return (err == ESP_OK);
}

void deinit() {
  esp_camera_deinit();
}

ScanResult scan() {
  // 1. Grab a frame
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    return ScanResult{false, "", false};
  }

  // 2. Create quirc decoder
  struct quirc* qr = quirc_new();
  if (!qr) {
    esp_camera_fb_return(fb);
    return ScanResult{false, "", false};
  }

  if (quirc_resize(qr, fb->width, fb->height) < 0) {
    quirc_destroy(qr);
    esp_camera_fb_return(fb);
    return ScanResult{false, "", false};
  }

  // 3. Copy grayscale data into quirc buffer
  uint8_t* image = quirc_begin(qr, NULL, NULL);
  memcpy(image, fb->buf, fb->width * fb->height);
  quirc_end(qr);

  // 4. Check for decoded QR codes
  int count = quirc_count(qr);
  ScanResult result = {false, "", false};

  for (int i = 0; i < count; i++) {
    struct quirc_code code;
    struct quirc_data data;

    quirc_extract(qr, i, &code);
    quirc_decode_error_t err = quirc_decode(&code, &data);
    if (err != QUIRC_SUCCESS) {
      continue;
    }

    // 5. Extract the data string
    String rawString((const char*)data.payload, data.payload_len);

    // 6. Validate with isValidUserId()
    bool valid = isValidUserId(rawString);

    // 7. Return ScanResult
    result = ScanResult{true, rawString, valid};
    break;  // Use the first successfully decoded QR code
  }

  // 8. Cleanup
  quirc_destroy(qr);
  esp_camera_fb_return(fb);

  return result;
}

} // namespace QRScanner

#else

// ─── Native (stub) Implementation ──────────────────────────────────────────

namespace QRScanner {

bool init() {
  return false;
}

void deinit() {
}

ScanResult scan() {
  return ScanResult{false, "", false};
}

} // namespace QRScanner

#endif

// ─── Validation Logic (shared across all platforms) ─────────────────────────

namespace QRScanner {

bool isValidUserId(const String& id) {
  // Must be exactly 4 characters
  if (id.length() != 4) {
    return false;
  }

  // All characters must be ASCII digits
  int numericValue = 0;
  for (size_t i = 0; i < 4; ++i) {
    char c = id[i];
    if (c < '0' || c > '9') {
      return false;
    }
    numericValue = numericValue * 10 + (c - '0');
  }

  // Range check: 1–1000 (i.e., "0001" through "1000")
  return numericValue >= 1 && numericValue <= 1000;
}

} // namespace QRScanner
