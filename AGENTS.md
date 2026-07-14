# AGENTS.md

This file provides guidance to Codex (Codex.ai/code) when working with code in this repository.

## Build & Upload

Build all targets:
```bash
pio run -e teensy_master -e pico_loadcell -e pico_pressure -e pico_encoder
```

Build one target:
```bash
pio run -e teensy_master
pio run -e pico_loadcell
```

Upload to hardware:
```bash
pio run -e teensy_master -t upload
pio run -e pico_loadcell -t upload
pio run -e pico_pressure -t upload
pio run -e pico_encoder -t upload
```

Serial monitor (115200 baud):
```bash
pio device monitor -b 115200
```

Run host-side unit tests without hardware:
```bash
pio test -e native_test
```

Embedded firmware environments intentionally set `test_ignore = *`; do not run
unit tests against Teensy/Pico envs unless hardware-targeted tests are added
explicitly.

VS Code tasks are configured for:
- `PIO: Test (native_test)` — default test task.
- `PIO: Build (all release)` — default build task.
- `PIO: Test + Build (host + firmware)` — runs tests, then release firmware builds.

The repo recommends `hbenl.vscode-test-explorer` and Microsoft's Test Adapter
Converter. Because `.vscode/settings.json` is ignored for local machine
settings, enable `testExplorer.useNativeTesting: true` in your user or
workspace settings if you want Test Explorer API-compatible providers to surface
tests in VS Code's native Testing tab. The reliable test command remains
`pio test -e native_test`; the converter does not run PlatformIO tests directly.
This repo also includes a local native Testing API extension at
`tools/vscode-platformio-unity-test-provider`; package/install it with the
`VS Code Extension: Install PlatformIO Unity Test Provider` task, then reload VS
Code to see PlatformIO Unity tests in the Testing tab.

Hardware validation is still done by reading the Teensy serial output and confirming packets arrive with correct checksums and incrementing sequence numbers.

## Architecture

This is a PlatformIO multi-environment firmware workspace. One Teensy 4.1 acts as the I2C master; multiple Raspberry Pi Pico boards act as I2C sensor-preprocessing modules.

**Data flow:** Each Pico reads its sensor(s), builds a `ModulePacket`, and holds it ready. The Teensy polls every 10 ms, reads 19 bytes from each known I2C address, validates the protocol version and XOR checksum, parses the module-specific payload, emits changed USB MIDI CC values, and optionally prints the result over serial.

**Shared libraries in `lib/`:**
- `HapticProtocol`: defines `ModulePacket` (19 bytes, packed), `ModuleKind`, `ModuleStatus`, `finalizePacket()`, and `validatePacket()`. Used by both Teensy and Pico targets.
- `PicoModuleCommon`: `PicoModule` class that wraps `Wire` in I2C target mode, handles `onRequest` ISR, and pulses the `IRQ` pin after `publish()`. All Pico `main.cpp` files instantiate one `PicoModule` and call `publish()` in their loop.
- `TeensyMasterCore`: host-testable Teensy master packet decode, polling state, timeout handling, and module-specific payload parsing.
- `PicoModuleCore`, `PicoLoadcellCore`, `PicoPressureCore`, `PicoEncoderCore`: host-testable Pico packet publishing and module-specific payload/status logic.

**I2C addresses** are set per-environment via `-D HAPTIC_I2C_ADDRESS` compile flags: `0x20` (loadcell), `0x21` (pressure), `0x22` (encoder).

**MIDI CC mapping** is defined in `TeensyMasterCore`: CC 1 flywheel velocity, 2 flywheel direction, 3 pneumatic pressure, 4 spring tension, 5 spring acoustic, 6 lean total, 7 lean balance, 8 matrix centroid X, 9 matrix centroid Y, 10 matrix pressure, 11 joystick 1 X, 12 joystick 1 Y, 13 joystick 2 X, 14 joystick 2 Y. Current firmware emits CCs 1–5 on MIDI channel 1 from the available modules and reserves 6–14. `MidiControlChangeCache` suppresses duplicate CC values before USB transmission. This is USB MIDI 1.0-compatible output; MIDI 2.0 UMP is planned but not implemented.

**Packet layout** (`ModulePacket`, 19 bytes, little-endian):
- bytes 0–3: `protocolVersion`, `moduleKind`, `status`, `sequence`
- bytes 4–5: `idAdc` (ADC reading of the `ID/ADDR` pin — static identity hint)
- bytes 6–17: `payload[6]` — six `int16_t` values whose meaning is module-specific
- byte 18: XOR checksum over all 19 bytes with checksum field zeroed

**IRQ pin** (default GP6): Pico asserts it HIGH after `publish()`, Teensy clears it by reading. The Teensy can also poll without IRQ.

**ID pin** (default GP26 / ADC0): sampled at boot and included in every packet; intended for resistor-ladder board identification, not yet used in logic.

## Current State of Each Module

- `teensy_master`: polls all three addresses at 10 ms intervals; validates and parses packets; emits changed USB MIDI CC values for the load-cell, pressure, and encoder modules; and optionally prints named readings over serial. No MIDI 2.0 UMP, OSC, or CV output yet.
- `pico_loadcell`: reads two HX711-backed load cells, clamps values to `int16_t`, and reports `SENSOR_FAULT` on HX711 signal timeout. Real calibration is still required.
- `pico_pressure`: reads MPX5010DP gauge pressure sensor (0–10 kPa) via A0. Voltage divider conditioning stage not yet designed; calibration constants (`kZeroAdcCount`, `kAdcPerKpa`) are placeholders.
- `pico_encoder`: interrupt-based quadrature decoder on GP14/GP15; publishes position, velocity in 0.1 RPM units, and direction (`-1`, `0`, `1`).

## Key Constraints

- Keep I2C pullups to 3.3V — do not pull `SDA`/`SCL` to 5V.
- Power Pico modules from `VSYS` (connector 5V rail); logic runs at 3.3V.
- `ModulePacket` must remain exactly 19 bytes (`static_assert` enforces this). Changing the struct requires bumping `kProtocolVersion` and updating both master and all modules.
- `PicoModule` uses a static singleton (`instance()`) so only one `PicoModule` can exist per Pico binary.
- `publish()` uses `noInterrupts()`/`interrupts()` to protect the packet while the `onRequest` ISR may fire simultaneously.
