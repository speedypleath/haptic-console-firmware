# Haptic Console Firmware Structure

This PlatformIO workspace uses one project with multiple environments:

- `teensy_master`: Teensy 4.0 central controller
- `pico_loadcell`: Pico module for two load cells
- `pico_pressure`: Pico module for MPXV7002DP pressure sensing
- `pico_encoder`: Pico module for the optical encoder

Shared code lives in `lib/`:

- `HapticProtocol`: packet layout, module IDs, checksums, status values
- `PicoModuleCommon`: reusable Pico I2C target behavior
- `TeensyMasterCore`: packet decoding, module-state tracking, sensor-to-MIDI
  conversion, and MIDI Control Change de-duplication

## Build Commands

```bash
pio run -e teensy_master
pio run -e pico_loadcell
pio run -e pico_pressure
pio run -e pico_encoder
```

## Upload Commands

```bash
pio run -e teensy_master -t upload
pio run -e pico_loadcell -t upload
pio run -e pico_pressure -t upload
pio run -e pico_encoder -t upload
```

## Connector Assumptions

- Teensy is the I2C master.
- Pico modules are I2C targets.
- I2C addresses are currently fixed per PlatformIO environment:
  - `0x20`: load-cell module
  - `0x21`: pressure module
  - `0x22`: encoder module
- `SDA` and `SCL` must be pulled up to 3.3V, not 5V.
- `5V` is power only.
- Pico modules should be powered from `VSYS` when using the connector 5V rail.
- The `ID / ADDR` pin is read as an analog identity hint on `GP26 / ADC0`.

## MIDI Output

The Teensy build uses `USB_MIDI_SERIAL`, which exposes USB MIDI 1.0-compatible
output alongside the serial debug interface. On each successful module poll,
the master converts the current sensor readings into channel-1 Control Change
messages. It sends a value only when that controller value changes, preventing
the 100 Hz I2C poll loop from repeatedly transmitting unchanged data.

| Module | MIDI controls |
|---|---|
| Load-cell | CC 4 `spring_tension`, CC 5 `spring_acoustic` |
| Pressure | CC 3 `pneumatic_pressure` |
| Encoder | CC 1 `flywheel_velocity`, CC 2 `flywheel_direction` |

CC 6–14 remain reserved for future modules. The complete control-surface
mapping, including planned numpad/button Note On/Off events and the MIDI 2.0
UMP direction, is documented in the repository README.

The MIDI 2.0 UMP endpoint is not implemented yet. The current firmware sends
MIDI 1.0-compatible CC messages only.

## Tests

Host-side tests in `test/test_teensy_master_core` cover packet validation,
module payload parsing, sensor-to-MIDI mappings, fault suppression, and MIDI
CC de-duplication. Run them with:

```bash
pio test -e native_test
```

## Development Order

1. Build and upload one Pico module with dummy data.
2. Confirm the Teensy can read valid packets over I2C.
3. Replace dummy values with real sensor reads, one module at a time.
4. Add calibration and filtering after the packet path is stable.
5. Verify USB MIDI output and its mappings with a MIDI monitor.
