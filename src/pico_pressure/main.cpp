#include <Arduino.h>
#include <PicoModuleCommon.h>

using namespace Haptic;

// CALIBRATE: measure these with hardware after the voltage divider is fitted.
// kZeroAdcCount: 12-bit ADC reading when sensor sees 0 kPa differential
//                (MPXV7002DP Vout ~2.5 V × voltage-divider ratio, scaled to 3.3 V reference).
// kAdcPerUnit:   ADC counts per output unit (1 unit = 0.01 kPa).
// kOverrangeAdc: extra ADC headroom before declaring SENSOR_FAULT (beyond ±2 kPa swing).
static constexpr int16_t kZeroAdcCount  = 2048;  // placeholder — mid-scale 12-bit
static constexpr int16_t kAdcPerUnit    = 100;    // placeholder — needs hardware measurement
static constexpr int16_t kOverrangeAdc  = 300;    // tolerance counts beyond ±2 kPa swing

PicoModule module(HAPTIC_I2C_ADDRESS, HAPTIC_MODULE_KIND);

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  module.begin();
}

void loop() {
  const int16_t raw = (int16_t)analogRead(A0);
  const int16_t delta = raw - kZeroAdcCount;

  // Convert to 0.01 kPa units. Range ±200 for ±2 kPa (fits int16_t well).
  const int16_t pressure = (int32_t)delta * 100 / kAdcPerUnit;

  // Overrange: ADC swing exceeds the ±2 kPa sensor spec plus tolerance.
  const bool overrange = abs(delta) > (kAdcPerUnit * 200 + kOverrangeAdc);
  const ModuleStatus status = overrange ? MODULE_STATUS_SENSOR_FAULT : MODULE_STATUS_OK;

  int16_t values[kMaxPayloadWords] = {};
  values[0] = pressure;
  values[1] = raw;
  values[2] = kZeroAdcCount;

  module.publish(status, values, kMaxPayloadWords);
  delay(5);
}
