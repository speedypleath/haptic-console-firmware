#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <HapticProtocol.h>

#if !defined(HAPTIC_I2C_ADDRESS) || !defined(HAPTIC_MODULE_KIND)
#if defined(__INTELLISENSE__) || defined(__clangd__)
#ifndef HAPTIC_I2C_ADDRESS
#define HAPTIC_I2C_ADDRESS 0x7F
#endif
#ifndef HAPTIC_MODULE_KIND
#define HAPTIC_MODULE_KIND MODULE_KIND_UNKNOWN
#endif
#else
#error "Pico module firmware must be built with HAPTIC_I2C_ADDRESS and HAPTIC_MODULE_KIND from platformio.ini"
#endif
#endif

namespace Haptic {

class PicoModule {
 public:
  PicoModule(uint8_t address, uint8_t kind, uint8_t irqPin = kDefaultIrqPin,
             uint8_t idPin = kDefaultIdPin)
      : address_(address), kind_(kind), irqPin_(irqPin), idPin_(idPin) {}

  void begin() {
#ifndef HAPTIC_DEBUG
    pinMode(irqPin_, OUTPUT);
    digitalWrite(irqPin_, LOW);
#endif

    packet_.protocolVersion = kProtocolVersion;
    packet_.moduleKind = kind_;
    packet_.status = MODULE_STATUS_BOOTING;
    packet_.sequence = 0;
    packet_.idAdc = readIdAdc();
    for (uint8_t i = 0; i < kMaxPayloadWords; ++i) {
      packet_.payload[i] = 0;
    }
    finalizePacket(packet_);

#ifdef HAPTIC_DEBUG
    for (uint32_t start = millis(); !Serial && millis() - start < 2000; ) { delay(10); }
    Serial.print("HAPTIC DEBUG addr=0x");
    Serial.print(address_, HEX);
    Serial.print(" kind=");
    Serial.println(moduleKindName(kind_));
#else
    instance() = this;
    Wire.begin(address_);
    Wire.onRequest(handleRequestThunk);
#endif
  }

  void publish(ModuleStatus status, const int16_t *values, uint8_t count) {
#ifndef HAPTIC_DEBUG
    noInterrupts();
#endif
    packet_.status = status;
    packet_.sequence++;
    packet_.idAdc = readIdAdc();
    for (uint8_t i = 0; i < kMaxPayloadWords; ++i) {
      packet_.payload[i] = i < count ? values[i] : 0;
    }
    finalizePacket(packet_);
#ifdef HAPTIC_DEBUG
    printDebug();
    delay(100);
#else
    interrupts();
    digitalWrite(irqPin_, HIGH);
#endif
  }

  uint16_t readIdAdc() const {
    return static_cast<uint16_t>(analogRead(idPin_));
  }

 private:
#ifdef HAPTIC_DEBUG
  void printDebug() {
    Serial.print("kind=");
    Serial.print(moduleKindName(packet_.moduleKind));
    Serial.print(" seq=");
    Serial.print(packet_.sequence);
    Serial.print(" status=");
    Serial.print(packet_.status);
    Serial.print(" idAdc=");
    Serial.print(packet_.idAdc);
    for (uint8_t i = 0; i < kMaxPayloadWords; ++i) {
      Serial.print(" p[");
      Serial.print(i);
      Serial.print("]=");
      Serial.print(packet_.payload[i]);
    }
    Serial.print(" csum=0x");
    Serial.println(packet_.checksum, HEX);
  }
#endif

  static void handleRequestThunk() {
    if (instance() != nullptr) {
      instance()->handleRequest();
    }
  }

  void handleRequest() {
    noInterrupts();
    ModulePacket snapshot = packet_;
    interrupts();

    Wire.write(reinterpret_cast<const uint8_t *>(&snapshot), sizeof(snapshot));
    digitalWrite(irqPin_, LOW);
  }

  uint8_t address_;
  uint8_t kind_;
  uint8_t irqPin_;
  uint8_t idPin_;
  ModulePacket packet_{};

  static PicoModule *&instance() {
    static PicoModule *module = nullptr;
    return module;
  }
};

}  // namespace Haptic
