#pragma once
#include <stdint.h>

namespace vibration {
void begin();
// Call every sender loop with its latest real sensor estimate and timestamp.
// valid must be false for missing contact or an invalid sensor calculation.
void update(uint32_t now, float spo2, bool valid, uint32_t estimatedAt);
bool setThreshold(float percent); // Future algorithm integration; (0, 100].
float threshold();
bool isActive();
} // namespace vibration
