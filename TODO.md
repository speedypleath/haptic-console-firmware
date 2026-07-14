# TODO

## Critical Bring-Up

- [x] Build all documented release firmware targets.
- [x] Implement I2C master polling loop on Teensy.
- [x] Implement I2C target mode and ModulePacket publishing on Picos.
- [x] Add packet validation (length, protocol version, XOR checksum).
- [x] Add per-module timeout/fault state machine (kMissLimit=5, kModuleTimeoutMs=200).
- [ ] Upload `teensy_master` to Teensy 4.1.
- [ ] Upload `pico_loadcell` to one Raspberry Pi Pico.
- [ ] Upload `pico_pressure` to one Raspberry Pi Pico.
- [ ] Upload `pico_encoder` to one Raspberry Pi Pico.
- [ ] Wire one Pico to Teensy over I2C with common ground.
- [ ] Confirm Teensy serial monitor receives valid packets from address `0x20`.
- [ ] Confirm checksum failure handling by disconnecting/reconnecting the module.
- [ ] Confirm USB MIDI output reaches a host MIDI monitor.

## Load-Cell Module

- [x] Choose HX711 library or write minimal HX711 driver.
- [x] Assign HX711 pins.
- [x] Implement dual HX711 driver (GP10-13).
- [x] Implement payload construction (int16 clamped cells + fault status).
- [x] Add sensor fault status for missing HX711 data.
- [x] Host-side tests: payload layout, int16 clamping, timeout fault.
- [ ] Add tare command or boot-time tare behaviour.
- [ ] Add simple low-pass filtering.
- [ ] Measure and set real HX711 calibration factors (currently placeholder 1.0f).

## Pressure Module

- [x] Convert ADC value to normalised pressure signal (centipascals).
- [x] Add overrange/fault status.
- [x] Host-side tests: ADC-to-centikPa, negative clamping, overrange detection.
- [ ] Design voltage scaling for MPX5010DP output into Pico ADC range.
- [ ] Measure and replace placeholder calibration constants (kZeroAdcCount=166, kAdcPerKpa=376, kOverrangeAdc=200).

## Encoder Module

- [x] Confirm encoder output type: NPN open collector.
- [x] Confirm safe pullup voltage: Pico INPUT_PULLUP to 3.3V.
- [x] Replace polling decoder with interrupt-based quadrature.
- [x] Implement velocity estimation (50 ms window, 0.1 RPM units).
- [x] Implement direction detection (-1/0/+1).
- [x] Host-side tests: quadrature table, velocity math, payload round-trip.
- [ ] Add wrap/range behaviour if the encoder maps to a bounded control.

## Teensy Master -- MIDI 1.0

- [x] Add module discovery scan.
- [x] Add per-module timeout/fault state.
- [x] Add readable serial debug output toggle.
- [x] Add USB MIDI CC mapping (CC 1-5 active, CC 6-14 reserved):
  - CC 1: flywheel_velocity
  - CC 2: flywheel_direction
  - CC 3: pneumatic_pressure
  - CC 4: spring_tension
  - CC 5: spring_acoustic
- [x] Add MidiControlChangeCache for duplicate suppression.
- [x] Add midiChangesForPacket() for module-kind dispatch.
- [x] Host-side tests: CC numbers, scaling, fault suppression, cache dedup.
- [x] Document in README, docs/firmware-structure.md, and Obsidian vault note.

## Teensy Master -- MIDI 2.0 UMP

- [ ] Extract transport-neutral HapticMidiEngine.
- [ ] Preserve MIDI 1.0 sender as one backend.
- [ ] Add UMP packet builder and endpoint/stream discovery state machine.
- [ ] Transmit controls as 32-bit MIDI 2.0 Channel Controller values.
- [ ] Add host protocol selection (default MIDI 1.0, switch to 2.0 on host request).
- [ ] Defer JR timestamps (10 ms cadence not yet justifying clock-sync).
- [ ] Add MIDI-CI discovery and read-only controller metadata.
- [ ] Add per-note interaction alongside a note-producing module.

## Teensy Master -- Additional

- [ ] Add numpad/panel button scanning (channel 16, notes 36-58).
- [ ] Add OSC output.
- [ ] Add CV/Gate bridge logic (Hybrid Bridge module).

## Documentation

- [x] Document I2C packet format.
- [x] Document connector standard.
- [x] Document firmware structure and MIDI mapping.
- [x] Create Obsidian vault note: Notes/Haptic Console - Firmware Architecture.md.
- [ ] Add wiring photo once the first Pico is connected.
- [ ] Document the final I2C pins used on Teensy and Pico.
- [ ] Link this firmware repo from the Obsidian connector standard note.
