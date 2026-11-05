#include <Arduino.h>
#include "Vibration.h"
#include "SpO2Alert.h"

namespace {
constexpr uint8_t vibrationPin = 27;
constexpr bool vibrationActiveHigh = true;
constexpr float initialSpO2Threshold = 97.0f;
constexpr uint32_t packetFreshMs = 2000;
SpO2Alert oxygenAlert;
bool motorOn = false;

void driveMotor(bool on) {
  digitalWrite(vibrationPin, (on == vibrationActiveHigh) ? HIGH : LOW);
  motorOn = on;
}
} // namespace

namespace vibration {
void begin() {
  driveMotor(false);
  pinMode(vibrationPin, OUTPUT);
  oxygenAlert = SpO2Alert();
  oxygenAlert.setThreshold(initialSpO2Threshold);
}

void update(uint32_t now, float spo2, bool valid, uint32_t receivedAt) {
  bool fresh = valid && uint32_t(now - receivedAt) <= packetFreshMs;
  bool on = oxygenAlert.update(now, fresh, spo2);
  if (on != motorOn) {
    driveMotor(on);
    Serial.printf("Vibration %s | SpO2 threshold <%.1f%%\n",
        on ? "ON (maximum 5 seconds)" : "OFF", oxygenAlert.threshold());
  }
}

bool setThreshold(float percent) { return oxygenAlert.setThreshold(percent); }
float threshold() { return oxygenAlert.threshold(); }
bool isActive() { return motorOn; }
} // namespace vibration
