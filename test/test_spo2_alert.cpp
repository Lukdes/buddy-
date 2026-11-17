#include "SpO2Alert.h"
#include <cassert>
#include <limits>

int main() {
  SpO2Alert alert;
  assert(alert.threshold() == 100);
  assert(!alert.update(0, false, 99));
  assert(!alert.update(1, true, 100));
  assert(alert.update(1, true, 99.9f)); // Whole-number display would round to 100.
  assert(!alert.update(1, true, 100));
  assert(alert.update(2, true, 99));
  assert(alert.update(5002, true, 99));
  assert(alert.update(60002, true, 98)); // No five-second timeout.
  assert(!alert.update(60003, false, 98));
  assert(alert.update(60004, true, 94)); // Contact return needs no re-arming.
  assert(!alert.update(60005, true, 100));
  assert(alert.update(60006, true, 99));
  assert(!alert.update(60007, true, 0));
  assert(!alert.update(60008, true, 101));
  assert(!alert.update(60009, true, std::numeric_limits<float>::quiet_NaN()));
  assert(!alert.update(60010, true, std::numeric_limits<float>::infinity()));
  assert(!alert.setThreshold(0));
  assert(!alert.setThreshold(101));
  assert(!alert.setThreshold(std::numeric_limits<float>::quiet_NaN()));
  assert(alert.threshold() == 100);
  assert(alert.setThreshold(97));
  assert(!alert.update(60011, true, 97));
  assert(alert.update(60012, true, 96));
  assert(alert.update(UINT32_MAX, true, 96));
  assert(alert.update(0, true, 96));
}
