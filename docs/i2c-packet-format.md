# I2C Packet Format

Version: v0.1

This document describes the packet currently implemented in
`lib/HapticProtocol/src/HapticProtocol.h`.

## Bus Model

- Teensy 4.0 is the I2C master.
- Pico modules are I2C targets.
- The master reads a fixed-size packet from each known module address.
- Pico modules keep the latest processed sensor state ready for the master.
- The optional `IRQ / DATA_READY` line can tell the master that a module has
  fresh data, but polling still works.

## Current Addresses

- `0x20`: load-cell module
- `0x21`: pressure module
- `0x22`: encoder module

## Packet Size

Each module returns exactly 19 bytes.

```cpp
struct ModulePacket {
  uint8_t protocolVersion;
  uint8_t moduleKind;
  uint8_t status;
  uint8_t sequence;
  uint16_t idAdc;
  int16_t payload[6];
  uint8_t checksum;
} __attribute__((packed));
```

## Field Layout

Byte offsets:

```text
0       protocolVersion   uint8_t
1       moduleKind        uint8_t
2       status            uint8_t
3       sequence          uint8_t
4..5    idAdc             uint16_t
6..17   payload[6]        six int16_t values
18      checksum          uint8_t
```

All multi-byte values are sent in the microcontroller's native little-endian
layout. The current system is Teensy 4.0 plus Raspberry Pi Pico, both little
endian.

## Protocol Version

Current value:

```text
1
```

The Teensy should reject packets with an unknown `protocolVersion`.

## Module Kinds

```text
0  unknown
1  loadcell
2  pressure
3  encoder
```

The module kind describes the firmware role of the target module.

## Status Values

```text
0  OK
1  BOOTING
2  SENSOR_FAULT
3  CALIBRATING
```

The Teensy should treat any nonzero status as useful diagnostic information.
Some nonzero states, such as `CALIBRATING`, are not necessarily fatal.

## Sequence

`sequence` increments every time the Pico publishes a new packet.

The Teensy can use this to detect:

- fresh data
- repeated/stale data
- module stalls
- missed updates

The value wraps naturally after `255`.

## ID ADC

`idAdc` stores the Pico ADC reading from the connector `ID / ADDR` pin.

This is a static identity hint, not a live sensor value. It can be used to
differentiate physical module boards through a resistor ladder or simple strap.
The Pico should read this at boot/reset and may also include the latest reading
in every packet for debug visibility.

## Payload

The packet contains six signed 16-bit payload values:

```text
payload[0]
payload[1]
payload[2]
payload[3]
payload[4]
payload[5]
```

Meanings are module-specific.

Suggested load-cell payload:

```text
payload[0]  load cell A filtered value
payload[1]  load cell B filtered value
payload[2]  load cell A raw or diagnostic value
payload[3]  load cell B raw or diagnostic value
payload[4]  reserved
payload[5]  reserved
```

Suggested pressure payload:

```text
payload[0]  filtered pressure value
payload[1]  raw ADC value
payload[2]  baseline / zero value
payload[3]  reserved
payload[4]  reserved
payload[5]  reserved
```

Suggested encoder payload:

```text
payload[0]  encoder position low word
payload[1]  encoder position high word
payload[2]  velocity estimate
payload[3]  button or index state
payload[4]  reserved
payload[5]  reserved
```

## Checksum

The checksum is a one-byte XOR over the full 19-byte packet with `checksum`
temporarily set to zero.

Packet creation:

```text
checksum = 0
checksum = xor(all 19 packet bytes)
```

Packet validation:

```text
saved = checksum
checksum = 0
valid if saved == xor(all 19 packet bytes)
```

This is not cryptographic. It is only a small wiring/protocol sanity check.

## Read Transaction

The Teensy reads one full packet from a module address:

```text
request 19 bytes from module address
if 19 bytes received:
  check protocolVersion
  check checksum
  read moduleKind, status, sequence, idAdc, payload
else:
  mark module read failure
```

## Future Extensions

Possible future protocol additions:

- master-to-module command writes
- explicit calibration command
- module discovery scan
- module serial number or firmware version
- larger packet for high-resolution data
- CRC-8 instead of XOR checksum

Keep v1.0 small until the first module-to-master path is stable.

