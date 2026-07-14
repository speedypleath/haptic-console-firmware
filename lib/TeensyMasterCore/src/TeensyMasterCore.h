#pragma once

#include <HapticProtocol.h>
#include <stddef.h>
#include <stdint.h>

namespace Haptic {
namespace TeensyMaster {

static constexpr uint8_t kModuleAddresses[] = {0x20, 0x21, 0x22};
static constexpr uint8_t kNumModules = sizeof(kModuleAddresses) / sizeof(kModuleAddresses[0]);
static constexpr uint32_t kPollIntervalMs = 10;
static constexpr uint32_t kModuleTimeoutMs = 200;
static constexpr uint8_t kMissLimit = 5;

struct ModuleState {
  bool active = false;
  uint32_t lastSuccessMs = 0;
  uint8_t missCount = 0;
};

struct LoadcellReading {
  int16_t cellA = 0;
  int16_t cellB = 0;
};

struct PressureReading {
  int16_t pressureCentikpa = 0;
  int16_t rawAdc = 0;
  int16_t zeroAdc = 0;
};

struct EncoderReading {
  int32_t position = 0;
  int16_t velocityTenthRpm = 0;
  int16_t direction = 0;
};

enum class PacketDecodeResult : uint8_t {
  Ok,
  WrongLength,
  WrongProtocol,
  BadChecksum,
};

inline bool isPollDue(uint32_t nowMs, uint32_t lastPollMs) {
  return nowMs - lastPollMs >= kPollIntervalMs;
}

inline PacketDecodeResult decodePacketBytes(const uint8_t *bytes, size_t length,
                                            ModulePacket &packet) {
  if (length != sizeof(ModulePacket)) {
    return PacketDecodeResult::WrongLength;
  }

  uint8_t *dst = reinterpret_cast<uint8_t *>(&packet);
  for (size_t i = 0; i < sizeof(ModulePacket); ++i) {
    dst[i] = bytes[i];
  }

  if (packet.protocolVersion != kProtocolVersion) {
    return PacketDecodeResult::WrongProtocol;
  }
  if (!validatePacket(packet)) {
    return PacketDecodeResult::BadChecksum;
  }
  return PacketDecodeResult::Ok;
}

inline void recordScanResult(ModuleState &state, bool present, uint32_t nowMs) {
  state.active = present;
  state.missCount = 0;
  state.lastSuccessMs = nowMs;
}

inline void recordReadSuccess(ModuleState &state, uint32_t nowMs) {
  state.active = true;
  state.missCount = 0;
  state.lastSuccessMs = nowMs;
}

inline bool recordReadFailure(ModuleState &state, uint32_t nowMs) {
  if (state.missCount < kMissLimit) {
    state.missCount++;
  }

  const bool timedOut =
      state.missCount >= kMissLimit &&
      nowMs - state.lastSuccessMs >= kModuleTimeoutMs;
  if (timedOut) {
    state.active = false;
  }
  return timedOut;
}

inline LoadcellReading parseLoadcellReading(const ModulePacket &packet) {
  LoadcellReading reading{};
  reading.cellA = packet.payload[0];
  reading.cellB = packet.payload[1];
  return reading;
}

inline PressureReading parsePressureReading(const ModulePacket &packet) {
  PressureReading reading{};
  reading.pressureCentikpa = packet.payload[0];
  reading.rawAdc = packet.payload[1];
  reading.zeroAdc = packet.payload[2];
  return reading;
}

inline EncoderReading parseEncoderReading(const ModulePacket &packet) {
  EncoderReading reading{};
  const uint32_t low = static_cast<uint16_t>(packet.payload[0]);
  const uint32_t high = static_cast<uint16_t>(packet.payload[1]);
  reading.position = static_cast<int32_t>((high << 16) | low);
  reading.velocityTenthRpm = packet.payload[2];
  reading.direction = packet.payload[3];
  return reading;
}

}  // namespace TeensyMaster
}  // namespace Haptic
