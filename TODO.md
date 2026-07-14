# TODO

## Critical Bring-Up

- [ ] Confirm Pico I2C target support works with the selected PlatformIO RP2040 Arduino framework.
- [ ] Explicitly set Pico I2C pins if framework defaults are not `GP4` / `GP5`.
- [x] Build all documented release firmware targets.
- [ ] Upload `teensy_master` to Teensy 4.1.
- [ ] Upload `pico_loadcell` to one Raspberry Pi Pico.
- [ ] Wire one Pico to Teensy over I2C with common ground.
- [ ] Confirm Teensy serial monitor receives valid packets from address `0x20`.
- [ ] Confirm checksum failure handling by disconnecting/reconnecting the module.

## Load-Cell Module

- [x] Choose HX711 library or write minimal HX711 driver.
- [x] Assign HX711 pins for load cell A.
- [x] Assign HX711 pins for load cell B.
- [x] Replace dummy analog reads with HX711 readings.
- [ ] Add tare command or boot-time tare behavior.
- [ ] Add simple low-pass filtering.
- [x] Add sensor fault status for missing HX711 data.
- [ ] Measure and set real HX711 calibration factors.

## Pressure Module

- [ ] Design voltage scaling for MPX5010DP output into Pico ADC range.
- [x] Measure real ADC range at rest and under pressure.
- [x] Convert ADC value to normalized pressure signal.
- [ ] Add measured calibration constants.
- [x] Add overrange/fault status.

## Encoder Module

- [x] Confirm encoder output type: NPN open collector.
- [x] Confirm safe pullup voltage: Pico `INPUT_PULLUP` to 3.3V.
- [x] Replace polling decoder with interrupt-based quadrature if needed.
- [x] Add velocity estimate.
- [ ] Add wrap/range behavior if the encoder maps to a bounded control.

## Teensy Master

- [x] Add module discovery scan.
- [x] Add per-module timeout/fault state.
- [x] Add readable serial debug output toggle.
- [x] Add initial USB MIDI CC mapping for load cell, pressure, and encoder readings.
- [ ] Add OSC output after basic MIDI behavior works.
- [ ] Add CV/Gate bridge logic later, after sensor packets are stable.

## Documentation

- [ ] Add wiring photo once the first Pico is connected.
- [ ] Document the final I2C pins used on Teensy and Pico.
- [ ] Document packet fields and module status codes.
- [ ] Link this firmware repo from the Obsidian connector standard note.
