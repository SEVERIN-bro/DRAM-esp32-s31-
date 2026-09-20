module dram_controller #(
    parameter int ROW_BITS = 14,
    parameter int COL_BITS = 13,
    parameter int DATA_BITS = 16
) (
    input logic clk,
    input logic reset_n,
    input logic req_valid,
    input logic req_write,
    input logic [ROW_BITS-1:0] req_row,
    input logic [COL_BITS-1:0] req_col,
    input logic [DATA_BITS-1:0] req_wdata,
    output logic req_ready,
    output logic resp_valid,
    output logic [DATA_BITS-1:0] resp_rdata,
    output logic [ROW_BITS-1:0] dram_addr,
    output logic dram_ras,
    output logic dram_cas,
    output logic dram_we,
    output logic [DATA_BITS-1:0] dram_dq_out,
    output logic dram_dq_oe,
    input logic [DATA_BITS-1:0] dram_dq_in
);
    typedef enum logic [2:0] { IDLE, ACTIVATE, COMMAND, READ_WAIT, PRECHARGE } state_t;
    state_t state;
    logic [ROW_BITS-1:0] row_q;
    logic [COL_BITS-1:0] col_q;
    logic [DATA_BITS-1:0] data_q;
    logic write_q;

    always_comb begin
        req_ready = (state == IDLE);
        resp_valid = (state == READ_WAIT);
        dram_addr = '0;
        dram_ras = 1'b0;
        dram_cas = 1'b0;
        dram_we = 1'b0;
        dram_dq_out = data_q;
        dram_dq_oe = 1'b0;
        case (state)
            ACTIVATE: begin dram_addr = row_q; dram_ras = 1'b1; end
            COMMAND: begin
                dram_addr = col_q;
                dram_cas = 1'b1;
                dram_we = write_q;
                dram_dq_oe = write_q;
            end
            default: begin end
        endcase
    end

    always_ff @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            state <= IDLE;
            row_q <= '0;
            col_q <= '0;
            data_q <= '0;
            write_q <= 1'b0;
            resp_rdata <= '0;
        end else begin
            case (state)
                IDLE: if (req_valid) begin
                    row_q <= req_row;
                    col_q <= req_col;
                    data_q <= req_wdata;
                    write_q <= req_write;
                    state <= ACTIVATE;
                end
                ACTIVATE: state <= COMMAND;
                COMMAND: state <= write_q ? PRECHARGE : READ_WAIT;
                READ_WAIT: begin resp_rdata <= dram_dq_in; state <= PRECHARGE; end
                PRECHARGE: state <= IDLE;
                default: state <= IDLE;
            endcase
        end
    end
endmodule
