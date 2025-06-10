`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 10/28/2024 12:47:16 PM
// Design Name: 
// Module Name: toggle_tb
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


module toggle_tb();
    
    reg  in    = 1'h0;
    reg  reset = 1'h0;
    wire out;
    
    toggle dut (
        .in(in),
        .reset(reset),
        .in_ext(out)
    );
    
    
    initial begin
        in    <= 1'h0;
        reset <= 1'h1;
        
        #10
        
        in <= 1'h1;
        reset <= 1'h0;
        
        #10
        
        in <= 1'h0;
        
        
    end


endmodule
