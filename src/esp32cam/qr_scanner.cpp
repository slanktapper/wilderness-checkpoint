#ifdef ARDUINO

#include "qr_scanner.h"
#include "uart_protocol.h"

#include <esp_camera.h>
#include <quirc.h>
#include <string.h>

// ── AI-Thinker ESP32-CAM pin configuration ──────────────────────

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ── Static quirc instance ───────────────────────────────────────

static struct quirc* qr = nullptr;

// ── Public API ──────────────────────────────────────────────────

bool qrScannerInit() {
    camera_config_t config;
    memset(&config, 0, sizeof(config));

    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;
    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;

    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d0 = Y2_GPIO_NUM;

    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href  = HREF_GPIO_NUM;
    config.pin_pclk  = PCLK_GPIO_NUM;

    config.xclk_freq_hz = 20000000;
    config.ledc_timer   = LEDC_TIMER_0;
    config.ledc_channel = LEDC_CHANNEL_0;

    config.pixel_format = PIXFORMAT_GRAYSCALE;
    config.frame_size   = FRAMESIZE_QVGA;  // 320x240
    config.fb_count     = 1;
    config.grab_mode    = CAMERA_GRAB_LATEST;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        return false;
    }

    // Create quirc decoder instance
    qr = quirc_new();
    if (qr == nullptr) {
        esp_camera_deinit();
        return false;
    }

    if (quirc_resize(qr, 320, 240) < 0) {
        quirc_destroy(qr);
        qr = nullptr;
        esp_camera_deinit();
        return false;
    }

    return true;
}

QrScanResult qrScanOneFrame() {
    QrScanResult scanResult;
    scanResult.result = QrResult::NO_QR;
    scanResult.userId[0] = '\0';

    // Capture a frame
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb == nullptr) {
        scanResult.result = QrResult::CAM_ERROR;
        return scanResult;
    }

    // Copy frame data into quirc buffer
    int w = 0;
    int h = 0;
    uint8_t* qrBuf = quirc_begin(qr, &w, &h);

    // Copy the grayscale image — sizes should match (320x240)
    size_t copyLen = (size_t)w * (size_t)h;
    if (fb->len < copyLen) {
        copyLen = fb->len;
    }
    memcpy(qrBuf, fb->buf, copyLen);

    quirc_end(qr);

    // Release the camera frame buffer
    esp_camera_fb_return(fb);

    // Check for decoded QR codes
    int count = quirc_count(qr);
    for (int i = 0; i < count; i++) {
        struct quirc_code code;
        struct quirc_data data;

        quirc_extract(qr, i, &code);
        quirc_decode_error_t decErr = quirc_decode(&code, &data);

        if (decErr != QUIRC_SUCCESS) {
            continue;  // decode error, try next code if any
        }

        // Null-terminate the payload for string operations
        // payload_len does not include a null terminator
        char payload[QUIRC_MAX_PAYLOAD + 1];
        size_t pLen = (size_t)data.payload_len;
        if (pLen > QUIRC_MAX_PAYLOAD) {
            pLen = QUIRC_MAX_PAYLOAD;
        }
        memcpy(payload, data.payload, pLen);
        payload[pLen] = '\0';

        if (isValidUserId(payload)) {
            scanResult.result = QrResult::FOUND_VALID;
            memcpy(scanResult.userId, payload, 4);
            scanResult.userId[4] = '\0';
        } else {
            scanResult.result = QrResult::FOUND_INVALID;
        }

        // Return on first decoded QR code
        return scanResult;
    }

    return scanResult;  // NO_QR — no codes found in this frame
}

void qrScannerDeinit() {
    if (qr != nullptr) {
        quirc_destroy(qr);
        qr = nullptr;
    }
    esp_camera_deinit();
}

#endif // ARDUINO
