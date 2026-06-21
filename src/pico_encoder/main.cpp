#include <Arduino.h>
#include <PicoModuleCommon.h>

using namespace Haptic;

constexpr uint8_t kEncoderAPin = 14;
constexpr uint8_t kEncoderBPin = 15;

PicoModule module(HAPTIC_I2C_ADDRESS, HAPTIC_MODULE_KIND);

int32_t encoderPosition = 0;
uint8_t lastState = 0;

void updateEncoder() {
  const uint8_t state = (digitalRead(kEncoderAPin) << 1) | digitalRead(kEncoderBPin);
  const int8_t transition = (lastState << 2) | state;
  if (transition == 0b0001 || transition == 0b0111 ||
      transition == 0b1110 || transition == 0b1000) {
    encoderPosition++;
  } else if (transition == 0b0010 || transition == 0b1011 ||
             transition == 0b1101 || transition == 0b0100) {
    encoderPosition--;
  }
  lastState = state;
}

void setup() {
  Serial.begin(115200);
  pinMode(kEncoderAPin, INPUT_PULLUP);
  pinMode(kEncoderBPin, INPUT_PULLUP);
  lastState = (digitalRead(kEncoderAPin) << 1) | digitalRead(kEncoderBPin);
  module.begin();
}

void loop() {
  updateEncoder();

  int16_t values[kMaxPayloadWords] = {};
  values[0] = static_cast<int16_t>(encoderPosition & 0xFFFF);
  values[1] = static_cast<int16_t>((encoderPosition >> 16) & 0xFFFF);

  module.publish(MODULE_STATUS_OK, values, kMaxPayloadWords);
  delay(1);
}

