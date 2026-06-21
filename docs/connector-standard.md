# Haptic Console Connector Standard

Version: v0.1

This document defines the internal module connector for Haptic Console v1.0.
The connector is intended for short internal chassis runs between the Teensy
4.0 master controller and Raspberry Pi Pico sensor modules.

## Purpose

The connector carries power, I2C communication, module readiness, reset, and
static module identity. It is designed for sensor preprocessing modules rather
than high-power actuator modules.

## Electrical Role

- Teensy 4.0 is the I2C master.
- Raspberry Pi Pico modules are I2C targets.
- Pico modules preprocess local sensor data and expose ready-to-read packets.
- I2C communication is 3.3V only.
- 5V is power only and must never be used as a logic level.

## Pinout

Recommended 8-pin pinout:

```text
1  GND
2  5V
3  3V3
4  SDA
5  SCL
6  IRQ / DATA_READY
7  RESET
8  ID / ADDR
```

## Pin Descriptions

`GND`

Shared ground for power and signal reference. Every module must connect this.

`5V`

Main module power rail. Use this for Pico `VSYS`, 5V sensors, local regulators,
LEDs, or other non-logic power needs. This rail must not directly drive GPIO.

`3V3`

Logic reference or light accessory rail. Do not back-power a Pico through both
`VSYS` and `3V3`. If the Pico is powered from `5V -> VSYS`, normally leave the
connector `3V3` disconnected or use it only as a reference where appropriate.

`SDA`

I2C data line. This is open-drain/open-collector style and must be pulled up to
3.3V, not 5V.

`SCL`

I2C clock line. This is driven by the Teensy master and must be pulled up to
3.3V, not 5V.

`IRQ / DATA_READY`

Module-to-master signal. A Pico can assert this pin when it has fresh data,
a fault, or a state change. The master may still poll modules periodically.

`RESET`

Master-to-module reset line. For Pico modules, this can connect to the Pico
`RUN` pin so the Teensy can recover a stuck module.

`ID / ADDR`

Static module identity input. This is not a data bus. The Pico reads it at boot
or reset to identify the physical module or choose a profile/address behavior.
The preferred implementation is an analog resistor ladder into `GP26 / ADC0`.

## Pico Wiring Reference

Recommended Pico module wiring:

```text
Connector GND    -> Pico GND
Connector 5V     -> Pico VSYS
Connector 3V3    -> leave unused unless needed as a reference
Connector SDA    -> Pico I2C SDA pin
Connector SCL    -> Pico I2C SCL pin
Connector IRQ    -> Pico GP6
Connector RESET  -> Pico RUN
Connector ID     -> Pico GP26 / ADC0
```

If the selected RP2040 Arduino framework does not default to the desired I2C
pins, set them explicitly in firmware before calling `Wire.begin(...)`.

## I2C Rules

- Pull `SDA` and `SCL` up to 3.3V only.
- Prefer one set of pullups on the master side.
- If modules include pullups, make them removable or leave them unpopulated.
- Keep internal cable runs short.
- Do not hot-plug modules unless the hardware is explicitly designed for it.
- Use one unique I2C address per module.

## Current Module Addresses

- `0x20`: load-cell module
- `0x21`: pressure module
- `0x22`: encoder module

## Sensor Notes

Load-cell module:

- Uses two load cells with local amplification, likely HX711.
- HX711 logic should be compatible with Pico GPIO levels.

Pressure module:

- MPXV7002DP is a 5V pressure sensor.
- Its analog output must be scaled or conditioned before entering a Pico ADC.
- Pico ADC inputs are not 5V tolerant.

Encoder module:

- Confirm whether encoder outputs are open collector, open drain, or active
  logic before wiring.
- Pull encoder signals to 3.3V when feeding Pico GPIO.

## Design Rationale

The v1.0 connector favors simple, serviceable sensor modules. It keeps the bus
small enough for fast bring-up while leaving enough pins for power, reset,
readiness, and identity. More rugged buses such as CAN or RS-485 remain good
future options, but they add hardware and firmware complexity that is not needed
for the first working instrument.

