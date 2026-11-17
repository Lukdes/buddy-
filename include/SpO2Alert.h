#pragma once
#include <cmath>
#include <stdint.h>

// Independent of GPIO and sensor code so a future algorithm can set the threshold.
class SpO2Alert {
 public:
  bool setThreshold(float percent) {
    if (!std::isfinite(percent) || percent <= 0 || percent > 100) return false;
    threshold_ = percent;
    return true;
  }

  float threshold() const { return threshold_; }

  // Continuous output for valid below-threshold readings; no latch or timer.
  bool update(uint32_t, bool valid, float spo2) {
    return valid && std::isfinite(spo2) && spo2 > 0 && spo2 <= 100 &&
        spo2 < threshold_;
  }

 private:
  float threshold_ = 100.0f;
};
