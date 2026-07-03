#include <Arduino.h>
#include <PicoModuleCommon.h>

using namespace Haptic;

// MPX5010DP: gauge pressure sensor, 0–10 kPa range, 5 V supply.
// Transfer function: Vout = Vs × (0.09 × P + 0.04), P in kPa, Vs = 5 V.
//   0 kPa  → 0.20 V
//   10 kPa → 4.70 V
// A resistive voltage divider is required to keep Vout within the Pico's
// 3.3 V ADC range (e.g. 33 kΩ / 68 kΩ → ×0.67 ratio).
//
// CALIBRATE: measure these constants with hardware once the divider is fitted.
// kZeroAdcCount: 12-bit ADC count at 0 kPa (0.20 V × divider ratio, ref 3.3 V).
// kAdcPerKpa:    ADC counts per kPa.
// kOverrangeAdc: extra ADC headroom above full scale before declaring SENSOR_FAULT.
static constexpr int16_t kZeroAdcCount = 166;  // placeholder — 0.20 V × 0.67 / 3.3 V × 4095
static constexpr int16_t kAdcPerKpa    = 376;  // placeholder — needs hardware measurement
static constexpr int16_t kOverrangeAdc = 200;  // tolerance counts beyond 10 kPa

PicoModule module(HAPTIC_I2C_ADDRESS, HAPTIC_MODULE_KIND);

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  module.begin();
}

void loop() {
  const int16_t raw   = (int16_t)analogRead(A0);
  const int16_t delta = raw - kZeroAdcCount;

  // Convert to 0.01 kPa units; 0–1000 spans the full 0–10 kPa range.
  const int16_t pressure = (int16_t)constrain(
      (int32_t)delta * 100 / kAdcPerKpa, 0, 32767);

  // Fault if ADC is above 10 kPa full scale + tolerance, or implausibly below zero.
  const bool overrange = raw > kZeroAdcCount + kAdcPerKpa * 10 + kOverrangeAdc
                      || delta < -kOverrangeAdc;
  const ModuleStatus status = overrange ? MODULE_STATUS_SENSOR_FAULT : MODULE_STATUS_OK;

  int16_t values[kMaxPayloadWords] = {};
  values[0] = pressure;
  values[1] = raw;
  values[2] = kZeroAdcCount;
  Serial.println("pressure=" + String(pressure) + " raw=" + String(raw) + " status=" + String(status));

  module.publish(status, values, kMaxPayloadWords);
  delay(20);
}
