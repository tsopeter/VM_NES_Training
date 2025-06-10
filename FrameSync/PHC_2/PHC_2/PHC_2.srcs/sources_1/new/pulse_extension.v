`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 10/25/2024 02:03:58 PM
// Design Name: 
// Module Name: pulse_extension
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


module pulse_extension#(
    parameter WIDTH = 5000  // 50 us
)(
    input  wire clock,
    input  wire pulse,
    output wire pulse_ext
    );
    
    reg state_machine = 1'h0;
    reg [15:0] counter = 16'h0000;
    
    always @(posedge clock) begin
        case (state_machine)
            (1'h0): begin
                counter <= 0;
                if (pulse)
                    state_machine <= 1'h1;
            end
            (1'h1): begin
                // counter till next state
                counter <= counter + 1;
                if (counter >= WIDTH) begin
                    state_machine <= 1'h0;
                end
            end
        endcase
    end
    assign pulse_ext = state_machine;
    
endmodule
