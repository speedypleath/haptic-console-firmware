#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#endif
#include <stddef.h>
#include <stdint.h>

namespace Haptic {

constexpr uint8_t kProtocolVersion = 1;
constexpr uint8_t kMaxPayloadWords = 6;
constexpr uint8_t kDefaultIrqPin = 6;
constexpr uint8_t kDefaultIdPin = 26;

enum ModuleKind : uint8_t {
  MODULE_KIND_UNKNOWN = 0,
  MODULE_KIND_LOADCELL = 1,
  MODULE_KIND_PRESSURE = 2,
  MODULE_KIND_ENCODER = 3,
};

enum ModuleStatus : uint8_t {
  MODULE_STATUS_OK = 0,
  MODULE_STATUS_BOOTING = 1,
  MODULE_STATUS_SENSOR_FAULT = 2,
  MODULE_STATUS_CALIBRATING = 3,
};

struct ModulePacket {
  uint8_t protocolVersion;
  uint8_t moduleKind;
  uint8_t status;
  uint8_t sequence;
  uint16_t idAdc;
  int16_t payload[kMaxPayloadWords];
  uint8_t checksum;
} __attribute__((packed));

static_assert(sizeof(ModulePacket) == 19, "Unexpected ModulePacket size");

inline uint8_t checksumBytes(const uint8_t *bytes, size_t length) {
  uint8_t sum = 0;
  for (size_t i = 0; i < length; ++i) {
    sum ^= bytes[i];
  }
  return sum;
}

inline void finalizePacket(ModulePacket &packet) {
  packet.checksum = 0;
  packet.checksum = checksumBytes(reinterpret_cast<const uint8_t *>(&packet),
                                 sizeof(ModulePacket));
}

inline bool validatePacket(const ModulePacket &packet) {
  ModulePacket copy = packet;
  const uint8_t expected = copy.checksum;
  copy.checksum = 0;
  return expected == checksumBytes(reinterpret_cast<const uint8_t *>(&copy),
                                   sizeof(ModulePacket));
}

inline const char *moduleKindName(uint8_t kind) {
  switch (kind) {
    case MODULE_KIND_LOADCELL:
      return "loadcell";
    case MODULE_KIND_PRESSURE:
      return "pressure";
    case MODULE_KIND_ENCODER:
      return "encoder";
    default:
      return "unknown";
  }
}

}  // namespace Haptic

