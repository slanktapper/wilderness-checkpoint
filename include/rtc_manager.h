#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <string>
// Provide String type alias for native builds
using String = std::string;
#endif

namespace RTC {

/// Initialize I2C on custom pins (GPIO 14 SCL / GPIO 15 SDA) and check DS3231 presence.
/// @return true if DS3231 responds on I2C
bool init();

/// Return current time as ISO 8601 string ("YYYY-MM-DDTHH:MM:SS").
/// Returns fallback "0000-00-00T00:00:00" if RTC is unresponsive.
String getTimestamp();

/// @return true if DS3231 RTC responds on I2C
bool isAvailable();

} // namespace RTC

#endif // RTC_MANAGER_H
