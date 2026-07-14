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

## MIDI Interaction Mapping

The control surface has two independent MIDI interaction lanes. Continuous
sensor data is sent on MIDI channel 1. The numpad and panel buttons are sent
as press/release events on MIDI channel 16, so they cannot be mistaken for
continuous controls.

### Continuous sensor controls

The current USB MIDI 1.0-compatible firmware emits 7-bit Control Change
messages on channel 1. The CC numbers below are retained for compatibility
with the existing UI app; they are a Haptic Console mapping, not a claim about
the standard meanings of those controller numbers.

| CC | Control | Source | MIDI 1.0 range |
|---:|---|---|---|
| 1 | `flywheel_velocity` | Encoder velocity magnitude | 0–60.0 RPM mapped to 0–127 |
| 2 | `flywheel_direction` | Encoder direction | reverse/stopped/forward = 0/64/127 |
| 3 | `pneumatic_pressure` | Pressure sensor | 0–10.00 kPa mapped to 0–127 |
| 4 | `spring_tension` | Load cell A | Signed `int16_t` mapped to 0–127 |
| 5 | `spring_acoustic` | Load cell B | Signed `int16_t` mapped to 0–127 |
| 6 | `lean_total` | Reserved | — |
| 7 | `lean_balance` | Reserved | — |
| 8 | `matrix_centroid_x` | Reserved | — |
| 9 | `matrix_centroid_y` | Reserved | — |
| 10 | `matrix_pressure` | Reserved | — |
| 11 | `joystick_1_x` | Reserved | — |
| 12 | `joystick_1_y` | Reserved | — |
| 13 | `joystick_2_x` | Reserved | — |
| 14 | `joystick_2_y` | Reserved | — |

### Numpad and panel buttons

The control unit's ten numpad keys, eight action buttons, and five control
buttons use Note On and Note Off messages on channel 16. A press sends Note On
with velocity 127; release sends Note Off. This preserves momentary-button
behavior and provides a distinct event for every press.

| Controls | MIDI channel | Note range |
|---|---:|---|
| Numpad `0`–`9` | 16 | 36–45, in numeric order |
| Action buttons `1`–`8` | 16 | 46–53 |
| Control buttons `1`–`5` | 16 | 54–58 |

Control buttons may also have local functions such as mode, shift, back,
confirm, or menu. Their local behavior remains deterministic; the matching
MIDI note is an optional mirror for the UI app or host.

### MIDI 2.0 direction

Native MIDI 2.0 support will preserve this layout on UMP Group 1: channel 1
will use 32-bit MIDI 2.0 Channel Controller messages for the continuous
controls, and channel 16 will use MIDI 2.0 Note On/Off messages for panel
events. The current `USB_MIDI_SERIAL` firmware is a USB MIDI 1.0-compatible
transport, so it emits the mappings above in MIDI 1.0 form until a USB MIDI
2.0 UMP transport and endpoint discovery are implemented.

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
