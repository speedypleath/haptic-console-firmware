#include <Arduino.h>
#include <Wire.h>
#include <HapticProtocol.h>
#include <TeensyMasterCore.h>

using namespace Haptic;
namespace TM = Haptic::TeensyMaster;

static TM::ModuleState states[TM::kNumModules];
static uint32_t    lastPollMs  = 0;
static bool        debugOutput = true;
static TM::MidiControlChangeCache midiCache;

bool readModule(uint8_t address, ModulePacket &packet) {
  const uint8_t expected = sizeof(ModulePacket);
  const uint8_t received = Wire.requestFrom(address, expected);
  if (received != expected) {
    while (Wire.available()) Wire.read();
    return false;
  }
  uint8_t *dst = reinterpret_cast<uint8_t *>(&packet);
  for (uint8_t i = 0; i < expected; ++i) dst[i] = Wire.read();
  return TM::decodePacketBytes(reinterpret_cast<const uint8_t *>(&packet),
                               sizeof(packet), packet) ==
         TM::PacketDecodeResult::Ok;
}

void printPacket(uint8_t address, const ModulePacket &packet) {
  Serial.print("0x");
  Serial.print(address, HEX);
  Serial.print(" ");
  Serial.print(moduleKindName(packet.moduleKind));
  Serial.print(" seq=");
  Serial.print(packet.sequence);
  Serial.print(" idAdc=");
  Serial.print(packet.idAdc);
  if (packet.moduleKind == MODULE_KIND_LOADCELL) {
    const TM::LoadcellReading reading = TM::parseLoadcellReading(packet);
    Serial.print(" cellA=");
    Serial.print(reading.cellA);
    Serial.print(" cellB=");
    Serial.print(reading.cellB);
  } else if (packet.moduleKind == MODULE_KIND_PRESSURE) {
    const TM::PressureReading reading = TM::parsePressureReading(packet);
    Serial.print(" pressure_cKpa=");
    Serial.print(reading.pressureCentikpa);
    Serial.print(" raw=");
    Serial.print(reading.rawAdc);
    Serial.print(" zero=");
    Serial.print(reading.zeroAdc);
  } else if (packet.moduleKind == MODULE_KIND_ENCODER) {
    const TM::EncoderReading reading = TM::parseEncoderReading(packet);
    Serial.print(" position=");
    Serial.print(reading.position);
    Serial.print(" velocity_0.1rpm=");
    Serial.print(reading.velocityTenthRpm);
    Serial.print(" direction=");
    Serial.print(reading.direction);
  } else {
    Serial.print(" values=");
    for (uint8_t i = 0; i < kMaxPayloadWords; ++i) {
      Serial.print(packet.payload[i]);
      if (i + 1 < kMaxPayloadWords) Serial.print(",");
    }
  }
  Serial.println();
}

void sendMidiForPacket(const ModulePacket &packet) {
  TM::MidiControlChange changes[3] = {};
  const uint8_t count = TM::midiChangesForPacket(packet, changes, 3);
  for (uint8_t i = 0; i < count; ++i) {
    const uint8_t control = changes[i].control;
    const uint8_t value = changes[i].value;
    if (!TM::shouldSendMidiControlChange(midiCache, control, value)) continue;
#if defined(USB_MIDI) || defined(USB_MIDI_SERIAL)
    usbMIDI.sendControlChange(control, value, TM::kMidiDefaultChannel);
#endif
  }
#if defined(USB_MIDI) || defined(USB_MIDI_SERIAL)
  while (usbMIDI.read()) {
    // Drain inbound USB MIDI so host-side buffers do not accumulate.
  }
#endif
}

void scanModules() {
  Serial.println("Scanning for modules...");
  for (uint8_t i = 0; i < TM::kNumModules; ++i) {
    Wire.beginTransmission(TM::kModuleAddresses[i]);
    const bool present = (Wire.endTransmission() == 0);
    TM::recordScanResult(states[i], present, millis());
    Serial.print("  0x");
    Serial.print(TM::kModuleAddresses[i], HEX);
    Serial.println(present ? " found" : " not found");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);
  delay(500);
  Serial.println("Haptic Console Teensy master");
  Serial.println("Serial commands: 'd' = toggle debug output");
  scanModules();
}

void loop() {
  // Toggle debug output on 'd' keypress.
  while (Serial.available()) {
    if (Serial.read() == 'd') {
      debugOutput = !debugOutput;
      Serial.println(debugOutput ? "debug on" : "debug off");
    }
  }

  const uint32_t now = millis();
  if (!TM::isPollDue(now, lastPollMs)) return;
  lastPollMs = now;

  for (uint8_t i = 0; i < TM::kNumModules; ++i) {
    if (!states[i].active) continue;

    ModulePacket packet;
    if (readModule(TM::kModuleAddresses[i], packet)) {
      TM::recordReadSuccess(states[i], now);
      sendMidiForPacket(packet);
      if (debugOutput) printPacket(TM::kModuleAddresses[i], packet);
    } else {
      if (TM::recordReadFailure(states[i], now)) {
        Serial.print("timeout: 0x");
        Serial.println(TM::kModuleAddresses[i], HEX);
      }
    }
  }
}
