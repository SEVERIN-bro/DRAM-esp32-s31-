# Silicon controller architecture for OpenDRAM

This document is a conceptual starting point for a silicon implementation of the OpenDRAM memory controller. It is intentionally implementation-neutral. It is not a foundry-specific layout or a production-ready PDK design.

## Goals

- Support a synchronous memory interface with a shared 16-bit DQ bus.
- Multiplex row and column addresses over the 14-bit address bus.
- Provide a clean RTL state machine for read/write transactions.
- Preserve the possibility of later adding banked organization, refresh, and precharge.

## Interface

```text
ADDR[13:0]     row/column address
DQ[15:0]       shared bidirectional data bus
CLK            master clock
CS             chip select
RAS            row address strobe
CAS            column address strobe
WE             write enable
VDD1/VDD2      power
VSS            ground
```

## Addressing model

For the 256 MiB logical target:

```text
row bits     = 14
column bits  = 13
word bits    = 16

address = (row << 13) | column
```

This yields:

```text
2^14 rows × 2^13 columns × 16 bits = 256 MiB
```

## Block-level structure

```text
Host interface
    │
    ▼
Command decoder / timing FSM
    │
    ├── row address latch
    ├── column address latch
    ├── data I/O buffer
    ├── refresh controller
    ├── precharge controller
    └── bank arbitration
            │
            ▼
      1T1C DRAM array
            │
            ▼
   sense amplifiers / write drivers
            │
            ▼
         DQ[15:0]
```

## Suggested command set

```text
CS RAS CAS WE  Command
0  0  0  X    NOP
0  1  0  0    ACTIVATE row
0  0  1  0    READ column
0  0  1  1    WRITE column
0  1  1  0    REFRESH
0  1  1  1    PRECHARGE
1  X  X  X    DESELECT
```

## Minimal controller state machine

```text
IDLE --> ACTIVATE --> COMMAND --> READ_WAIT/WRITE --> PRECHARGE --> IDLE
         \___________________ refresh path ___________________________/
```

The `dram_controller.sv` file in this repository is the starting RTL skeleton. It exposes the key controller phases without claiming foundry-specific timing or silicon implementation details.

## Important notes

- This is not a finished production silicon design.
- It does not yet include bank interleaving, write recovery, or refresh scheduling.
- The exact PDK and bitcell library must be chosen before a real silicon implementation can be closed.
- For a true DRAM design, the next step is to add a banked organization and a realistic cell-level timing model.

## Recommended next steps

1. Choose the process node and PDK.
2. Decide on bank count (4, 8, or more).
3. Add refresh controller and precharge timing.
4. Add write recovery, sense amplifier timing, and I/O timing checks.
5. Add DRC/LVS checks and sign-off.
6. Add BIST and memory test mode.

This repository is intentionally left as a conceptual and RTL-oriented starting point for future silicon implementation work.
