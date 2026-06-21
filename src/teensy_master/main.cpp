#include <Arduino.h>
#include <Wire.h>
#include <HapticProtocol.h>

using namespace Haptic;

constexpr uint8_t kModuleAddresses[] = {0x20, 0x21, 0x22};
constexpr uint32_t kPollIntervalMs = 10;

uint32_t lastPollMs = 0;

bool readModule(uint8_t address, ModulePacket &packet) {
  const uint8_t expected = sizeof(ModulePacket);
  const uint8_t received = Wire.requestFrom(address, expected);
  if (received != expected) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }

  uint8_t *dst = reinterpret_cast<uint8_t *>(&packet);
  for (uint8_t i = 0; i < expected; ++i) {
    dst[i] = Wire.read();
  }

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
    if (i + 1 < kMaxPayloadWords) {
      Serial.print(",");
    }
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);
  delay(500);
  Serial.println("Haptic Console Teensy master");
}

void loop() {
  const uint32_t now = millis();
  if (now - lastPollMs < kPollIntervalMs) {
    return;
  }
  lastPollMs = now;

  for (uint8_t address : kModuleAddresses) {
    ModulePacket packet;
    if (readModule(address, packet)) {
      printPacket(address, packet);
    }
  }
}

