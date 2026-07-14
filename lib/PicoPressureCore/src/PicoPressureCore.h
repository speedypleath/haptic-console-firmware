#pragma once

#include <HapticProtocol.h>
#include <stdint.h>

namespace Haptic {
namespace PicoPressure {

static constexpr int16_t kZeroAdcCount = 166;
static constexpr int16_t kAdcPerKpa = 376;
static constexpr int16_t kOverrangeAdc = 200;

inline int16_t clampToInt16(int32_t value) {
  if (value > 32767) return 32767;
  if (value < -32768) return -32768;
  return static_cast<int16_t>(value);
}

inline int16_t pressureCentikpaFromRaw(int16_t raw,
                                       int16_t zeroAdcCount = kZeroAdcCount,
                                       int16_t adcPerKpa = kAdcPerKpa) {
  if (adcPerKpa <= 0) return 0;

  const int32_t delta = static_cast<int32_t>(raw) - zeroAdcCount;
  const int32_t pressure = delta * 100 / adcPerKpa;
  if (pressure < 0) return 0;
  return clampToInt16(pressure);
}

inline ModuleStatus statusForRaw(int16_t raw,
                                 int16_t zeroAdcCount = kZeroAdcCount,
                                 int16_t adcPerKpa = kAdcPerKpa,
                                 int16_t overrangeAdc = kOverrangeAdc) {
  const int32_t delta = static_cast<int32_t>(raw) - zeroAdcCount;
  const int32_t highLimit =
      static_cast<int32_t>(zeroAdcCount) +
      static_cast<int32_t>(adcPerKpa) * 10 +
      overrangeAdc;
  const bool overrange = raw > highLimit || delta < -overrangeAdc;
  return overrange ? MODULE_STATUS_SENSOR_FAULT : MODULE_STATUS_OK;
}

inline ModuleStatus buildPayload(int16_t raw, int16_t values[kMaxPayloadWords],
                                 int16_t zeroAdcCount = kZeroAdcCount,
                                 int16_t adcPerKpa = kAdcPerKpa,
                                 int16_t overrangeAdc = kOverrangeAdc) {
  for (uint8_t i = 0; i < kMaxPayloadWords; ++i) {
    values[i] = 0;
  }
  values[0] = pressureCentikpaFromRaw(raw, zeroAdcCount, adcPerKpa);
  values[1] = raw;
  values[2] = zeroAdcCount;
  return statusForRaw(raw, zeroAdcCount, adcPerKpa, overrangeAdc);
}

}  // namespace PicoPressure
}  // namespace Haptic
