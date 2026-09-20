module opendram_memory_controller #(
    parameter int ROW_BITS = 14,
    parameter int COL_BITS = 13,
    parameter int DATA_BITS = 16,
    parameter int SIM_ADDR_BITS = 12
) (
    // Main synchronous memory interface
    input  logic                 clk,
    input  logic                 reset_n,
    input  logic [ROW_BITS-1:0]  addr,
    input  logic                 cs_n,
    input  logic                 ras,
    input  logic                 cas,
    input  logic                 we,
    inout  logic [DATA_BITS-1:0] dq,

    // Low-speed configuration interface
    input  logic                 cfg_sclk,
    input  logic                 cfg_cs_n,
    input  logic                 cfg_mosi,
    output logic                 cfg_miso
);

    localparam int SIM_DEPTH = 1 << SIM_ADDR_BITS;
    localparam logic [7:0] CMD_READ_REG  = 8'h01;
    localparam logic [7:0] CMD_WRITE_REG = 8'h02;

    typedef enum logic [2:0] {
        ST_RESET,
        ST_IDLE,
        ST_READ_WAIT,
        ST_PRECHARGE,
        ST_REFRESH
    } state_t;

    state_t state;
    logic [ROW_BITS-1:0] row_latch;
    logic [COL_BITS-1:0] col_latch;
    logic [DATA_BITS-1:0] mem [0:SIM_DEPTH-1];
    logic [DATA_BITS-1:0] dq_out;
    logic dq_oe;
    logic [DATA_BITS-1:0] read_data;
    logic [DATA_BITS-1:0] write_data;

    logic [15:0] timing_regs [0:7];
    logic [7:0] cfg_shift;
    logic [7:0] cfg_command;
    logic [5:0] cfg_index;
    logic [15:0] cfg_write_data;
    logic [4:0] cfg_bit_count;
    logic [15:0] cfg_read_shift;
    logic cfg_read_active;

    wire selected = !cs_n;
    wire [SIM_ADDR_BITS-1:0] sim_word_address =
        {row_latch, col_latch}[SIM_ADDR_BITS-1:0];

    assign dq = dq_oe ? dq_out : 'z;
    assign cfg_miso = cfg_read_active ? cfg_read_shift[15] : 1'b0;

    // Main memory command path. The full array is intentionally simulation-sized;
    // a silicon implementation replaces mem with the DRAM macro interface.
    always_ff @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            state      <= ST_RESET;
            row_latch  <= '0;
            col_latch  <= '0;
            dq_out     <= '0;
            dq_oe      <= 1'b0;
            read_data  <= '0;
        end else begin
            dq_oe <= 1'b0;

            case (state)
                ST_RESET: begin
                    state <= ST_IDLE;
                end

                ST_IDLE: begin
                    if (selected && ras && !cas) begin
                        row_latch <= addr;
                    end else if (selected && !ras && cas) begin
                        col_latch <= addr[COL_BITS-1:0];

                        if (we) begin
                            mem[sim_word_address] <= dq;
                        end else begin
                            read_data <= mem[sim_word_address];
                            state <= ST_READ_WAIT;
                        end
                    end else if (selected && ras && cas && !we) begin
                        state <= ST_REFRESH;
                    end
                end

                ST_READ_WAIT: begin
                    dq_out <= read_data;
                    dq_oe  <= selected;
                    state  <= ST_PRECHARGE;
                end

                ST_PRECHARGE: begin
                    dq_oe <= 1'b0;
                    state <= ST_IDLE;
                end

                ST_REFRESH: begin
                    state <= ST_IDLE;
                end

                default: begin
                    state <= ST_RESET;
                end
            endcase
        end
    end

    // Simple configuration-register transport. This is a deliberately small
    // SPI-like prototype; a production ASIC requires CDC synchronizers and a
    // defined register commit/acknowledge protocol.
    always_ff @(posedge cfg_sclk or negedge reset_n) begin
        if (!reset_n) begin
            cfg_shift      <= '0;
            cfg_command    <= '0;
            cfg_index      <= '0;
            cfg_write_data <= '0;
            cfg_bit_count  <= '0;
            cfg_read_shift <= '0;
            cfg_read_active <= 1'b0;
        end else if (cfg_cs_n) begin
            cfg_bit_count   <= '0;
            cfg_read_active <= 1'b0;
        end else begin
            cfg_shift <= {cfg_shift[6:0], cfg_mosi};
            cfg_bit_count <= cfg_bit_count + 1'b1;

            if (cfg_bit_count == 5'd7) begin
                cfg_command <= {cfg_shift[6:0], cfg_mosi};
            end
            if (cfg_bit_count == 5'd15) begin
                cfg_index <= {cfg_shift[4:0], cfg_mosi};
            end
            if (cfg_bit_count == 5'd31) begin
                cfg_write_data <= {cfg_shift[14:0], cfg_mosi};
            end
        end
    end

    // Configuration registers are intentionally kept separate from the main
    // clock domain. Integration must add a CDC-safe shadow/commit mechanism.
    always_ff @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            timing_regs[0] <= 16'd1; // tRCD clocks
            timing_regs[1] <= 16'd1; // tCAS clocks
            timing_regs[2] <= 16'd1; // tRP clocks
            timing_regs[3] <= 16'd1; // tRAS clocks
            timing_regs[4] <= 16'd1; // tWR clocks
            timing_regs[5] <= 16'd1; // refresh enable
            timing_regs[6] <= 16'd1000; // refresh interval placeholder
            timing_regs[7] <= 16'd0;
        end
    end
endmodule
