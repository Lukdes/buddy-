#pragma once
#include <cmath>
#include <stdint.h>

// Independent of GPIO and sensor code so a future algorithm can set the threshold.
class SpO2Alert {
 public:
  static constexpr uint32_t pulseMs = 5000;

  bool setThreshold(float percent) {
    if (!std::isfinite(percent) || percent <= 0 || percent > 100) return false;
    threshold_ = percent;
    return true;
  }

  float threshold() const { return threshold_; }

  // Call regularly with the latest fresh estimate. Invalid input stops the motor
  // but does not re-arm a consumed alert; only valid recovery can do that.
  bool update(uint32_t now, bool valid, float spo2) {
    valid = valid && std::isfinite(spo2) && spo2 > 0 && spo2 <= 100;
    if (!valid) {
      active_ = false;
      return false;
    }
    if (active_ && uint32_t(now - started_) >= pulseMs) active_ = false;
    if (active_) return true;

    if (spo2 >= threshold_) {
      armed_ = true;
    } else if (armed_) {
      armed_ = false;
      active_ = true;
      started_ = now;
    }
    return active_;
  }

 private:
  float threshold_ = 97.0f;
  uint32_t started_ = 0;
  bool armed_ = true;
  bool active_ = false;
};
