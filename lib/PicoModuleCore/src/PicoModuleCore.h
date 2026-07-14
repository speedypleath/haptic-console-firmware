#pragma once

#include <HapticProtocol.h>
#include <stdint.h>

namespace Haptic {
namespace PicoCore {

inline void initializePacket(ModulePacket &packet, uint8_t moduleKind,
                             uint16_t idAdc) {
  packet.protocolVersion = kProtocolVersion;
  packet.moduleKind = moduleKind;
  packet.status = MODULE_STATUS_BOOTING;
  packet.sequence = 0;
  packet.idAdc = idAdc;
  for (uint8_t i = 0; i < kMaxPayloadWords; ++i) {
    packet.payload[i] = 0;
  }
  finalizePacket(packet);
}

inline void publishPacket(ModulePacket &packet, ModuleStatus status,
                          uint16_t idAdc, const int16_t *values,
                          uint8_t count) {
  packet.status = status;
  packet.sequence++;
  packet.idAdc = idAdc;
  for (uint8_t i = 0; i < kMaxPayloadWords; ++i) {
    packet.payload[i] = i < count ? values[i] : 0;
  }
  finalizePacket(packet);
}

}  // namespace PicoCore
}  // namespace Haptic
