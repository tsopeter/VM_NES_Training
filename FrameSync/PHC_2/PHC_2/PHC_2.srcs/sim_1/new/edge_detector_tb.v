`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 10/28/2024 02:24:48 PM
// Design Name: 
// Module Name: edge_detector_tb
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


module edge_detector_tb();

    reg clock = 1'h0;
    reg signal = 1'h0;
    wire re;
    
    always begin
        clock <= 1'h0; #1;
        clock <= 1'h1; #1;
    end

    edge_detector dut (
        .clock(clock),
        .edge_in(signal),
        .rising_edge(re)
    );
    
    initial begin
        signal <= 1'h0;
        
        #10;
        
        signal <= 1'h1;
        
        #11;
        
        signal <= 1'h0;
    end

endmodule
