#include <Arduino.h>
#include <PicoModuleCommon.h>

using namespace Haptic;

PicoModule module(HAPTIC_I2C_ADDRESS, HAPTIC_MODULE_KIND);

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  module.begin();
}

void loop() {
  int16_t values[kMaxPayloadWords] = {};

  // TODO: Read MPXV7002DP through a divider/conditioning stage scaled for 3.3V ADC.
  values[0] = analogRead(A0);

  module.publish(MODULE_STATUS_OK, values, kMaxPayloadWords);
  delay(5);
}

