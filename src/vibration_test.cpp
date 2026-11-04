#include <Arduino.h>
#include "SpO2Alert.h"

// Standalone bench test: no sensor readings, no radio, one pulse per boot.
constexpr uint8_t vibrationPin = 27;
SpO2Alert testAlert;
uint32_t testStarted;
bool wasOn = false;
bool finished = false;

void setup() {
  digitalWrite(vibrationPin, LOW);
  pinMode(vibrationPin, OUTPUT);
  Serial.begin(115200);
  testStarted = millis();
}

void loop() {
  if (finished) return;
  uint32_t elapsed = millis() - testStarted;
  if (elapsed < 10000) return; // Allow the operator to attach a monitor.
  bool on = testAlert.update(millis(), true, 96.0f);
  if (on != wasOn) {
    digitalWrite(vibrationPin, on ? HIGH : LOW);
    Serial.println(on
        ? "SYNTHETIC MOTOR TEST: 96% input (not measured), GPIO27 ON for 5 seconds."
        : "SYNTHETIC MOTOR TEST: GPIO27 OFF. Complete; no repeat until reset.");
    wasOn = on;
    if (!on) finished = true;
  }
  delay(1);
}
