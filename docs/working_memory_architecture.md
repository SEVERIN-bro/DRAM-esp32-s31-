# Working implementation plan

## Architecture

```text
ESP32-S31
  │
  ├── Main memory bus
  │     ADDR[13:0], DQ[15:0], CS, RAS, CAS, WE, CLK
  │
  └── Configuration bus
        CFG_SCLK, CFG_MOSI, CFG_MISO, CFG_CS_N
                              │
                              ▼
              ┌────────────────────────┐
              │ Embedded DRAM controller│
              │ command decode          │
              │ row/column latches      │
              │ timing FSM              │
              │ refresh/precharge       │
              │ config registers        │
              └───────────┬────────────┘
                          ▼
                    1T1C DRAM macro
```

## Files

- `opendram_memory_controller.sv`: top-level controller prototype with the main bus and configuration port.
- `dram_controller.sv`: reusable lower-level row/column timing FSM.
- `opendram_bus.c/.h`: ESP32 low-speed GPIO prototype; not suitable for 133 MHz operation.
- `app_main.c`: example application; GPIO values must be replaced for a real board.

## Main bus transaction

Row phase:

```text
CS=0, RAS=1, CAS=0, WE=0, ADDR=ROW[13:0], CLK rising edge
```

Column read:

```text
CS=0, RAS=0, CAS=1, WE=0, ADDR[12:0]=COL[12:0], DQ=high-Z
```

Column write:

```text
CS=0, RAS=0, CAS=1, WE=1, ADDR[12:0]=COL[12:0], DQ=DATA[15:0]
```

The logical address is:

```text
address = (row << 13) | column
```

## Configuration port

The configuration port is separate from the data bus and is used for timing, reset, status, and test controls. The prototype command format is SPI-like and must be replaced by a CDC-safe committed-register protocol in a production ASIC.

Suggested registers:

```text
0x00 CHIP_ID
0x01 STATUS
0x10 T_RCD
0x11 T_CAS
0x12 T_RP
0x13 T_RAS
0x14 T_WR
0x20 REFRESH_ENABLE
0x21 REFRESH_INTERVAL
0x30 BIST_CONTROL
0x31 BIST_STATUS
```

## Required production work

The current RTL is a simulation/architecture prototype. Before silicon sign-off, add:

1. A real refresh scheduler and row counter.
2. Explicit precharge, tRCD, tRAS, tRP, tWR, and read-latency counters.
3. CDC synchronizers and shadow/commit registers for `cfg_sclk` to `clk`.
4. Bank organization and an active-row register per bank.
5. A DRAM macro wrapper instead of the simulation memory array.
6. DQ turnaround and output-enable timing checks.
7. Reset, power-on, test mode, BIST, ECC/redundancy as required.
8. RTL simulation, lint, CDC analysis, synthesis, STA, DRC, and LVS.

The module is therefore a working architectural starting point, not a manufacturing-ready DRAM macro.
