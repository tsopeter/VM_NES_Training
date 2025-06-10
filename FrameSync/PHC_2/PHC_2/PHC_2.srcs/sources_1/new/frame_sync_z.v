`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 06/10/2025 03:40:21 PM
// Design Name: 
// Module Name: frame_sync_z
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


module frame_sync_z #(
    parameter BIT_SKIP = 0,
    parameter BIT_COUNT = 12,
    parameter MPT_MINIMUM = 133500
)(
    input  wire clock,
    input  wire frame_pulse_in,
    input  wire bit_pulse_in,
    input  wire com_pulse_in,
    input  wire reset,
    
    output wire frame_pulse_out,
    output wire user_mode,
    output wire [3:0] state,
    output wire [31:0] bp_count_v
   
    );
    
    wire bp;
    wire re;
    wire pp;
    reg  tg_r  = 1'h0;
    wire user;
    reg  pulse = 1'h0;
    wire pulse_ext;
    
    reg mpt_set   = 1'h0;
    wire mpt_ready;
    
    edge_detector ed (
        .clock(clock),
        .edge_in(frame_pulse_in),
        .rising_edge(re)
    );
    
    edge_detector ed2 (
        .clock(clock),
        .edge_in(com_pulse_in),
        .rising_edge(user)
    );
    
    edge_detector ed3 (
        .clock(clock),
        .edge_in(bit_pulse_in),
        .rising_edge(bp)
    );
    
    assign user_mode = user;
    
    pulse_extension #(
        .WIDTH(10000)
    ) pe (
        .clock(clock),
        .pulse(pulse),
        .pulse_ext(pulse_ext)
    );
    
    reg [3:0]  state_machine = 4'h0;
    reg [31:0] bp_count      = 32'h0;
    
    minimum_pulse_timer #(
        .PULSE_TIME(MPT_MINIMUM)
    )
    mpt (
        .clock(clock),
        .reset(reset),
        .set(mpt_set),
        .ready(mpt_ready)
    );
    
    always @(posedge clock) begin
        if (reset)
            state_machine <= 2'h0;
        else
            case (state_machine)
                (4'h0): begin   // default state, where we have no rising edge and no user
                    tg_r        <= 1'h0;
                    bp_count    <= 32'h0;
                    if (user)
                        state_machine <= 4'h1; // enter user state
                end
                (4'h1): begin   // user state, where we now wait for a rising edge
                    if (re) begin
                        state_machine <= 4'h9; // enter pulse state
                    end
                end
                (4'h4): begin // pulse_count state
                    if (bp) begin
                        bp_count <= bp_count + 1;
                        state_machine <= 4'h9;
                    end
                end
                (4'h7): begin
                    pulse <= 1'h0;
                    
                    if (bp_count >= (BIT_SKIP + BIT_COUNT-1)) begin
                        tg_r <= 1'h1;
                        state_machine <= 4'h0;
                    end
                    else begin
                        state_machine <= 4'h4;
                    end
                end
                (4'h9): begin
                    // if mpt is not ready, busy wait till ready
                    if (~mpt_ready) begin
                        state_machine <= 4'h9;
                    end
                    else begin
                        // set mpt to start counting
                        mpt_set <= 1'h1;
                        pulse   <= 1'h1;
                        state_machine <= 4'ha;
                    end
                end
                (4'ha): begin
                    mpt_set <= 1'h0;    // unset mpt
                    pulse   <= 1'h0;
                    
                    state_machine <= 4'h7;  // go to end state
                end
            endcase
    end
    
    assign frame_pulse_out = pulse_ext;
    assign state = state_machine;
    assign bp_count_v = bp_count;
    
endmodule
