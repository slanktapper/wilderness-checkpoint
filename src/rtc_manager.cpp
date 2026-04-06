#include "rtc_manager.h"
#include "config.h"

#ifdef ARDUINO

#include <Wire.h>
#include <RTClib.h>

// ─── ESP32 / DS3231 Implementation ─────────────────────────────────────────

namespace {

static RTC_DS3231 ds3231;
static bool       rtcFound = false;

static const char* FALLBACK_TIMESTAMP = "0000-00-00T00:00:00";

} // anonymous namespace

namespace RTC {

bool init() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  if (ds3231.begin(&Wire)) {
    rtcFound = true;
  } else {
    rtcFound = false;
  }
  return rtcFound;
}

String getTimestamp() {
  if (!rtcFound) {
    return String(FALLBACK_TIMESTAMP);
  }

  DateTime now = ds3231.now();

  // Basic validity check — year 0 or extremely far future likely means bad data
  if (now.year() < 2000 || now.year() > 2099) {
    return String(FALLBACK_TIMESTAMP);
  }

  char buf[20]; // "YYYY-MM-DDTHH:MM:SS" = 19 chars + null
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  return String(buf);
}

bool isAvailable() {
  return rtcFound;
}

} // namespace RTC

#else

// ─── Native (stub) Implementation ──────────────────────────────────────────

namespace RTC {

bool init() {
  return false;
}

String getTimestamp() {
  return "0000-00-00T00:00:00";
}

bool isAvailable() {
  return false;
}

} // namespace RTC

#endif
