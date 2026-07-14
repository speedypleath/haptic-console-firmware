#pragma once

#include <HapticProtocol.h>
#include <stdint.h>

namespace Haptic {
namespace PicoEncoder {

static constexpr int32_t kPulsesPerRev = 600;
static constexpr int32_t kCountsPerRev = kPulsesPerRev * 4;
static constexpr uint32_t kVelocityWindowMs = 50;

inline int8_t quadratureDelta(uint8_t lastState, uint8_t newState) {
  static const int8_t kQuadTable[16] = {
      0,  1, -1,  0,
     -1,  0,  0,  1,
      1,  0,  0, -1,
      0, -1,  1,  0,
  };
  return kQuadTable[((lastState & 0x03) << 2) | (newState & 0x03)];
}

inline int16_t clampToInt16(int32_t value) {
  if (value > 32767) return 32767;
  if (value < -32768) return -32768;
  return static_cast<int16_t>(value);
}

inline int16_t velocityTenthRpmFromDelta(int32_t positionDelta,
                                         uint32_t elapsedMs,
                                         int32_t countsPerRev = kCountsPerRev) {
  if (elapsedMs == 0 || countsPerRev <= 0) return 0;
  const int32_t countsPerSec =
      (positionDelta * 1000L) / static_cast<int32_t>(elapsedMs);
  return clampToInt16((countsPerSec * 600L) / countsPerRev);
}

inline int16_t directionFromDelta(int32_t positionDelta) {
  if (positionDelta > 0) return 1;
  if (positionDelta < 0) return -1;
  return 0;
}

inline bool shouldUpdateVelocity(uint32_t elapsedMs) {
  return elapsedMs >= kVelocityWindowMs;
}

inline void buildPayload(int32_t position, int16_t velocityTenthRpm,
                         int16_t direction,
                         int16_t values[kMaxPayloadWords]) {
  for (uint8_t i = 0; i < kMaxPayloadWords; ++i) {
    values[i] = 0;
  }
  values[0] = static_cast<int16_t>(position & 0xFFFF);
  values[1] = static_cast<int16_t>((position >> 16) & 0xFFFF);
  values[2] = velocityTenthRpm;
  values[3] = direction;
}

inline int32_t positionFromPayload(const int16_t values[kMaxPayloadWords]) {
  const uint32_t low = static_cast<uint16_t>(values[0]);
  const uint32_t high = static_cast<uint16_t>(values[1]);
  return static_cast<int32_t>((high << 16) | low);
}

}  // namespace PicoEncoder
}  // namespace Haptic
