#ifndef DEVICE_STATE_H
#define DEVICE_STATE_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <string>
typedef std::string String;
typedef unsigned long ulong;
#endif

// ─── State Machine States ───────────────────────────────────────────────────
enum DeviceState {
  STATE_INIT,
  STATE_READY,
  STATE_QR_SCANNING,
  STATE_QR_INVALID,
  STATE_QR_ACCEPTED,
  STATE_COUNTDOWN,
  STATE_CAPTURE,
  STATE_SAVING,
  STATE_SUCCESS,
  STATE_ERROR,
  STATE_SLEEP_PENDING
};

// ─── Device Context ─────────────────────────────────────────────────────────
struct DeviceContext {
  DeviceState currentState;
  String currentUserId;               // User_ID from last valid QR scan
  unsigned long stateEnteredAt;       // millis() when current state began
  unsigned long lastScanAt;           // millis() of last completed scan
  unsigned long countdownStartAt;     // millis() when countdown began
  bool sdReady;                       // SD card health status
  bool rtcReady;                      // RTC health status
};

#endif // DEVICE_STATE_H
