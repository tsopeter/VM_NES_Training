`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 10/28/2024 01:45:24 PM
// Design Name: 
// Module Name: one_hot_encoder
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


module one_hot_encoder#(
    parameter WIDTH = 2
)(
    input  wire [WIDTH-1:0] in,
    output wire [(2**WIDTH)-1:0] out
    );
    
    assign out = 1 << in;  
endmodule
