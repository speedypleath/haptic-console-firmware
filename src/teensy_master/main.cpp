#include <Arduino.h>
#include <Wire.h>
#include <HapticProtocol.h>

using namespace Haptic;

static constexpr uint8_t  kModuleAddresses[] = {0x20, 0x21, 0x22};
static constexpr uint8_t  kNumModules        = sizeof(kModuleAddresses);
static constexpr uint32_t kPollIntervalMs    = 10;
static constexpr uint32_t kModuleTimeoutMs   = 200;
static constexpr uint8_t  kMissLimit         = 5;

struct ModuleState {
  bool     active       = false;
  uint32_t lastSuccessMs = 0;
  uint8_t  missCount    = 0;
};

static ModuleState states[kNumModules];
static uint32_t    lastPollMs  = 0;
static bool        debugOutput = true;

bool readModule(uint8_t address, ModulePacket &packet) {
  const uint8_t expected = sizeof(ModulePacket);
  const uint8_t received = Wire.requestFrom(address, expected);
  if (received != expected) {
    while (Wire.available()) Wire.read();
    return false;
  }
  uint8_t *dst = reinterpret_cast<uint8_t *>(&packet);
  for (uint8_t i = 0; i < expected; ++i) dst[i] = Wire.read();
  return packet.protocolVersion == kProtocolVersion && validatePacket(packet);
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
  Serial.print(" values=");
  for (uint8_t i = 0; i < kMaxPayloadWords; ++i) {
    Serial.print(packet.payload[i]);
    if (i + 1 < kMaxPayloadWords) Serial.print(",");
  }
  Serial.println();
}

void scanModules() {
  Serial.println("Scanning for modules...");
  for (uint8_t i = 0; i < kNumModules; ++i) {
    Wire.beginTransmission(kModuleAddresses[i]);
    const bool present = (Wire.endTransmission() == 0);
    states[i].active = present;
    states[i].missCount = 0;
    states[i].lastSuccessMs = millis();
    Serial.print("  0x");
    Serial.print(kModuleAddresses[i], HEX);
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
  if (now - lastPollMs < kPollIntervalMs) return;
  lastPollMs = now;

  for (uint8_t i = 0; i < kNumModules; ++i) {
    if (!states[i].active) continue;

    ModulePacket packet;
    if (readModule(kModuleAddresses[i], packet)) {
      states[i].missCount    = 0;
      states[i].lastSuccessMs = now;
      if (debugOutput) printPacket(kModuleAddresses[i], packet);
    } else {
      if (states[i].missCount < kMissLimit) {
        states[i].missCount++;
      }
      if (states[i].missCount >= kMissLimit &&
          now - states[i].lastSuccessMs >= kModuleTimeoutMs) {
        states[i].active = false;
        Serial.print("timeout: 0x");
        Serial.println(kModuleAddresses[i], HEX);
      }
    }
  }
}
