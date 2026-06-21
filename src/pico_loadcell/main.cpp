#include <Arduino.h>
#include <HX711_ADC.h>
#include <PicoModuleCommon.h>

using namespace Haptic;

// Pins — avoid GP4/5 (I2C), GP6 (IRQ), GP26 (ID/ADC).
static constexpr uint8_t kLcADout = 10;
static constexpr uint8_t kLcAClk  = 11;
static constexpr uint8_t kLcBDout = 12;
static constexpr uint8_t kLcBClk  = 13;

// CALIBRATE: run getNewCalibration(known_mass_grams) and replace this value.
static constexpr float kCalFactor = 1.0f;

HX711_ADC lcA(kLcADout, kLcAClk);
HX711_ADC lcB(kLcBDout, kLcBClk);
PicoModule module(HAPTIC_I2C_ADDRESS, HAPTIC_MODULE_KIND);

void setup() {
  Serial.begin(115200);

  lcA.begin();
  lcB.begin();
  lcA.setCalFactor(kCalFactor);
  lcB.setCalFactor(kCalFactor);

  // Run both HX711s through their stabilization + tare state machine in parallel.
  bool doneA = false, doneB = false;
  const uint32_t startMs = millis();
  while (!doneA || !doneB) {
    if (!doneA) doneA = lcA.startMultiple(2000, true);
    if (!doneB) doneB = lcB.startMultiple(2000, true);
    if (millis() - startMs > 6000) {
      break;  // proceed even if one sensor is absent; fault will be reported in loop
    }
  }

  module.begin();
}

void loop() {
  lcA.update();
  lcB.update();

  if (lcA.getSignalTimeoutFlag() || lcB.getSignalTimeoutFlag()) {
    int16_t vals[kMaxPayloadWords] = {};
    module.publish(MODULE_STATUS_SENSOR_FAULT, vals, kMaxPayloadWords);
    delay(5);
    return;
  }

  int16_t values[kMaxPayloadWords] = {};
  // getData() returns calFactor-scaled float. Clamp to int16 range.
  // With kCalFactor=1.0 the raw 24-bit counts will saturate until calibration is set.
  values[0] = (int16_t)constrain((int32_t)lcA.getData(), -32768, 32767);
  values[1] = (int16_t)constrain((int32_t)lcB.getData(), -32768, 32767);

  module.publish(MODULE_STATUS_OK, values, kMaxPayloadWords);
  delay(5);
}
