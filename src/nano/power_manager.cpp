#ifdef ARDUINO

#include "power_manager.h"
#include <avr/sleep.h>
#include <avr/interrupt.h>

// Empty ISR for INT0 wake — execution resumes after sleep_cpu().
static void wakeISR() {
    // intentionally empty
}

void powerInit() {
    // MOSFET gate HIGH = ESP32-CAM power OFF (P-channel, active LOW)
    pinMode(PIN_MOSFET_GATE, OUTPUT);
    digitalWrite(PIN_MOSFET_GATE, HIGH);

    // Wake button with internal pull-up (external 10kΩ also present)
    pinMode(PIN_WAKE_BUTTON, INPUT_PULLUP);
}

void espPowerOn() {
    digitalWrite(PIN_MOSFET_GATE, LOW);
}

void espPowerOff() {
    digitalWrite(PIN_MOSFET_GATE, HIGH);
}

void attachWakeInterrupt() {
    attachInterrupt(digitalPinToInterrupt(PIN_WAKE_BUTTON), wakeISR, FALLING);
}

void enterDeepSleep() {
    // Attach INT0 falling-edge interrupt to wake from sleep
    attachWakeInterrupt();

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    // Ensure interrupts are enabled before sleeping
    sei();
    sleep_cpu();

    // Execution resumes here after INT0 fires
    sleep_disable();

    // Detach interrupt to avoid spurious triggers during normal operation
    detachInterrupt(digitalPinToInterrupt(PIN_WAKE_BUTTON));
}

#endif // ARDUINO
