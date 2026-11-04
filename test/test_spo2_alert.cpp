#include "SpO2Alert.h"
#include <cassert>
#include <limits>

int main() {
  SpO2Alert alert;
  assert(!alert.update(0, false, 0));
  assert(!alert.update(10, true, 100));
  assert(!alert.update(20, true, 97)); // Strictly below the threshold.
  assert(alert.update(30, true, 96));
  assert(alert.update(5029, true, 96));
  assert(!alert.update(5030, true, 96)); // Exactly five seconds, no repeat.
  assert(!alert.update(20000, true, 95));
  assert(!alert.update(21000, false, 0));
  assert(!alert.update(22000, true, 96)); // Invalid input is not recovery.
  assert(!alert.update(23000, true, 97));
  assert(alert.update(24000, true, 96));
  assert(!alert.update(24001, false, 96)); // Stop early on contact/data loss.
  assert(!alert.update(25000, true, 96));

  assert(!alert.setThreshold(0));
  assert(!alert.setThreshold(101));
  assert(!alert.setThreshold(std::numeric_limits<float>::quiet_NaN()));
  assert(alert.threshold() == 97);
  assert(alert.setThreshold(95));
  assert(!alert.update(26000, true, 96));
  assert(alert.update(27000, true, 94));
  assert(!alert.update(27001, true, std::numeric_limits<float>::infinity()));
  assert(!alert.update(27002, true, 101));

  SpO2Alert recovery;
  assert(recovery.update(0, true, 96));
  assert(recovery.update(100, true, 99)); // Complete the timed pulse on recovery.
  assert(!recovery.update(5000, true, 99));
  assert(recovery.update(5001, true, 96));

  SpO2Alert rollover;
  const uint32_t start = UINT32_MAX - 1000;
  assert(rollover.update(start, true, 96));
  assert(rollover.update(uint32_t(start + 4999), true, 96));
  assert(!rollover.update(uint32_t(start + 5000), true, 96));
}
