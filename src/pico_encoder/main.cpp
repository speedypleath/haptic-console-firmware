#include <Arduino.h>
#include <PicoModuleCommon.h>

using namespace Haptic;

// Taiss 600P/R optical encoder, AB quadrature, 5–24 V supply.
// With 4× decoding (both edges of both channels): 2400 counts per revolution.
//
// HARDWARE — voltage levels:
//   The encoder outputs are NPN open-collector. Power the encoder from 5 V (VSYS).
//   The Pico's INPUT_PULLUP (3.3 V, ~50 kΩ) serves as the pull-up resistor.
//   Do NOT add external pull-ups to 5 V — that would drive the Pico GPIO above 3.3 V.
//   If your encoder has push-pull outputs at supply voltage, a level shifter is required
//   before connecting to the Pico.
static constexpr uint8_t  kEncoderAPin    = 14;
static constexpr uint8_t  kEncoderBPin    = 15;
static constexpr int32_t  kPulsesPerRev   = 600;
static constexpr int32_t  kCountsPerRev   = kPulsesPerRev * 4;  // 2400

// Full quadrature decode table: index = (lastState << 2) | newState.
// +1 = CW, -1 = CCW, 0 = invalid or no change.
static const int8_t kQuadTable[16] = {
    0,  1, -1,  0,
   -1,  0,  0,  1,
    1,  0,  0, -1,
    0, -1,  1,  0,
};

static volatile int32_t encoderPosition  = 0;
static volatile uint8_t lastEncoderState = 0;

void encoderISR() {
  const uint8_t s = (digitalRead(kEncoderAPin) << 1) | digitalRead(kEncoderBPin);
  encoderPosition += kQuadTable[(lastEncoderState << 2) | s];
  lastEncoderState = s;
}

PicoModule module(HAPTIC_I2C_ADDRESS, HAPTIC_MODULE_KIND);

static int32_t  prevPosition      = 0;
static uint32_t prevTimeMs        = 0;
static int16_t  velocityTenthRpm  = 0;  // 0.1 RPM units

void setup() {
  Serial.begin(115200);
  pinMode(kEncoderAPin, INPUT_PULLUP);
  pinMode(kEncoderBPin, INPUT_PULLUP);

  lastEncoderState = (digitalRead(kEncoderAPin) << 1) | digitalRead(kEncoderBPin);
  attachInterrupt(digitalPinToInterrupt(kEncoderAPin), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(kEncoderBPin), encoderISR, CHANGE);

  prevTimeMs = millis();
  module.begin();
}

void loop() {
  noInterrupts();
  const int32_t pos = encoderPosition;
  interrupts();

  // Update velocity every 50 ms for stable low-speed readings.
  // At 10 RPM this yields ~20 counts per window — sufficient resolution.
  const uint32_t now = millis();
  const uint32_t dt  = now - prevTimeMs;
  if (dt >= 50) {
    const int32_t delta       = pos - prevPosition;
    const int32_t countsPerSec = (delta * 1000L) / (int32_t)dt;
    // 0.1 RPM = counts/sec × 60 × 10 / kCountsPerRev
    const int32_t tenthRpm    = (countsPerSec * 600L) / kCountsPerRev;
    velocityTenthRpm = (int16_t)constrain(tenthRpm, -32768, 32767);
    prevPosition = pos;
    prevTimeMs   = now;
  }

  int16_t values[kMaxPayloadWords] = {};
  values[0] = (int16_t)(pos & 0xFFFF);           // position low word
  values[1] = (int16_t)((pos >> 16) & 0xFFFF);   // position high word
  values[2] = velocityTenthRpm;                   // 0.1 RPM, signed

  module.publish(MODULE_STATUS_OK, values, kMaxPayloadWords);
  delay(1);
}
