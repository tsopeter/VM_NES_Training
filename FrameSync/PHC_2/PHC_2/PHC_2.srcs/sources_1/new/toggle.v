`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 10/25/2024 01:35:30 PM
// Design Name: 
// Module Name: toggle
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


module toggle(
    input  wire in,
    input  wire reset,
    output wire in_ext
    );
    
    reg state_machine = 1'h0;
    always @(in, reset, state_machine) begin
        case (state_machine)
            (1'h0): begin
                if (in)
                    state_machine <= 1'h1;
            end
            (1'h1): begin
                if (reset)
                    state_machine <= 1'h0;
            end
        endcase
    end
    assign in_ext = state_machine;
endmodule
