# SCPI Specification

This document defines the minimum SCPI command subset for OpenAWG
and their behaviour. The goal is consistent behaviour across
transports (UART/TCP/BLE/USB/WebSocket) and output backends
(internal DAC, External DAC, DDS Chips, etc)

## Conventions

### Command Formatting

- Commands are case-insensitive
- Commands are ASCII text terminated by `\n` (accepts `\r\n`)
- Queries end with `?` and return and ASCII response terminated by `\n`.

### Numeric Parsing

The implementaion accepts

- Plain decimal floats/ints `1000`, `1.5`
- Scientific notation `1e3`, `2.5E-3`
- SI Suffixes
  - Frequency: `k`, `M`, `G` (e.g., `1k`, `2.5M`)
  - Voltage: `m` (e.g `200m`, `200mV`)

## Commands

### IEEE 488.2 Common Commands

#### IDN

Command: `*IDN?`

The identification (IDN) query outputs an indentifying string, in the format
`<company name>, <model number>, <serial number>, <firmware revision>`
