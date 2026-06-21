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

  // TODO: Replace with HX711 readings from the two load cells.
  values[0] = analogRead(A0);
  values[1] = analogRead(A1);

  module.publish(MODULE_STATUS_OK, values, kMaxPayloadWords);
  delay(5);
}

