#include <Arduino.h>
#include <PicoModuleCommon.h>
#include <PicoPressureCore.h>

using namespace Haptic;

// MPX5010DP: gauge pressure sensor, 0–10 kPa range, 5 V supply.
// Transfer function: Vout = Vs × (0.09 × P + 0.04), P in kPa, Vs = 5 V.
//   0 kPa  → 0.20 V
//   10 kPa → 4.70 V
// A resistive voltage divider is required to keep Vout within the Pico's
// 3.3 V ADC range (e.g. 33 kΩ / 68 kΩ → ×0.67 ratio).
//
// CALIBRATE: measure PicoPressure::kZeroAdcCount and PicoPressure::kAdcPerKpa
// with hardware once the divider is fitted.

PicoModule module(HAPTIC_I2C_ADDRESS, HAPTIC_MODULE_KIND);

void setup() {
  Serial1.begin(115200);
  analogReadResolution(12);
  module.begin();
}

void loop() {
  const int16_t raw = (int16_t)analogRead(A0);

  int16_t values[kMaxPayloadWords] = {};
  const ModuleStatus status = PicoPressure::buildPayload(raw, values);
  Serial1.println("pressure=" + String(values[0]) + " raw=" + String(raw) + " status=" + String(status));

  module.publish(status, values, kMaxPayloadWords);
  delay(20);
}
