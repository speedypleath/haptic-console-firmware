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

enum MidiControl : uint8_t {
  MIDI_CC_FLYWHEEL_VELOCITY = 1,
  MIDI_CC_FLYWHEEL_DIRECTION = 2,
  MIDI_CC_PNEUMATIC_PRESSURE = 3,
  MIDI_CC_SPRING_TENSION = 4,
  MIDI_CC_SPRING_ACOUSTIC = 5,
  MIDI_CC_LEAN_TOTAL = 6,
  MIDI_CC_LEAN_BALANCE = 7,
  MIDI_CC_MATRIX_CENTROID_X = 8,
  MIDI_CC_MATRIX_CENTROID_Y = 9,
  MIDI_CC_MATRIX_PRESSURE = 10,
  MIDI_CC_JOYSTICK_1_X = 11,
  MIDI_CC_JOYSTICK_1_Y = 12,
  MIDI_CC_JOYSTICK_2_X = 13,
  MIDI_CC_JOYSTICK_2_Y = 14,
};

static constexpr uint8_t kMidiDefaultChannel = 1;
static constexpr int16_t kPressureFullScaleCentikpa = 1000;       // 10.00 kPa
static constexpr int16_t kFlywheelFullScaleTenthRpm = 600;        // 60.0 RPM

struct MidiControlChange {
  uint8_t control = 0;
  uint8_t value = 0;
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

inline uint8_t clampMidiValue(int32_t value) {
  if (value < 0) return 0;
  if (value > 127) return 127;
  return static_cast<uint8_t>(value);
}

inline uint8_t mapUnipolarToMidi(int32_t value, int32_t fullScale) {
  if (fullScale <= 0) return 0;
  if (value <= 0) return 0;
  if (value >= fullScale) return 127;
  return clampMidiValue((value * 127 + fullScale / 2) / fullScale);
}

inline uint8_t mapSignedInt16ToMidi(int16_t value) {
  const int32_t shifted = static_cast<int32_t>(value) + 32768;
  return clampMidiValue((shifted * 127 + 32767) / 65535);
}

inline uint8_t mapDirectionToMidi(int16_t direction) {
  if (direction < 0) return 0;
  if (direction > 0) return 127;
  return 64;
}

inline uint8_t mapVelocityToMidi(int16_t velocityTenthRpm) {
  const int32_t magnitude =
      velocityTenthRpm < 0 ? -static_cast<int32_t>(velocityTenthRpm)
                           : static_cast<int32_t>(velocityTenthRpm);
  return mapUnipolarToMidi(magnitude, kFlywheelFullScaleTenthRpm);
}

inline uint8_t mapPressureToMidi(int16_t pressureCentikpa) {
  return mapUnipolarToMidi(pressureCentikpa, kPressureFullScaleCentikpa);
}

inline uint8_t loadcellValueToMidi(int16_t value) {
  return mapSignedInt16ToMidi(value);
}

inline uint8_t midiChangesForPacket(const ModulePacket &packet,
                                    MidiControlChange *changes,
                                    uint8_t capacity) {
  if (changes == nullptr || capacity == 0 || packet.status != MODULE_STATUS_OK) {
    return 0;
  }

  uint8_t count = 0;
  const auto append = [&](uint8_t control, uint8_t value) {
    if (count < capacity) {
      changes[count].control = control;
      changes[count].value = value;
      count++;
    }
  };

  if (packet.moduleKind == MODULE_KIND_LOADCELL) {
    const LoadcellReading reading = parseLoadcellReading(packet);
    append(MIDI_CC_SPRING_TENSION, loadcellValueToMidi(reading.cellA));
    append(MIDI_CC_SPRING_ACOUSTIC, loadcellValueToMidi(reading.cellB));
  } else if (packet.moduleKind == MODULE_KIND_PRESSURE) {
    const PressureReading reading = parsePressureReading(packet);
    append(MIDI_CC_PNEUMATIC_PRESSURE,
           mapPressureToMidi(reading.pressureCentikpa));
  } else if (packet.moduleKind == MODULE_KIND_ENCODER) {
    const EncoderReading reading = parseEncoderReading(packet);
    append(MIDI_CC_FLYWHEEL_VELOCITY,
           mapVelocityToMidi(reading.velocityTenthRpm));
    append(MIDI_CC_FLYWHEEL_DIRECTION,
           mapDirectionToMidi(reading.direction));
  }

  return count;
}

}  // namespace TeensyMaster
}  // namespace Haptic
