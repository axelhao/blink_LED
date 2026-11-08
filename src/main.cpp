#include <Arduino.h>

// Blue Pill onboard LED: PC13, active LOW.
// The STM32 Arduino core defines LED_BUILTIN in the board variant; keep this
// fallback so the sketch also builds for variants that do not.
#ifndef LED_BUILTIN
#define LED_BUILTIN PC13
#endif

namespace {

constexpr pin_size_t kLedPin = LED_BUILTIN;

// The onboard LED is connected between 3.3 V and PC13, so a LOW pin turns it on.
constexpr bool kLedActiveLow = true;
constexpr PinStatus kLedOnLevel = kLedActiveLow ? LOW : HIGH;
constexpr PinStatus kLedOffLevel = kLedActiveLow ? HIGH : LOW;

constexpr uint32_t kBlinkIntervalMs = 1000;

void setLed(bool on) {
  digitalWrite(kLedPin, on ? kLedOnLevel : kLedOffLevel);
}

}  // namespace

void setup() {
  pinMode(kLedPin, OUTPUT);
  setLed(false);  // LED off
}

void loop() {
  static bool ledOn = false;
  static uint32_t lastToggleMs = 0;

  const uint32_t now = millis();

  // Unsigned subtraction also handles millis() rollover correctly.
  if (now - lastToggleMs >= kBlinkIntervalMs) {
    lastToggleMs = now;  // do not try to catch up after a long stall
    ledOn = !ledOn;
    setLed(ledOn);
  }

  // loop() no longer blocks, so other work can run here.
}


