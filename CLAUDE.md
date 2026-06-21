# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

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

There are no automated tests. Validation is done by reading the Teensy serial output and confirming packets arrive with correct checksums and incrementing sequence numbers.

## Architecture

This is a PlatformIO multi-environment firmware workspace. One Teensy 4.0 acts as the I2C master; multiple Raspberry Pi Pico boards act as I2C sensor-preprocessing modules.

**Data flow:** Each Pico reads its sensor(s), builds a `ModulePacket`, and holds it ready. The Teensy polls every 10 ms, reads 19 bytes from each known I2C address, validates the protocol version and XOR checksum, and prints the result over serial.

**Shared libraries in `lib/`:**
- `HapticProtocol`: defines `ModulePacket` (19 bytes, packed), `ModuleKind`, `ModuleStatus`, `finalizePacket()`, and `validatePacket()`. Used by both Teensy and Pico targets.
- `PicoModuleCommon`: `PicoModule` class that wraps `Wire` in I2C target mode, handles `onRequest` ISR, and pulses the `IRQ` pin after `publish()`. All Pico `main.cpp` files instantiate one `PicoModule` and call `publish()` in their loop.

**I2C addresses** are set per-environment via `-D HAPTIC_I2C_ADDRESS` compile flags: `0x20` (loadcell), `0x21` (pressure), `0x22` (encoder).

**Packet layout** (`ModulePacket`, 19 bytes, little-endian):
- bytes 0–3: `protocolVersion`, `moduleKind`, `status`, `sequence`
- bytes 4–5: `idAdc` (ADC reading of the `ID/ADDR` pin — static identity hint)
- bytes 6–17: `payload[6]` — six `int16_t` values whose meaning is module-specific
- byte 18: XOR checksum over all 19 bytes with checksum field zeroed

**IRQ pin** (default GP6): Pico asserts it HIGH after `publish()`, Teensy clears it by reading. The Teensy can also poll without IRQ.

**ID pin** (default GP26 / ADC0): sampled at boot and included in every packet; intended for resistor-ladder board identification, not yet used in logic.

## Current State of Each Module

- `teensy_master`: polls all three addresses at 10 ms intervals; prints packets over serial. No MIDI/OSC/CV output yet.
- `pico_loadcell`: publishes dummy `analogRead(A0/A1)` values. HX711 driver not yet written.
- `pico_pressure`: publishes dummy `analogRead(A0)`. MPXV7002DP conditioning stage not yet designed.
- `pico_encoder`: polling quadrature decoder on GP14/GP15; position split across `payload[0]` (low word) and `payload[1]` (high word). Interrupt-based decode not yet implemented.

## Key Constraints

- Keep I2C pullups to 3.3V — do not pull `SDA`/`SCL` to 5V.
- Power Pico modules from `VSYS` (connector 5V rail); logic runs at 3.3V.
- `ModulePacket` must remain exactly 19 bytes (`static_assert` enforces this). Changing the struct requires bumping `kProtocolVersion` and updating both master and all modules.
- `PicoModule` uses a static singleton (`instance()`) so only one `PicoModule` can exist per Pico binary.
- `publish()` uses `noInterrupts()`/`interrupts()` to protect the packet while the `onRequest` ISR may fire simultaneously.
