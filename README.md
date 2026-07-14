# Haptic Console Firmware

PlatformIO firmware workspace for the Haptic Console v1.0 module bus.

The current architecture uses a Teensy 4.1 as the central controller and
Raspberry Pi Pico boards as sensor preprocessing modules. The module bus is
I2C-based: Pico modules prepare clean sensor packets, and the Teensy polls
those packets at a fixed interval.

## Project Layout

```text
.
├── platformio.ini
├── src/
│   ├── teensy_master/
│   ├── pico_loadcell/
│   ├── pico_pressure/
│   └── pico_encoder/
├── lib/
│   ├── HapticProtocol/
│   └── PicoModuleCommon/
└── docs/
    ├── firmware-structure.md
    ├── connector-standard.md
    └── i2c-packet-format.md
```

## Firmware Targets

- `teensy_master`: Teensy 4.1 I2C master and future MIDI/OSC/CV brain
- `pico_loadcell`: Pico module for two HX711-backed load cells
- `pico_pressure`: Pico module for MPX5010DP pressure sensing
- `pico_encoder`: Pico module for the optical encoder

## Build

Build everything:

```bash
pio run -e teensy_master -e pico_loadcell -e pico_pressure -e pico_encoder
```

Build one target:

```bash
pio run -e teensy_master
pio run -e pico_loadcell
```

## Host Unit Tests

Run host-side logic tests without Teensy or Pico hardware:

```bash
pio test -e native_test
```

Embedded firmware environments set `test_ignore = *`, so PlatformIO will not
try to upload/run these host tests on Teensy or Pico boards.

In VS Code, use `Terminal > Run Test Task...` and choose
`PIO: Test (native_test)`. This repo also recommends Test Explorer extensions:

- `hbenl.vscode-test-explorer` for the Test Explorer UI.
- `ms-vscode.test-adapter-converter` as the bridge from the Test Explorer API
  into VS Code's native Testing UI.

The reliable runner remains `pio test -e native_test`. The converter does not
run PlatformIO tests directly; it only exposes tests from Test Explorer
API-compatible providers in VS Code's native Testing tab. If the Testing tab
still shows no tests, use the configured VS Code test task or PlatformIO's
Project Tasks view.

For native VS Code Testing tab integration, this repo includes a local extension
under `tools/vscode-platformio-unity-test-provider`. Install it from VS Code via:

1. `Terminal > Run Task...`
2. `VS Code Extension: Install PlatformIO Unity Test Provider`
3. Reload the VS Code window.

After reload, the Testing tab should show `PlatformIO Unity` tests discovered
from `RUN_TEST(...)` calls in `test/**/test_main.cpp`.

## Upload

```bash
pio run -e teensy_master -t upload
pio run -e pico_loadcell -t upload
pio run -e pico_pressure -t upload
pio run -e pico_encoder -t upload
```

## Serial Monitor

```bash
pio device monitor -b 115200
```

## Debug Builds

Pico module debug environments are available for SWD/GDB bring-up through a
picoprobe-compatible CMSIS-DAP probe and OpenOCD:

```bash
pio run -e pico_loadcell_debug
pio run -e pico_pressure_debug
pio run -e pico_encoder_debug
```

## I2C Module Addresses

- `0x20`: load-cell module
- `0x21`: pressure module
- `0x22`: encoder module

## Connector Assumptions

The firmware assumes the Haptic Console 8-pin module connector:

- `GND`: shared ground
- `5V`: module power only, never logic
- `3V3`: logic reference / light accessory rail
- `SDA`: I2C data, pulled up to 3.3V
- `SCL`: I2C clock, pulled up to 3.3V
- `IRQ / DATA_READY`: Pico signals fresh data
- `RESET`: Teensy can reset module hardware
- `ID / ADDR`: static module identity hint

For Pico modules, power the Pico from `VSYS` when using the connector 5V rail.
Keep I2C pullups on the master side where possible, and do not pull `SDA` or
`SCL` up to 5V.

## Bring-Up Order

1. Build all firmware targets.
2. Upload `teensy_master`.
3. Upload `pico_loadcell`.
4. Wire one Pico to the Teensy over I2C.
5. Confirm the Teensy prints valid packets.
6. Calibrate the HX711 load-cell readings.
7. Repeat for pressure and encoder modules.

Do not bring up every module at once. The first milestone is one Pico returning
valid packets to the Teensy.
