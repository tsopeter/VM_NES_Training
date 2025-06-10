`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 10/25/2024 01:39:16 PM
// Design Name: 
// Module Name: edge_detector
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


module edge_detector(
    input  wire clock,
    input  wire edge_in,
    output wire rising_edge
    );
    reg sig_dly = 1'h0;

    // This always block ensures that sig_dly is exactly 1 clock behind sig
	always @ (posedge clock) begin
		sig_dly <= edge_in;
	end

	assign rising_edge = edge_in & ~sig_dly;
endmodule
