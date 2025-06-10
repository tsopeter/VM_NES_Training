
module synchronizer_t #(
    // number of bitplanes
    parameter N_BIT_PLANES = 12
)(
    input  wire clk,
    input  wire rst,

    input  wire vsync,
    input  wire serial,
    input  wire bitplane,

    output reg  send = 1'h0

);
    localparam S_WAIT_FOR_SERIAL = 8'h00;
    localparam S_WAIT_FOR_VSYNC  = 8'h01;
    localparam S_BIT_PLANE_COUNT = 8'h02;
    localparam S_SEND_SIGNAL     = 8'h03;
    localparam S_RESTART         = 8'h04;

    reg [7:0] state = S_WAIT_FOR_SERIAL;
    reg [15:0] plane_count = 16'h0000;

    always @(posedge clk) begin
        case (state)
            S_SERIAL_READY: begin
                if (serial) begin
                    state <= S_WAIT_FOR_VSYNC;
                end
            end
            S_WAIT_FOR_VSYNC: begin
                if (vsync) begin
                    plane_count <= 0;   // reset the counter
                    state <= S_BIT_PLANE_COUNT;
                end
            end
            S_BIT_PLANE_COUNT: begin
                if (bitplane) begin     // if a bitplane signal comes in
                    send  <= 1'h1;      // send signal and go to next state
                    state <= S_SEND_SIGNAL;
                end
            end
            S_SEND_SIGNAL: begin
                send <= 1'h0;           // set send back to 0, and increment plane_count
                plane_count <= plane_count + 1;
                state <= S_RESTART;
            end
            S_RESTART: begin    // if the number of plane_counts is at or over limit, go back to ready state
                if (plane_count >= N_BIT_PLANES) begin
                    state <= S_WAIT_FOR_SERIAL;
                end else begin
                    state <= S_BIT_PLANE_COUNT;
                end
            end
        endcase
    end




endmodule