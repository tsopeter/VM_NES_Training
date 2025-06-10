`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 10/25/2024 02:13:25 PM
// Design Name: 
// Module Name: base
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


module base (
  inout wire [14:0]DDR_addr,
  inout wire [2:0]DDR_ba,
  inout wire DDR_cas_n,
  inout wire DDR_ck_n,
  inout wire DDR_ck_p,
  inout wire DDR_cke,
  inout wire DR_cs_n,
  inout wire [3:0]DDR_dm,
  inout wire [31:0]DDR_dq,
  inout wire [3:0]DDR_dqs_n,
  inout wire [3:0]DDR_dqs_p,
  inout wire DDR_odt,
  inout wire DDR_ras_n,
  inout wire DDR_reset_n,
  inout wire DDR_we_n,
  inout wire FIXED_IO_ddr_vrn,
  inout wire FIXED_IO_ddr_vrp,
  inout wire [53:0]FIXED_IO_mio,
  inout wire FIXED_IO_ps_clk,
  inout wire FIXED_IO_ps_porb,
  inout wire FIXED_IO_ps_srstb,
  
  /* user details */
  input  wire fpin,     /* frame pulse input */
  input  wire bpin,     /* bit pulse input */
  input  wire ppin,     /* PLM pulse input */
  
  output wire fpout,    /* PMOD output */
  output wire fpout_nn,
  
  output wire fpin_led, /* LEDs */
  output wire [3:0] states,
  output wire user_led,
  output wire reset_led,
  
  input  wire btn_fpin,  /* SW */
  input  wire btn_flip,
  input  wire btn_bpin,
  input  wire btn_user,
  input  wire btn_com,
  input  wire btn_ppin,
  input  wire btn_plm_skip,
  
  output wire btn_flip_led,     /* PMOD led */
  output wire btn_plm_skip_led,
  output wire bpin_led,
  output wire ppin_led
  );
    
    assign btn_flip_led = btn_flip;
    assign btn_plm_skip_led = btn_plm_skip;
    assign bpin_led = bpin;
    assign ppin_led = ppin;
    
    wire [31:0] bp_count;
    
    wire FCLK;
    wire [31:0]M_AVALON_0_address;
    wire M_AVALON_0_read;
    reg [31:0]M_AVALON_0_readdata;
    wire M_AVALON_0_write;
    wire [31:0]M_AVALON_0_writedata;
    
    wire fpout_n;
    assign fpout = ~fpout_n;
    assign fpout_nn = fpout_n;
    
    wire fpin_s = (btn_user) ? (btn_fpin) : ((btn_flip) ? fpin : ~fpin);
    assign fpin_led = fpin_s;
    
    wire bpin_s = (btn_user) ? (btn_bpin) : ((btn_flip) ? bpin : ~bpin);
    
    wire ppin_s = (btn_user) ? (btn_ppin) : ((btn_flip) ? ppin : ~ppin);
    
    reg upin  = 1'h0;
    reg reset = 1'h0;
    wire [3:0] state;
    
    assign user_led = upin;
    assign reset_led = reset;
    always @(posedge FCLK) begin
        if (M_AVALON_0_write)
            case (M_AVALON_0_address)
                (32'h43c00000): upin  <= M_AVALON_0_writedata[0];
                (32'h43c0000C): reset <= M_AVALON_0_writedata[0];
            endcase
    end
    
    always @(posedge FCLK) begin
        if (M_AVALON_0_read)
            case (M_AVALON_0_address)
                (32'h43c00000): M_AVALON_0_readdata[0]   <= {31'h0, upin};
                (32'h43c00004): M_AVALON_0_readdata[0]   <= {31'h0, fpin_s};
                (32'h43c00008): M_AVALON_0_readdata[1:0] <= {30'h0, state};
                (32'h43c0000C): M_AVALON_0_readdata[0]   <= {31'h0, reset};
                (32'h43c00010): M_AVALON_0_readdata      <= bp_count;
            endcase
    end
    
    assign states = state;
    
    wire fpin_s_sample, bpin_s_sample, ppin_s_sample;
    
    sampler sm0 (
        .clock(FCLK),
        .value(fpin_s),
        .sample(fpin_s_sample)
    );
    
    sampler sm1 (
        .clock(FCLK),
        .value(bpin_s),
        .sample(bpin_s_sample)
    );
    
    sampler sm2 (
        .clock(FCLK),
        .value(ppin_s),
        .sample(ppin_s_sample)
    );
    
    frame_sync_z#
    (
        .BIT_SKIP(0),
        .BIT_COUNT(12),
        .MPT_MINIMUM(50000) // 500 us
    ) fs (
        .clock(FCLK),
        .reset(reset),
        .frame_pulse_in(fpin_s_sample),
        .bit_pulse_in(bpin_s_sample),
        .com_pulse_in(upin | btn_com),
        .frame_pulse_out(fpout_n),
        .state(state),
        .bp_count_v(bp_count)
    );
    
    design_1_wrapper d1w (
        DDR_addr,
        DDR_ba,
        DDR_cas_n,
        DDR_ck_n,
        DDR_ck_p,
        DDR_cke,
        DDR_cs_n,
        DDR_dm,
        DDR_dq,
        DDR_dqs_n,
        DDR_dqs_p,
        DDR_odt,
        DDR_ras_n,
        DDR_reset_n,
        DDR_we_n,
        FCLK,
        FIXED_IO_ddr_vrn,
        FIXED_IO_ddr_vrp,
        FIXED_IO_mio,
        FIXED_IO_ps_clk,
        FIXED_IO_ps_porb,
        FIXED_IO_ps_srstb,
        M_AVALON_0_address,
        M_AVALON_0_read,
        M_AVALON_0_readdata,
        M_AVALON_0_write,
        M_AVALON_0_writedata
    );
    
endmodule
