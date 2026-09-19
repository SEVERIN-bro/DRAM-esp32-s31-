# OpenDRAM-256MiB-110nm

Open 256 MiB synchronous DRAM concept for high-performance microcontrollers.

> This document defines the interface and a behavioral RTL model.  
> It is not a complete physical DRAM implementation, memory-macro design,
> transistor-level circuit, layout, or manufacturing-qualified technology
> specification.

## License

MIT License

Copyright (c) 2026 OpenDRAM Contributor

The complete license text is provided in the standard `LICENSE` file.

## Specifications

- **Capacity:** 256 MiB
- **Organization:** `16,384 rows × 8,192 columns × 16 bits`
- **Total words:** 134,217,728 × 16-bit words
- **Total capacity:** 2,147,483,648 bits = 256 MiB
- **Process target:** 110 nm CMOS mixed-signal
- **Memory cell:** 1T1C dynamic memory cell
- **Interface:** 51-pin synchronous parallel interface
- **Data buses:** Separate 16-bit read and write buses
- **Clock frequency:** Up to 133 MHz
- **Clock period:** 7.5 ns
- **Theoretical bandwidth:** Up to 266 MB/s with a 16-bit bus at 133 MHz
- **Nominal logic voltage:** 3.3 V LVCMOS
- **Addressing:** Multiplexed row/column address
- **Minimum read latency:** Two clock cycles:
  1. `ACTIVATE` captures the row address;
  2. `READ` captures the column address and returns the data.

### Memory Organization

```text
14 row address bits    = 16,384 rows
13 column address bits = 8,192 columns
16 data bits per word

2^14 × 2^13 × 16 bits = 2^31 bits = 256 MiB
```

## Pin Assignment

| Pins | Signal | Direction | Description |
|---:|---|---|---|
| 1–14 | `ADDR[13:0]` | Input | Multiplexed row/column address bus |
| 15 | `RAS` | Input | Part of the command code |
| 16 | `CAS` | Input | Part of the command code |
| 17–32 | `RX_DATA[15:0]` | Output | Data from DRAM to MCU |
| 33–48 | `TX_DATA[15:0]` | Input | Data from MCU to DRAM |
| 49 | `VDD1` | Power | +3.3 V digital supply |
| 50 | `VDD2` | Power | +3.3 V digital supply |
| 51 | `CLK` | Input | Synchronous clock input |
| Flanges | `VSS` | Power | Common ground |

> This 51-pin version does not provide separate `CS`, `WE`, `OE`, `RESET`,
> or `DQM` pins. The operation type is encoded using `RAS` and `CAS`.

## Command Encoding

| `RAS` | `CAS` | Command | Description |
|---:|---:|---|---|
| 0 | 0 | `NOP` | No operation |
| 1 | 0 | `ACTIVATE` | Capture the row address |
| 0 | 1 | `READ` | Read from the selected column |
| 1 | 1 | `WRITE` | Write `TX_DATA` to the selected column |

### Read Operation

```text
Cycle N:     ACTIVATE, ADDR = row_address
Cycle N + 1: READ,     ADDR = column_address
Cycle N + 1: RX_DATA becomes valid after the rising edge of CLK
```

### Write Operation

```text
Cycle N:     ACTIVATE, ADDR = row_address
Cycle N + 1: WRITE,    ADDR = column_address, TX_DATA = write_data
```

Before activating a different row, the memory controller must complete the
current operation and close the previously active row.

## Refresh

Because the array uses 1T1C DRAM cells, periodic refresh is required.

- **Target retention time:** At least 64 ms under nominal conditions
- **Refresh rows:** 8,192 rows every 64 ms
- **Average refresh interval:** Approximately 7.8 µs
- **Refresh control:** Performed by an internal refresh controller
- External commands may be temporarily stalled during internal refresh.

A production implementation must account for temperature, leakage current,
process variation, and worst-case data retention.

## Behavioral SystemVerilog Model

The following model implements the stated memory geometry and separates read
and write operations. A complete 256 MiB array may require significant memory
during simulation. An ASIC memory macro, FPGA block RAM, or sparse behavioral
memory model should be used for practical simulation.

```systemverilog
module sync_dram_256mib_110nm (
    input  logic        clk,
    input  logic        ras,
    input  logic        cas,
    input  logic [13:0] addr,
    input  logic [15:0] tx_data,
    output logic [15:0] rx_data
);

    localparam int ROW_BITS  = 14;
    localparam int COL_BITS  = 13;
    localparam int DATA_BITS = 16;
    localparam int DEPTH     = 1 << (ROW_BITS + COL_BITS);

    typedef enum logic [1:0] {
        CMD_NOP      = 2'b00,
        CMD_ACTIVATE = 2'b10,
        CMD_READ     = 2'b01,
        CMD_WRITE    = 2'b11
    } command_t;

    command_t command;

    logic [13:0] row_latch;
    logic [15:0] dram_matrix [0:DEPTH-1];

    logic [26:0] word_address;

    assign command = command_t'({ras, cas});

    // 14-bit row address + 13-bit column address = 27-bit word address.
    assign word_address = {row_latch, addr[12:0]};

    always_ff @(posedge clk) begin
        case (command)

            CMD_NOP: begin
                // No operation.
            end

            CMD_ACTIVATE: begin
                row_latch <= addr;
            end

            CMD_READ: begin
                // Registered read output.
                rx_data <= dram_matrix[word_address];
            end

            CMD_WRITE: begin
                dram_matrix[word_address] <= tx_data;
            end

            default: begin
                // Defensive default.
            end

        endcase
    end

endmodule
```

The modeled memory contains:

```text
134,217,728 words × 16 bits = 256 MiB
```

For large simulations, a sparse memory implementation is recommended because
allocating the complete array may be slow or memory-intensive.

## Internal Block Structure

```text
ADDR[13:0]
    │
    ▼
[Input Buffers]
    │
    ├── ACTIVATE ──► [Row Address Latch]
    │                       │
    │                       ▼
    │                [Row Decoder 1:16,384]
    │
    └── READ/WRITE ─► [Column Address Latch]
                            │
                            ▼
                     [Column Decoder 1:8,192]
                            │
                            ▼
                     [1T1C DRAM Array]
                            │
              ┌─────────────┴─────────────┐
              ▼                           ▼
      [Sense Amplifiers]           [Write Drivers]
              │                           │
              ▼                           ▼
       RX_DATA[15:0]                TX_DATA[15:0]
```

## 1T1C Memory Cell

```text
Bit Line ───────────────┐
                        │
                    [Access NMOS]
                        │
Word Line ───────► Gate │
                        │
                        ├──── Storage Node
                        │
                   [Storage Capacitor]
                        │
                  Plate / Reference
```

A real implementation must also include:

- precharge circuitry;
- sense amplifiers;
- restore operation after destructive reads;
- leakage and retention characterization;
- redundant rows and columns;
- defect analysis and repair fuses;
- temperature-compensated refresh control.

## PCB Layout Recommendations

1. Use at least a four-layer PCB stackup:

   ```text
   Signal / GND / Power / Signal
   ```

2. Provide a continuous ground reference plane beneath high-speed signals.

3. Use controlled single-ended impedance, approximately 50 Ω ±10%, if
   supported by the selected PCB stackup.

4. Match trace lengths within the following targets:

   - `RX_DATA[15:0]`: ±0.5 mm;
   - `TX_DATA[15:0]`: ±0.5 mm;
   - `ADDR[13:0]`: ±1.0 mm;
   - `CLK`: minimize length and layer transitions.

5. Maintain at least 3W spacing between `CLK` and aggressive neighboring signals
   where practical. Keep a continuous return-current path for the clock.

6. Place a 0.1 µF ceramic decoupling capacitor near each `VDD` pin.
   A distance of less than 2 mm is recommended where practical.

7. Add bulk decoupling near the module power entry point.

8. Do not route high-speed signals across gaps or splits in the ground plane.

9. Verify the following before manufacturing:

   - rise and fall times;
   - input thresholds;
   - output drive strength;
   - capacitive loading;
   - setup and hold timing;
   - simultaneous-switching output noise;
   - power integrity;
   - signal integrity.

## Initial Timing Targets

The following values are initial design targets and are not guaranteed
parameters for a fabricated device:

| Parameter | Target |
|---|---:|
| `tCK` | 7.5 ns |
| `tRCD` | 1 clock |
| `tCL` | 1 clock |
| Row-to-column latency | 2 clocks total |
| `tRAS` | To be defined by the memory controller |
| `tRP` | To be defined by the memory controller |
| Average refresh interval | ≤ 7.8 µs |
| Logic voltage | 3.3 V nominal |

Final timing parameters must be validated using SPICE simulations, post-layout
extraction, process-voltage-temperature corner analysis, and silicon
characterization.

## Limitations

This document describes a simplified open memory interface and behavioral model.
A production device would additionally require:

- transistor-level DRAM cell design;
- sense amplifier design;
- high-voltage word-line circuitry;
- refresh controller;
- DRC/LVS-clean layout;
- timing and power sign-off;
- characterized I/O libraries;
- ESD and latch-up protection;
- test structures;
- wafer-level validation;
- package-level validation.
