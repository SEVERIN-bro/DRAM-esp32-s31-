# OpenDRAM for ESP32-S31

A practical reference design for connecting an external DRAM memory device to an ESP32-S31 using a parallel bus, a dedicated configuration interface, and a lightweight controller implemented in SystemVerilog.

## Why this project exists

Embedded systems often run into memory limits much sooner than expected. On-chip RAM is limited, and custom boards frequently need a larger external memory buffer for:

- image processing,
- networking buffers,
- audio/video pipelines,
- data logging,
- intermediate processing stages,
- experimentation with FPGA-like memory interfaces.

This repository explores a realistic architecture for attaching external DRAM to an ESP32-S31 while keeping the design understandable and easy to experiment with.

## What this project does

This project provides a working blueprint for:

- an external DRAM memory bus,
- GPIO-based address and data wiring,
- row/column address decoding,
- a memory controller abstraction layer,
- basic read/write access from ESP-IDF firmware.

It is intended as a learning and prototyping project for memory interfacing on ESP32-class hardware.

## Who this is for

This project is useful for:

- embedded systems developers,
- hardware engineers exploring DRAM interfaces,
- students learning memory controller design,
- researchers prototyping custom memory access patterns,
- makers building ESP32 boards with external memory expansion.

## Problem it solves

Many embedded projects need more memory than the MCU provides, but external DRAM is hard to approach because of:

- complex addressing,
- timing-sensitive control signals,
- bus layout concerns,
- difficult integration with firmware.

This repository makes the architecture concrete: it shows one practical path for connecting and accessing DRAM using an ESP32-S31.

## High-level architecture

The repository follows a realistic memory organization model:

- the memory device contains an internal DRAM controller,
- the host accesses the DRAM through a high-speed parallel bus,
- a separate configuration bus is used for setup, reset, and status,
- row/column multiplexing reconstructs a logical address.

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

## Repository structure

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

## What is implemented

- `app_main.c` — example firmware entry point for testing memory access
- `opendram_bus.h` / `opendram_bus.c` — C API for DRAM bus read/write operations
- `dram_controller.sv` — digital logic for DRAM control
- `opendram_memory_controller.sv` — higher-level memory controller logic

## Example usage

The example firmware writes a 16-bit value to memory and reads it back:

```c
const uint32_t address = opendram_make_address(0, 0);
ESP_ERROR_CHECK(opendram_bus_write16(address, 0xABCD));

uint16_t value = 0;
ESP_ERROR_CHECK(opendram_bus_read16(address, &value));
```

This demonstrates the basic flow:

1. configure GPIOs,
2. initialize the DRAM bus,
3. perform read/write operations,
4. validate the memory transaction.

## Why this is useful

This project is valuable because it turns an otherwise abstract hardware topic into a concrete implementation:

- practical bus wiring,
- address mapping,
- memory access abstraction,
- firmware-level validation,
- a useful base for future experiments.

## Important note

This repository is a reference architecture and prototype. For real full-speed DRAM operation, a production-grade memory controller still requires proper refresh timing, precharge timing, row activation sequencing, and DRAM timing compliance.

This project is best understood as a practical design blueprint and learning platform rather than a final product-ready memory controller.

## Getting started

1. Open the project in your ESP-IDF environment.
2. Configure the GPIO mappings in `app_main.c`.
3. Build and flash the firmware.
4. Validate read/write operations on the external memory bus.

## Roadmap ideas

- add a proper timing state machine,
- improve refresh and precharge logic,
- support additional DRAM sizes,
- document board wiring and GPIO mapping,
- add a block diagram and timing illustrations,
- expand the testbench and validation examples.

## Contributing

Contributions, bug fixes, hardware validation, and improved documentation are welcome.

## License

MIT

---

If you want to make this repository more discoverable on GitHub, the best next steps are:

- add a strong repository description,
- add topics such as `esp32`, `dram`, `embedded`, `memory-controller`, `systemverilog`, `hardware`, `esp-idf`, `rtl`,
- add a diagram or screenshot,
- publish a short project announcement explaining the problem it solves.
