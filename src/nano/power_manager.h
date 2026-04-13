#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#endif

// MOSFET gate pin (active LOW to power on ESP32-CAM)
static const uint8_t PIN_MOSFET_GATE = 8;

// Wake button pin (INT0 on D2)
static const uint8_t PIN_WAKE_BUTTON = 2;

// Initialize MOSFET pin (HIGH = ESP off), button pin with pull-up.
void powerInit();

// Power on ESP32-CAM: drive MOSFET gate LOW.
void espPowerOn();

// Power off ESP32-CAM: drive MOSFET gate HIGH.
void espPowerOff();

// Enter Nano power-down sleep mode. Wakes on INT0 (button press).
// Before calling: ensure LED is off, MOSFET gate HIGH.
void enterDeepSleep();

// Attach wake interrupt on button pin (falling edge).
void attachWakeInterrupt();

#endif
