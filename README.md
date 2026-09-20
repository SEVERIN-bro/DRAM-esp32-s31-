# OpenDRAM for ESP32-S31

A practical prototype for interfacing an external DRAM memory device with the ESP32-S31 using a parallel memory bus, address decoding, and a custom controller implemented in SystemVerilog.

## Why this project exists

Embedded systems often run out of memory long before the application logic is complete. On-chip RAM is limited, and many custom boards need more storage for:

- framebuffer and image buffers,
- network packet processing,
- data logging,
- signal buffering,
- real-time processing pipelines,
- hardware experimentation and memory expansion.

This project explores a realistic way to connect external DRAM to an ESP32-S31 and access it from firmware in a controlled and understandable way.

## What this project does

This repository provides a concrete design for:

- a high-speed external DRAM bus,
- GPIO-based address and data routing,
- row/column multiplexing,
- memory address translation,
- read/write access from ESP-IDF code,
- a hardware controller model for experimentation.

It is intended as a practical blueprint for external memory interfacing on ESP32-class hardware.

## Who this is for

This project is useful for:

- embedded systems developers,
- hardware engineers working with DRAM interfaces,
- students learning memory controller design,
- researchers prototyping custom memory systems,
- makers building ESP32-based boards with external memory expansion.

## The problem it solves

External DRAM is powerful, but difficult to integrate because of:

- address decoding complexity,
- timing-sensitive control signals,
- bus wiring constraints,
- poor visibility into how memory is accessed,
- limited documentation for embedded experiments.

This repository turns that complexity into a concrete, inspectable implementation.

## Architecture overview

The design follows a realistic DRAM organization:

- the memory device includes an internal DRAM controller,
- the host communicates through a parallel memory bus,
- a separate configuration interface is used for setup and status,
- logical addresses are reconstructed using row/column multiplexing.

### Main memory bus

```text
ADDR[13:0]
DQ[15:0]
RAS
CAS
WE
CS
CLK
```

### Configuration bus

```text
CFG_SCLK
CFG_MOSI
CFG_MISO
CFG_CS_N
```

### Address mapping

```text
row = address >> 13
col = address & 0x1fff
word_address = (row << 13) | col
```

This yields a logical memory target of 256 MiB.

## Repository layout

```text
.
├── README.md
├── app_main.c
├── dram_controller.sv
├── opendram_bus.c
├── opendram_bus.h
├── opendram_memory_controller.sv
├── docs/
├── LICENSE
└── .gitignore
```

## Core components

- `app_main.c` — example firmware entry point
- `opendram_bus.h` / `opendram_bus.c` — C abstraction for bus access
- `dram_controller.sv` — RTL logic for DRAM control
- `opendram_memory_controller.sv` — higher-level memory controller model

## Example usage

The sample firmware writes a 16-bit value to memory and reads it back:

```c
const uint32_t address = opendram_make_address(0, 0);
ESP_ERROR_CHECK(opendram_bus_write16(address, 0xABCD));

uint16_t value = 0;
ESP_ERROR_CHECK(opendram_bus_read16(address, &value));
```

This demonstrates the basic flow:

1. configure the GPIO mappings,
2. initialize the bus,
3. perform read/write transactions,
4. validate the results.

## Why this is useful

This project is valuable because it makes an otherwise difficult hardware topic concrete:

- practical DRAM bus wiring,
- address decoding and mapping,
- firmware-level access patterns,
- reusable controller structure,
- a foundation for custom embedded memory designs.

## Important note

This repository is a reference architecture and prototype. For real full-speed DRAM operation, a production-grade controller still requires proper refresh timing, precharge timing, and row activation sequencing.

This project should be seen as a practical learning and prototyping platform rather than a final production-ready memory controller.

## Getting started

1. Open the project in an ESP-IDF environment.
2. Configure the GPIO mappings in `app_main.c`.
3. Build and flash the firmware.
4. Validate read/write behavior against the external memory bus.

## Roadmap ideas

- add proper timing state machines,
- improve refresh and precharge handling,
- support more DRAM variants,
- document board wiring and GPIO mapping,
- add timing diagrams and architecture visuals,
- expand validation examples and tests.

## Contributing

Contributions, bug reports, documentation improvements, and hardware validation are welcome.

## License

MIT

---

## Summary

This project is a practical blueprint for experimenting with external DRAM on the ESP32-S31. It aims to make the difficult topic of memory interface design more accessible, testable, and reusable for embedded engineers, makers, and students.
