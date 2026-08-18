#include <Arduino.h>
#include <Wire.h>
#include <HapticProtocol.h>
#include <TeensyMasterCore.h>

using namespace Haptic;
namespace TM = Haptic::TeensyMaster;

static TM::ModuleState states[TM::kNumModules];
static uint32_t    lastPollMs  = 0;
static bool        debugOutput = true;
static TM::MidiControlChangeCache midiCache;

// ---------------------------------------------------------------------------
// Panel input bring-up harness: arcade buttons, a 5-pin joystick, and a
// numpad matrix wired directly to the Teensy for wiring/debounce validation
// ahead of the real panel loom. All switches are wired common-to-GND and read
// with internal pullups (pressed = LOW). Remove once the real panel loom
// replaces this harness.
//
// Arcade button, 2-pin (switch only).
static constexpr uint8_t kArcade2PinSwitchPin = 2;
//
// Arcade button, 4-pin (switch + LED). LED lights while the switch is held.
static constexpr uint8_t kArcade4PinSwitchPin = 3;
static constexpr uint8_t kArcade4PinLedPin = 4;
//
// Joystick, 5-pin (common + 4 microswitch directions). Digital directions
// only print for now — the README's joystick CC lane expects a continuous
// axis, which this discrete harness does not attempt to synthesize.
static constexpr uint8_t kJoystickUpPin = 5;
static constexpr uint8_t kJoystickDownPin = 6;
static constexpr uint8_t kJoystickLeftPin = 7;
static constexpr uint8_t kJoystickRightPin = 8;
//
// Numpad, 4x3 matrix (rows driven, columns read with pullups).
static constexpr uint8_t kNumpadRowPins[4] = {9, 10, 11, 12};
static constexpr uint8_t kNumpadColPins[3] = {14, 15, 16};
static constexpr char kNumpadKeymap[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'},
};

static TM::DebouncedInput arcade2PinInput;
static TM::DebouncedInput arcade4PinInput;
static bool arcade4PinLedLatched = false;
static TM::DebouncedInput joystickUpInput;
static TM::DebouncedInput joystickDownInput;
static TM::DebouncedInput joystickLeftInput;
static TM::DebouncedInput joystickRightInput;
static TM::DebouncedInput numpadInputs[4][3];

void setUpPanelTestHarness() {
  pinMode(kArcade2PinSwitchPin, INPUT_PULLUP);
  pinMode(kArcade4PinSwitchPin, INPUT_PULLUP);
  pinMode(kArcade4PinLedPin, OUTPUT);
  digitalWrite(kArcade4PinLedPin, LOW);

  pinMode(kJoystickUpPin, INPUT_PULLUP);
  pinMode(kJoystickDownPin, INPUT_PULLUP);
  pinMode(kJoystickLeftPin, INPUT_PULLUP);
  pinMode(kJoystickRightPin, INPUT_PULLUP);

  for (uint8_t row = 0; row < 4; ++row) {
    pinMode(kNumpadRowPins[row], OUTPUT);
    digitalWrite(kNumpadRowPins[row], HIGH);
  }
  for (uint8_t col = 0; col < 3; ++col) {
    pinMode(kNumpadColPins[col], INPUT_PULLUP);
  }
}

void sendPanelNote(uint8_t note, TM::InputEdge edge) {
#if defined(USB_MIDI) || defined(USB_MIDI_SERIAL)
  if (edge == TM::InputEdge::Pressed) {
    usbMIDI.sendNoteOn(note, TM::kMidiNoteVelocity, TM::kMidiPanelChannel);
  } else if (edge == TM::InputEdge::Released) {
    usbMIDI.sendNoteOff(note, 0, TM::kMidiPanelChannel);
  }
#else
  (void)note;
  (void)edge;
#endif
}

void printPanelEdge(const char *label, TM::InputEdge edge) {
  if (!debugOutput || edge == TM::InputEdge::None) return;
  Serial.print(label);
  Serial.println(edge == TM::InputEdge::Pressed ? " pressed" : " released");
}

void pollArcadeButtons(uint32_t now) {
  const TM::InputEdge edge2Pin = TM::updateDebouncedInput(
      arcade2PinInput, digitalRead(kArcade2PinSwitchPin) == LOW, now);
  printPanelEdge("arcade2pin", edge2Pin);
  sendPanelNote(TM::kMidiNoteActionBase, edge2Pin);

  const TM::InputEdge edge4Pin = TM::updateDebouncedInput(
      arcade4PinInput, digitalRead(kArcade4PinSwitchPin) == LOW, now);
  printPanelEdge("arcade4pin", edge4Pin);
  sendPanelNote(TM::kMidiNoteControlBase, edge4Pin);
  // Latch: each press toggles the LED on/off; release is ignored for the LED
  // (the switch itself is still momentary and keeps sending MIDI normally).
  if (edge4Pin == TM::InputEdge::Pressed) {
    arcade4PinLedLatched = !arcade4PinLedLatched;
    digitalWrite(kArcade4PinLedPin, arcade4PinLedLatched ? HIGH : LOW);
  }
}

void pollJoystick(uint32_t now) {
  const TM::InputEdge up = TM::updateDebouncedInput(
      joystickUpInput, digitalRead(kJoystickUpPin) == LOW, now);
  const TM::InputEdge down = TM::updateDebouncedInput(
      joystickDownInput, digitalRead(kJoystickDownPin) == LOW, now);
  const TM::InputEdge left = TM::updateDebouncedInput(
      joystickLeftInput, digitalRead(kJoystickLeftPin) == LOW, now);
  const TM::InputEdge right = TM::updateDebouncedInput(
      joystickRightInput, digitalRead(kJoystickRightPin) == LOW, now);
  printPanelEdge("joystick up", up);
  printPanelEdge("joystick down", down);
  printPanelEdge("joystick left", left);
  printPanelEdge("joystick right", right);
}

void pollNumpad(uint32_t now) {
  for (uint8_t row = 0; row < 4; ++row) {
    digitalWrite(kNumpadRowPins[row], LOW);
    delayMicroseconds(5);  // let the driven row settle before sampling columns

    for (uint8_t col = 0; col < 3; ++col) {
      const bool pressed = digitalRead(kNumpadColPins[col]) == LOW;
      const TM::InputEdge edge =
          TM::updateDebouncedInput(numpadInputs[row][col], pressed, now);
      if (edge == TM::InputEdge::None) continue;

      const char key = kNumpadKeymap[row][col];
      if (debugOutput) {
        Serial.print("numpad ");
        Serial.print(key);
        Serial.println(edge == TM::InputEdge::Pressed ? " pressed" : " released");
      }

      uint8_t note = 0;
      if (TM::numpadNoteForKey(key, note)) {
        sendPanelNote(note, edge);
      }
    }

    digitalWrite(kNumpadRowPins[row], HIGH);
  }
}

void pollPanelTestHarness(uint32_t now) {
  pollArcadeButtons(now);
  pollJoystick(now);
  pollNumpad(now);
}

bool readModule(uint8_t address, ModulePacket &packet) {
  const uint8_t expected = sizeof(ModulePacket);
  const uint8_t received = Wire.requestFrom(address, expected);
  if (received != expected) {
    while (Wire.available()) Wire.read();
    return false;
  }
  uint8_t *dst = reinterpret_cast<uint8_t *>(&packet);
  for (uint8_t i = 0; i < expected; ++i) dst[i] = Wire.read();
  return TM::decodePacketBytes(reinterpret_cast<const uint8_t *>(&packet),
                               sizeof(packet), packet) ==
         TM::PacketDecodeResult::Ok;
}

void printPacket(uint8_t address, const ModulePacket &packet) {
  Serial.print("0x");
  Serial.print(address, HEX);
  Serial.print(" ");
  Serial.print(moduleKindName(packet.moduleKind));
  Serial.print(" seq=");
  Serial.print(packet.sequence);
  Serial.print(" idAdc=");
  Serial.print(packet.idAdc);
  if (packet.moduleKind == MODULE_KIND_LOADCELL) {
    const TM::LoadcellReading reading = TM::parseLoadcellReading(packet);
    Serial.print(" cellA=");
    Serial.print(reading.cellA);
    Serial.print(" cellB=");
    Serial.print(reading.cellB);
  } else if (packet.moduleKind == MODULE_KIND_PRESSURE) {
    const TM::PressureReading reading = TM::parsePressureReading(packet);
    Serial.print(" pressure_cKpa=");
    Serial.print(reading.pressureCentikpa);
    Serial.print(" raw=");
    Serial.print(reading.rawAdc);
    Serial.print(" zero=");
    Serial.print(reading.zeroAdc);
  } else if (packet.moduleKind == MODULE_KIND_ENCODER) {
    const TM::EncoderReading reading = TM::parseEncoderReading(packet);
    Serial.print(" position=");
    Serial.print(reading.position);
    Serial.print(" velocity_0.1rpm=");
    Serial.print(reading.velocityTenthRpm);
    Serial.print(" direction=");
    Serial.print(reading.direction);
  } else {
    Serial.print(" values=");
    for (uint8_t i = 0; i < kMaxPayloadWords; ++i) {
      Serial.print(packet.payload[i]);
      if (i + 1 < kMaxPayloadWords) Serial.print(",");
    }
  }
  Serial.println();
}

void sendMidiForPacket(const ModulePacket &packet) {
  TM::MidiControlChange changes[3] = {};
  const uint8_t count = TM::midiChangesForPacket(packet, changes, 3);
  for (uint8_t i = 0; i < count; ++i) {
    const uint8_t control = changes[i].control;
    const uint8_t value = changes[i].value;
    if (!TM::shouldSendMidiControlChange(midiCache, control, value)) continue;
#if defined(USB_MIDI) || defined(USB_MIDI_SERIAL)
    usbMIDI.sendControlChange(control, value, TM::kMidiDefaultChannel);
#endif
  }
#if defined(USB_MIDI) || defined(USB_MIDI_SERIAL)
  while (usbMIDI.read()) {
    // Drain inbound USB MIDI so host-side buffers do not accumulate.
  }
#endif
}

void scanModules() {
  Serial.println("Scanning for modules...");
  for (uint8_t i = 0; i < TM::kNumModules; ++i) {
    Wire.beginTransmission(TM::kModuleAddresses[i]);
    const bool present = (Wire.endTransmission() == 0);
    TM::recordScanResult(states[i], present, millis());
    Serial.print("  0x");
    Serial.print(TM::kModuleAddresses[i], HEX);
    Serial.println(present ? " found" : " not found");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);
  delay(500);
  Serial.println("Haptic Console Teensy master");
  Serial.println("Serial commands: 'd' = toggle debug output");
  setUpPanelTestHarness();
  scanModules();
}

void loop() {
  // Toggle debug output on 'd' keypress.
  while (Serial.available()) {
    if (Serial.read() == 'd') {
      debugOutput = !debugOutput;
      Serial.println(debugOutput ? "debug on" : "debug off");
    }
  }

  const uint32_t now = millis();
  pollPanelTestHarness(now);

  if (!TM::isPollDue(now, lastPollMs)) return;
  lastPollMs = now;

  for (uint8_t i = 0; i < TM::kNumModules; ++i) {
    if (!states[i].active) continue;

    ModulePacket packet;
    if (readModule(TM::kModuleAddresses[i], packet)) {
      TM::recordReadSuccess(states[i], now);
      sendMidiForPacket(packet);
      if (debugOutput) printPacket(TM::kModuleAddresses[i], packet);
    } else {
      if (TM::recordReadFailure(states[i], now)) {
        Serial.print("timeout: 0x");
        Serial.println(TM::kModuleAddresses[i], HEX);
      }
    }
  }
}
