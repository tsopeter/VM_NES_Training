// (c) Copyright 1995-2024 Xilinx, Inc. All rights reserved.
// 
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
// 
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
// 
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
// 
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES.
// 
// DO NOT MODIFY THIS FILE.


// IP VLNV: xilinx.com:ip:axi_amm_bridge:1.0
// IP Revision: 17

(* X_CORE_INFO = "axi_amm_bridge_v1_0_17_top,Vivado 2022.2" *)
(* CHECK_LICENSE_TYPE = "design_1_axi_amm_bridge_0_0,axi_amm_bridge_v1_0_17_top,{}" *)
(* CORE_GENERATION_INFO = "design_1_axi_amm_bridge_0_0,axi_amm_bridge_v1_0_17_top,{x_ipProduct=Vivado 2022.2,x_ipVendor=xilinx.com,x_ipLibrary=ip,x_ipName=axi_amm_bridge,x_ipVersion=1.0,x_ipCoreRevision=17,x_ipLanguage=VERILOG,x_ipSimLanguage=MIXED,C_ADDRESS_MODE=0,C_HAS_FIXED_WAIT=1,C_HAS_RESPONSE=0,C_FIXED_WRITE_WAIT=1,C_FIXED_READ_WAIT=1,C_HAS_FIXED_READ_LATENCY=1,C_READ_LATENCY=1,C_S_AXI_DATA_WIDTH=32,C_S_AXI_ID_WIDTH=1,C_S_AXI_ADDR_WIDTH=32,C_USE_WSTRB=0,C_AVM_BURST_WIDTH=1,C_AXI_LOCK_WIDTH=1,C_BURST_LENGTH=1,C_DPHAS\
E_TIMEOUT=256,C_NUM_ADDRESS_RANGES=0,C_NUM_OUTSTANDING=2,C_PROTOCOL=0,C_BASE1_ADDR=0x0000000000000000,C_BASE2_ADDR=0x0000000000000004,C_BASE3_ADDR=0x0000000000000008,C_BASE4_ADDR=0x000000000000000C,C_HIGH1_ADDR=0x0000000000000003,C_HIGH2_ADDR=0x0000000000000005,C_HIGH3_ADDR=0x0000000000000009,C_HIGH4_ADDR=0x000000000000000F,C_FAMILY=zynq}" *)
(* DowngradeIPIdentifiedWarnings = "yes" *)
module design_1_axi_amm_bridge_0_0 (
  s_axi_aclk,
  s_axi_aresetn,
  s_axi_awaddr,
  s_axi_awvalid,
  s_axi_awready,
  s_axi_wdata,
  s_axi_wstrb,
  s_axi_wvalid,
  s_axi_wready,
  s_axi_bresp,
  s_axi_bvalid,
  s_axi_bready,
  s_axi_araddr,
  s_axi_arvalid,
  s_axi_arready,
  s_axi_rdata,
  s_axi_rresp,
  s_axi_rvalid,
  s_axi_rready,
  avm_address,
  avm_write,
  avm_read,
  avm_writedata,
  avm_readdata
);

(* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME s_axi_aclk, ASSOCIATED_BUSIF S_AXI_LITE:S_AXI_FULL:M_AVALON, ASSOCIATED_RESET s_axi_aresetn, FREQ_HZ 100000000, FREQ_TOLERANCE_HZ 0, PHASE 0.0, CLK_DOMAIN design_1_processing_system7_0_0_FCLK_CLK0, INSERT_VIP 0" *)
(* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 s_axi_aclk CLK" *)
input wire s_axi_aclk;
(* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME s_axi_aresetn, POLARITY ACTIVE_LOW, INSERT_VIP 0" *)
(* X_INTERFACE_INFO = "xilinx.com:signal:reset:1.0 s_axi_aresetn RST" *)
input wire s_axi_aresetn;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE AWADDR" *)
input wire [31 : 0] s_axi_awaddr;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE AWVALID" *)
input wire s_axi_awvalid;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE AWREADY" *)
output wire s_axi_awready;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE WDATA" *)
input wire [31 : 0] s_axi_wdata;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE WSTRB" *)
input wire [3 : 0] s_axi_wstrb;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE WVALID" *)
input wire s_axi_wvalid;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE WREADY" *)
output wire s_axi_wready;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE BRESP" *)
output wire [1 : 0] s_axi_bresp;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE BVALID" *)
output wire s_axi_bvalid;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE BREADY" *)
input wire s_axi_bready;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE ARADDR" *)
input wire [31 : 0] s_axi_araddr;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE ARVALID" *)
input wire s_axi_arvalid;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE ARREADY" *)
output wire s_axi_arready;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE RDATA" *)
output wire [31 : 0] s_axi_rdata;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE RRESP" *)
output wire [1 : 0] s_axi_rresp;
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE RVALID" *)
output wire s_axi_rvalid;
(* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME S_AXI_LITE, DATA_WIDTH 32, PROTOCOL AXI4LITE, FREQ_HZ 100000000, ID_WIDTH 0, ADDR_WIDTH 32, AWUSER_WIDTH 0, ARUSER_WIDTH 0, WUSER_WIDTH 0, RUSER_WIDTH 0, BUSER_WIDTH 0, READ_WRITE_MODE READ_WRITE, HAS_BURST 0, HAS_LOCK 0, HAS_PROT 0, HAS_CACHE 0, HAS_QOS 0, HAS_REGION 0, HAS_WSTRB 1, HAS_BRESP 1, HAS_RRESP 1, SUPPORTS_NARROW_BURST 0, NUM_READ_OUTSTANDING 8, NUM_WRITE_OUTSTANDING 8, MAX_BURST_LENGTH 1, PHASE 0.0, CLK_DOMAIN design_1_processing_system7_0_0_FCLK_CLK0, NUM_READ_THR\
EADS 4, NUM_WRITE_THREADS 4, RUSER_BITS_PER_BYTE 0, WUSER_BITS_PER_BYTE 0, INSERT_VIP 0" *)
(* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI_LITE RREADY" *)
input wire s_axi_rready;
(* X_INTERFACE_INFO = "xilinx.com:interface:avalon:1.0 M_AVALON ADDRESS" *)
output wire [31 : 0] avm_address;
(* X_INTERFACE_INFO = "xilinx.com:interface:avalon:1.0 M_AVALON WRITE" *)
output wire avm_write;
(* X_INTERFACE_INFO = "xilinx.com:interface:avalon:1.0 M_AVALON READ" *)
output wire avm_read;
(* X_INTERFACE_INFO = "xilinx.com:interface:avalon:1.0 M_AVALON WRITEDATA" *)
output wire [31 : 0] avm_writedata;
(* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME M_AVALON, ADDR_WIDTH 32" *)
(* X_INTERFACE_INFO = "xilinx.com:interface:avalon:1.0 M_AVALON READDATA" *)
input wire [31 : 0] avm_readdata;

  axi_amm_bridge_v1_0_17_top #(
    .C_ADDRESS_MODE(0),
    .C_HAS_FIXED_WAIT(1),
    .C_HAS_RESPONSE(0),
    .C_FIXED_WRITE_WAIT(1),
    .C_FIXED_READ_WAIT(1),
    .C_HAS_FIXED_READ_LATENCY(1),
    .C_READ_LATENCY(1),
    .C_S_AXI_DATA_WIDTH(32),
    .C_S_AXI_ID_WIDTH(1),
    .C_S_AXI_ADDR_WIDTH(32),
    .C_USE_WSTRB(0),
    .C_AVM_BURST_WIDTH(1),
    .C_AXI_LOCK_WIDTH(1),
    .C_BURST_LENGTH(1),
    .C_DPHASE_TIMEOUT(256),
    .C_NUM_ADDRESS_RANGES(0),
    .C_NUM_OUTSTANDING(2),
    .C_PROTOCOL(0),
    .C_BASE1_ADDR(64'H0000000000000000),
    .C_BASE2_ADDR(64'H0000000000000004),
    .C_BASE3_ADDR(64'H0000000000000008),
    .C_BASE4_ADDR(64'H000000000000000C),
    .C_HIGH1_ADDR(64'H0000000000000003),
    .C_HIGH2_ADDR(64'H0000000000000005),
    .C_HIGH3_ADDR(64'H0000000000000009),
    .C_HIGH4_ADDR(64'H000000000000000F),
    .C_FAMILY("zynq")
  ) inst (
    .s_axi_aclk(s_axi_aclk),
    .s_axi_aresetn(s_axi_aresetn),
    .s_axi_awid(1'D0),
    .s_axi_awaddr(s_axi_awaddr),
    .s_axi_awlen(8'B0),
    .s_axi_awsize(3'D0),
    .s_axi_awburst(2'D0),
    .s_axi_awvalid(s_axi_awvalid),
    .s_axi_awready(s_axi_awready),
    .s_axi_wdata(s_axi_wdata),
    .s_axi_wstrb(s_axi_wstrb),
    .s_axi_wlast(1'B0),
    .s_axi_wvalid(s_axi_wvalid),
    .s_axi_wready(s_axi_wready),
    .s_axi_bid(),
    .s_axi_bresp(s_axi_bresp),
    .s_axi_bvalid(s_axi_bvalid),
    .s_axi_bready(s_axi_bready),
    .s_axi_arid(1'D0),
    .s_axi_araddr(s_axi_araddr),
    .s_axi_arlen(8'B0),
    .s_axi_arsize(3'D0),
    .s_axi_arburst(2'D0),
    .s_axi_arvalid(s_axi_arvalid),
    .s_axi_arready(s_axi_arready),
    .s_axi_rid(),
    .s_axi_rdata(s_axi_rdata),
    .s_axi_rresp(s_axi_rresp),
    .s_axi_rlast(),
    .s_axi_rvalid(s_axi_rvalid),
    .s_axi_rready(s_axi_rready),
    .avm_address(avm_address),
    .avm_write(avm_write),
    .avm_read(avm_read),
    .avm_byteenable(),
    .avm_writedata(avm_writedata),
    .avm_readdata(avm_readdata),
    .avm_resp(2'D0),
    .avm_readdatavalid(1'D0),
    .avm_burstcount(),
    .avm_beginbursttransfer(),
    .avm_writeresponsevalid(1'D0),
    .avm_waitrequest(1'D0)
  );
endmodule
