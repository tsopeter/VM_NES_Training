`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/22/2024 03:28:51 PM
// Design Name: 
// Module Name: sampler
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


module sampler(
    input  wire clock,
    input  wire value,
    output reg  sample = 1'h0
    );
    
    always @(posedge clock) begin
        sample <= value;
    end
    
endmodule
