#include <Arduino.h>
#include <HX711_ADC.h>
#include <PicoModuleCommon.h>
#include <PicoLoadcellCore.h>

using namespace Haptic;

// Pins — avoid GP4/5 (I2C), GP6 (IRQ), GP26 (ID/ADC).
static constexpr uint8_t kLcADout = 10;
static constexpr uint8_t kLcAClk  = 11;
static constexpr uint8_t kLcBDout = 12;
static constexpr uint8_t kLcBClk  = 13;

// CALIBRATE: tare with no load (send 't'), place a known mass, then send
// "a<grams>" or "b<grams>" (e.g. "a5000") over serial to compute and print a
// new calFactor for that channel. Hardcode the printed value here and reflash.
static constexpr float kCalFactorA = 1.0f;
static constexpr float kCalFactorB = 1.0f;

HX711_ADC lcA(kLcADout, kLcAClk);
HX711_ADC lcB(kLcBDout, kLcBClk);
PicoModule module(HAPTIC_I2C_ADDRESS, HAPTIC_MODULE_KIND);

void handleCalibrationSerial() {
  if (Serial.available() <= 0) return;

  const char command = Serial.read();
  if (command == 't') {
    lcA.tareNoDelay();
    lcB.tareNoDelay();
    Serial.println("Taring both channels...");
    return;
  }

  if (command != 'a' && command != 'b') return;

  const float knownMassGrams = Serial.parseFloat();
  if (knownMassGrams <= 0) return;

  HX711_ADC &cell = (command == 'a') ? lcA : lcB;
  cell.refreshDataSet();
  const float newCalFactor = cell.getNewCalibration(knownMassGrams);
  Serial.print("New calFactor for channel ");
  Serial.print(command);
  Serial.print(" = ");
  Serial.println(newCalFactor, 4);
  Serial.println("Hardcode this into kCalFactorA/B in main.cpp and reflash.");
}

void setup() {
  Serial.begin(115200);

  lcA.begin();
  lcB.begin();
  lcA.setCalFactor(kCalFactorA);
  lcB.setCalFactor(kCalFactorB);

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
  handleCalibrationSerial();

  const ModuleStatus status = PicoLoadcell::statusForTimeoutFlags(
      lcA.getSignalTimeoutFlag(), lcB.getSignalTimeoutFlag());
  if (status != MODULE_STATUS_OK) {
    int16_t values[kMaxPayloadWords] = {};
    Serial.println("loadcell FAULT timeoutA=" + String(lcA.getSignalTimeoutFlag()) +
                    " timeoutB=" + String(lcB.getSignalTimeoutFlag()) +
                    " status=" + String(status));
    module.publish(status, values, kMaxPayloadWords);
    delay(5);
    return;
  }

  // getData() returns calFactor-scaled float. Clamp to int16 range.
  // With kCalFactorA/B=1.0 the raw 24-bit counts will saturate until calibration is set.
  int16_t values[kMaxPayloadWords] = {};
  PicoLoadcell::buildPayload(static_cast<int32_t>(lcA.getData()),
                             static_cast<int32_t>(lcB.getData()), values);
  Serial.println("loadcell A=" + String(values[0]) + " B=" + String(values[1]) +
                  " status=" + String(status) +
                  " spsA=" + String(lcA.getSPS()) + " spsB=" + String(lcB.getSPS()) +
                  " offsetA=" + String(lcA.getTareOffset()) + " offsetB=" + String(lcB.getTareOffset()));

  module.publish(status, values, kMaxPayloadWords);
  delay(5);
}
