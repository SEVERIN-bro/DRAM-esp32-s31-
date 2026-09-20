## Working memory architecture

This repository now follows the practical scheme below:

- the memory module includes an internal DRAM controller;
- the host accesses the memory through a high-speed parallel bus;
- a separate 4-wire config interface is used for setup, reset, and status;
- row/column multiplexing is used to reconstruct the logical 27-bit address.

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

This yields a 256 MiB logical memory target.

### Practical status

This is the architecture used for the working design blueprint in the repository. It is realistic for an external memory module with an embedded controller, and it is much more practical than a pure GPIO bit-bang design.

For full-speed operation at DRAM levels, a real memory controller with refresh and precharge timing is still required.
