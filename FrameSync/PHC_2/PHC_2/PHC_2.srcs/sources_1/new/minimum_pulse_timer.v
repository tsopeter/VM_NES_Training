`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/20/2024 03:31:33 PM
// Design Name: 
// Module Name: minimum_pulse_timer
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module minimum_pulse_timer #(
    parameter PULSE_TIME = 133500
)(
    input  wire clock,
    input  wire reset,
    input  wire set,
    output wire ready
);
    reg [1:0] state = 2'h0;
    reg ready_r     = 1'h1;
    
    reg [31:0] counter = 32'h0;
    
    wire [31:0] zPULSE_TIME = PULSE_TIME;

    always @(posedge clock) begin
        if (reset) begin
            state   <= 2'h0;
            ready_r <= 1'h1;
            counter <= 32'h0;
        end
        else begin
            case (state)
                (2'h0): begin   // initial state (assume that it is ready, since there
                                // has been no pulse yet
                    ready_r <= 1'h1;
                    counter <= 32'h0;
                    if (set)
                        state <= 2'h1;
                end
                (2'h1): begin   // counting state
                    ready_r <= 1'h0;
                    counter <= counter + 1;
                    if (counter >= zPULSE_TIME) begin
                        state <= 2'h2;
                    end
                end
                (2'h2): begin
                    state <= 2'h0;
                end
            endcase
        end
    end
    
    assign ready = ready_r;
    
    
endmodule
