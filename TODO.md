# TODO

## Critical Bring-Up

- [ ] Confirm Pico I2C target support works with the selected PlatformIO RP2040 Arduino framework.
- [ ] Explicitly set Pico I2C pins if framework defaults are not `GP4` / `GP5`.
- [ ] Upload `teensy_master` to Teensy 4.0.
- [ ] Upload `pico_loadcell` to one Raspberry Pi Pico.
- [ ] Wire one Pico to Teensy over I2C with common ground.
- [ ] Confirm Teensy serial monitor receives valid packets from address `0x20`.
- [ ] Confirm checksum failure handling by disconnecting/reconnecting the module.

## Load-Cell Module

- [ ] Choose HX711 library or write minimal HX711 driver.
- [ ] Assign HX711 pins for load cell A.
- [ ] Assign HX711 pins for load cell B.
- [ ] Replace dummy analog reads with HX711 readings.
- [ ] Add tare command or boot-time tare behavior.
- [ ] Add simple low-pass filtering.
- [ ] Add sensor fault status for missing HX711 data.

## Pressure Module

- [ ] Design voltage scaling for MPXV7002DP output into Pico ADC range.
- [ ] Measure real ADC range at rest and under pressure.
- [ ] Convert ADC value to normalized pressure signal.
- [ ] Add calibration constants.
- [ ] Add overrange/fault status.

## Encoder Module

- [ ] Confirm encoder output type: open collector, open drain, or active logic.
- [ ] Confirm safe pullup voltage.
- [ ] Replace polling decoder with interrupt-based quadrature if needed.
- [ ] Add velocity estimate.
- [ ] Add wrap/range behavior if the encoder maps to a bounded control.

## Teensy Master

- [ ] Add module discovery scan.
- [ ] Add per-module timeout/fault state.
- [ ] Add readable serial debug output toggle.
- [ ] Add MIDI mapping for first load-cell gesture.
- [ ] Add OSC output after basic MIDI behavior works.
- [ ] Add CV/Gate bridge logic later, after sensor packets are stable.

## Documentation

- [ ] Add wiring photo once the first Pico is connected.
- [ ] Document the final I2C pins used on Teensy and Pico.
- [ ] Document packet fields and module status codes.
- [ ] Link this firmware repo from the Obsidian connector standard note.

