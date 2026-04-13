#include "rtc_manager.h"

// ── Pure logic (compiles on native host and Arduino) ────────────

static const char FALLBACK_TIMESTAMP[] = "0000-00-00T00:00:00";

// Minimum buffer size for ISO 8601 timestamp (19 chars + null)
static const uint8_t TIMESTAMP_BUF_MIN = 20;

uint8_t formatTimestamp(char* buf, uint8_t bufSize,
                        uint16_t year, uint8_t month, uint8_t day,
                        uint8_t hour, uint8_t minute, uint8_t second) {
    if (buf == nullptr || bufSize < TIMESTAMP_BUF_MIN) {
        return 0;
    }

    // Format: YYYY-MM-DDTHH:MM:SS
    buf[0]  = '0' + (char)((year / 1000) % 10);
    buf[1]  = '0' + (char)((year / 100)  % 10);
    buf[2]  = '0' + (char)((year / 10)   % 10);
    buf[3]  = '0' + (char)(year % 10);
    buf[4]  = '-';
    buf[5]  = '0' + (char)(month / 10);
    buf[6]  = '0' + (char)(month % 10);
    buf[7]  = '-';
    buf[8]  = '0' + (char)(day / 10);
    buf[9]  = '0' + (char)(day % 10);
    buf[10] = 'T';
    buf[11] = '0' + (char)(hour / 10);
    buf[12] = '0' + (char)(hour % 10);
    buf[13] = ':';
    buf[14] = '0' + (char)(minute / 10);
    buf[15] = '0' + (char)(minute % 10);
    buf[16] = ':';
    buf[17] = '0' + (char)(second / 10);
    buf[18] = '0' + (char)(second % 10);
    buf[19] = '\0';

    return 19;
}

const char* rtcFallbackTimestamp() {
    return FALLBACK_TIMESTAMP;
}

// ── Hardware-dependent (Arduino only) ───────────────────────────

#ifdef ARDUINO

#include <RTClib.h>

static RTC_DS3231 rtc;
static bool rtcAvailable = false;

bool rtcInit() {
    if (!rtc.begin()) {
        rtcAvailable = false;
        return false;
    }
    rtcAvailable = true;
    return true;
}

bool rtcGetTimestamp(char* buffer, uint8_t bufferSize) {
    if (!rtcAvailable || buffer == nullptr || bufferSize < TIMESTAMP_BUF_MIN) {
        return false;
    }

    DateTime now = rtc.now();
    uint8_t written = formatTimestamp(buffer, bufferSize,
                                      now.year(), now.month(), now.day(),
                                      now.hour(), now.minute(), now.second());
    return written > 0;
}

#endif // ARDUINO
