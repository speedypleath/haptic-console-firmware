#pragma once

#include <HapticProtocol.h>
#include <stdint.h>

namespace Haptic {
namespace PicoLoadcell {

inline int16_t clampToInt16(int32_t value) {
  if (value > 32767) return 32767;
  if (value < -32768) return -32768;
  return static_cast<int16_t>(value);
}

inline ModuleStatus statusForTimeoutFlags(bool timeoutA, bool timeoutB) {
  return (timeoutA || timeoutB) ? MODULE_STATUS_SENSOR_FAULT : MODULE_STATUS_OK;
}

inline void buildPayload(int32_t loadCellA, int32_t loadCellB,
                         int16_t values[kMaxPayloadWords]) {
  for (uint8_t i = 0; i < kMaxPayloadWords; ++i) {
    values[i] = 0;
  }
  values[0] = clampToInt16(loadCellA);
  values[1] = clampToInt16(loadCellB);
}

}  // namespace PicoLoadcell
}  // namespace Haptic
