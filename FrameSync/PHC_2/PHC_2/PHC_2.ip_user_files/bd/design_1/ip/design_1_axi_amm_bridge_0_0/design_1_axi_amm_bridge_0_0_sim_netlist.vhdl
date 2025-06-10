-- Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2022.2 (win64) Build 3671981 Fri Oct 14 05:00:03 MDT 2022
-- Date        : Fri Oct 25 15:06:24 2024
-- Host        : toybox running 64-bit major release  (build 9200)
-- Command     : write_vhdl -force -mode funcsim
--               c:/Users/p221t801/Downloads/PHC/PHC.gen/sources_1/bd/design_1/ip/design_1_axi_amm_bridge_0_0/design_1_axi_amm_bridge_0_0_sim_netlist.vhdl
-- Design      : design_1_axi_amm_bridge_0_0
-- Purpose     : This VHDL netlist is a functional simulation representation of the design and should not be modified or
--               synthesized. This netlist cannot be used for SDF annotated simulation.
-- Device      : xc7z020clg484-1
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_lite is
  port (
    s_axi_rvalid_reg_0 : out STD_LOGIC;
    s_axi_bresp : out STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_rdata : out STD_LOGIC_VECTOR ( 31 downto 0 );
    avm_address : out STD_LOGIC_VECTOR ( 31 downto 0 );
    avm_writedata : out STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_rresp : out STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_bvalid_reg_0 : out STD_LOGIC;
    s_axi_awready : out STD_LOGIC;
    s_axi_wready : out STD_LOGIC;
    s_axi_arready : out STD_LOGIC;
    avm_write : out STD_LOGIC;
    avm_read : out STD_LOGIC;
    s_axi_rready : in STD_LOGIC;
    s_axi_aresetn : in STD_LOGIC;
    s_axi_aclk : in STD_LOGIC;
    avm_readdata : in STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_wdata : in STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_arvalid : in STD_LOGIC;
    s_axi_awvalid : in STD_LOGIC;
    s_axi_wvalid : in STD_LOGIC;
    s_axi_bready : in STD_LOGIC;
    s_axi_araddr : in STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_awaddr : in STD_LOGIC_VECTOR ( 31 downto 0 )
  );
  attribute ORIG_REF_NAME : string;
  attribute ORIG_REF_NAME of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_lite : entity is "axi_amm_bridge_v1_0_17_lite";
end design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_lite;

architecture STRUCTURE of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_lite is
  signal \FIX_RL.avm_readdatavalid_ii_i_2_n_0\ : STD_LOGIC;
  signal \FIX_RL.avm_readdatavalid_ii_reg_n_0\ : STD_LOGIC;
  signal \FIX_RL.rd_lat_count[0]_i_1_n_0\ : STD_LOGIC;
  signal \FIX_RL.rd_lat_count[5]_i_1_n_0\ : STD_LOGIC;
  signal \FIX_RL.rd_lat_count_reg\ : STD_LOGIC_VECTOR ( 5 downto 0 );
  signal \FSM_onehot_current_state[0]_i_1_n_0\ : STD_LOGIC;
  signal \FSM_onehot_current_state[1]_i_1_n_0\ : STD_LOGIC;
  signal \FSM_onehot_current_state[1]_i_2_n_0\ : STD_LOGIC;
  signal \FSM_onehot_current_state[2]_i_1_n_0\ : STD_LOGIC;
  signal \FSM_onehot_current_state[3]_i_1_n_0\ : STD_LOGIC;
  signal \FSM_onehot_current_state[3]_i_2_n_0\ : STD_LOGIC;
  signal \FSM_onehot_current_state[4]_i_1_n_0\ : STD_LOGIC;
  signal \FSM_onehot_current_state[5]_i_1_n_0\ : STD_LOGIC;
  signal \FSM_onehot_current_state[6]_i_1_n_0\ : STD_LOGIC;
  signal \FSM_onehot_current_state[7]_i_1_n_0\ : STD_LOGIC;
  signal \FSM_onehot_current_state[8]_i_1_n_0\ : STD_LOGIC;
  signal \FSM_onehot_current_state_reg_n_0_[0]\ : STD_LOGIC;
  signal \FSM_onehot_current_state_reg_n_0_[2]\ : STD_LOGIC;
  signal \FSM_onehot_current_state_reg_n_0_[3]\ : STD_LOGIC;
  signal \FSM_onehot_current_state_reg_n_0_[5]\ : STD_LOGIC;
  signal \FSM_onehot_current_state_reg_n_0_[7]\ : STD_LOGIC;
  signal \FSM_onehot_current_state_reg_n_0_[8]\ : STD_LOGIC;
  signal \NO_FIX_WT.avm_waitrequest_i_i_1_n_0\ : STD_LOGIC;
  signal \NO_FIX_WT.avm_waitrequest_i_i_2_n_0\ : STD_LOGIC;
  signal \NO_FIX_WT.avm_waitrequest_i_reg_n_0\ : STD_LOGIC;
  signal \NO_FIX_WT.fix_wait_count[0]_i_1_n_0\ : STD_LOGIC;
  signal \NO_FIX_WT.fix_wait_count[7]_i_1_n_0\ : STD_LOGIC;
  signal \NO_FIX_WT.fix_wait_count[7]_i_3_n_0\ : STD_LOGIC;
  signal \NO_FIX_WT.fix_wait_count_reg\ : STD_LOGIC_VECTOR ( 7 downto 0 );
  signal \avm_address[0]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[10]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[11]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[12]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[13]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[14]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[15]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[16]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[17]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[18]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[19]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[1]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[20]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[21]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[22]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[23]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[24]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[25]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[26]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[27]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[28]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[29]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[2]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[30]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[31]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[31]_i_2_n_0\ : STD_LOGIC;
  signal \avm_address[31]_i_3_n_0\ : STD_LOGIC;
  signal \avm_address[31]_i_4_n_0\ : STD_LOGIC;
  signal \avm_address[3]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[4]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[5]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[6]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[7]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[8]_i_1_n_0\ : STD_LOGIC;
  signal \avm_address[9]_i_1_n_0\ : STD_LOGIC;
  signal \^avm_read\ : STD_LOGIC;
  signal avm_read_i_1_n_0 : STD_LOGIC;
  signal avm_readdata_i : STD_LOGIC;
  signal avm_readdata_id : STD_LOGIC_VECTOR ( 31 downto 0 );
  signal avm_readdatavalid_i : STD_LOGIC;
  signal avm_readdatavalid_ii10_out : STD_LOGIC;
  signal avm_resp_i1 : STD_LOGIC;
  signal avm_resp_id : STD_LOGIC_VECTOR ( 1 to 1 );
  signal \^avm_write\ : STD_LOGIC;
  signal avm_write_i_1_n_0 : STD_LOGIC;
  signal avm_write_i_2_n_0 : STD_LOGIC;
  signal \avm_writedata[31]_i_1_n_0\ : STD_LOGIC;
  signal \avm_writedata[31]_i_2_n_0\ : STD_LOGIC;
  signal clear : STD_LOGIC;
  signal current_state_d : STD_LOGIC_VECTOR ( 3 downto 0 );
  signal current_state_reg : STD_LOGIC_VECTOR ( 2 downto 0 );
  signal p_0_in : STD_LOGIC_VECTOR ( 7 downto 1 );
  signal p_0_in17_in : STD_LOGIC;
  signal p_0_in18_in : STD_LOGIC;
  signal \p_0_in__0\ : STD_LOGIC_VECTOR ( 8 downto 0 );
  signal \p_0_in__1\ : STD_LOGIC_VECTOR ( 5 downto 1 );
  signal \^s_axi_arready\ : STD_LOGIC;
  signal s_axi_arready_i_1_n_0 : STD_LOGIC;
  signal \^s_axi_awready\ : STD_LOGIC;
  signal s_axi_awready_i_1_n_0 : STD_LOGIC;
  signal s_axi_awready_i_2_n_0 : STD_LOGIC;
  signal s_axi_awready_i_3_n_0 : STD_LOGIC;
  signal s_axi_awready_i_4_n_0 : STD_LOGIC;
  signal \^s_axi_bresp\ : STD_LOGIC_VECTOR ( 0 to 0 );
  signal \s_axi_bresp[1]_i_1_n_0\ : STD_LOGIC;
  signal \s_axi_bresp[1]_i_2_n_0\ : STD_LOGIC;
  signal \s_axi_bresp[1]_i_3_n_0\ : STD_LOGIC;
  signal s_axi_bvalid_i_1_n_0 : STD_LOGIC;
  signal \^s_axi_bvalid_reg_0\ : STD_LOGIC;
  signal \s_axi_rdata[31]_i_1_n_0\ : STD_LOGIC;
  signal \s_axi_rdata[31]_i_3_n_0\ : STD_LOGIC;
  signal \s_axi_rdata[31]_i_4_n_0\ : STD_LOGIC;
  signal \^s_axi_rresp\ : STD_LOGIC_VECTOR ( 0 to 0 );
  signal \s_axi_rresp[1]_i_1_n_0\ : STD_LOGIC;
  signal \s_axi_rresp[1]_i_2_n_0\ : STD_LOGIC;
  signal s_axi_rvalid_i_1_n_0 : STD_LOGIC;
  signal \^s_axi_rvalid_reg_0\ : STD_LOGIC;
  signal \^s_axi_wready\ : STD_LOGIC;
  signal s_axi_wready_i_1_n_0 : STD_LOGIC;
  signal start : STD_LOGIC;
  signal start_i_1_n_0 : STD_LOGIC;
  signal start_i_2_n_0 : STD_LOGIC;
  signal start_i_3_n_0 : STD_LOGIC;
  signal start_i_4_n_0 : STD_LOGIC;
  signal start_i_6_n_0 : STD_LOGIC;
  signal start_i_7_n_0 : STD_LOGIC;
  signal start_i_8_n_0 : STD_LOGIC;
  signal start_i_9_n_0 : STD_LOGIC;
  signal start_reg_n_0 : STD_LOGIC;
  signal \tout_counter[8]_i_3_n_0\ : STD_LOGIC;
  signal tout_counter_reg : STD_LOGIC_VECTOR ( 8 downto 0 );
  attribute SOFT_HLUTNM : string;
  attribute SOFT_HLUTNM of \FIX_RL.avm_readdatavalid_ii_i_2\ : label is "soft_lutpair9";
  attribute SOFT_HLUTNM of \FIX_RL.current_state_d[1]_i_1\ : label is "soft_lutpair13";
  attribute SOFT_HLUTNM of \FIX_RL.current_state_d[2]_i_1\ : label is "soft_lutpair15";
  attribute SOFT_HLUTNM of \FIX_RL.rd_lat_count[2]_i_1\ : label is "soft_lutpair9";
  attribute SOFT_HLUTNM of \FIX_RL.rd_lat_count[3]_i_1\ : label is "soft_lutpair7";
  attribute SOFT_HLUTNM of \FIX_RL.rd_lat_count[4]_i_1\ : label is "soft_lutpair7";
  attribute SOFT_HLUTNM of \FSM_onehot_current_state[0]_i_1\ : label is "soft_lutpair1";
  attribute SOFT_HLUTNM of \FSM_onehot_current_state[1]_i_2\ : label is "soft_lutpair13";
  attribute SOFT_HLUTNM of \FSM_onehot_current_state[2]_i_1\ : label is "soft_lutpair17";
  attribute SOFT_HLUTNM of \FSM_onehot_current_state[5]_i_1\ : label is "soft_lutpair8";
  attribute SOFT_HLUTNM of \FSM_onehot_current_state[6]_i_1\ : label is "soft_lutpair14";
  attribute SOFT_HLUTNM of \FSM_onehot_current_state[8]_i_1\ : label is "soft_lutpair17";
  attribute FSM_ENCODED_STATES : string;
  attribute FSM_ENCODED_STATES of \FSM_onehot_current_state_reg[0]\ : label is "READ_ADDRESS:000000001,READ_DATA:000000010,WRITE_RESP:000100000,WRITE_AD_DATA:010000000,IDLE:000001000,INV_WRITE_DATA:001000000,INV_READ_ADDRESS:000000100,INV_READ_DATA:000010000,INV_WRITE_ADDRESS:100000000";
  attribute FSM_ENCODED_STATES of \FSM_onehot_current_state_reg[1]\ : label is "READ_ADDRESS:000000001,READ_DATA:000000010,WRITE_RESP:000100000,WRITE_AD_DATA:010000000,IDLE:000001000,INV_WRITE_DATA:001000000,INV_READ_ADDRESS:000000100,INV_READ_DATA:000010000,INV_WRITE_ADDRESS:100000000";
  attribute FSM_ENCODED_STATES of \FSM_onehot_current_state_reg[2]\ : label is "READ_ADDRESS:000000001,READ_DATA:000000010,WRITE_RESP:000100000,WRITE_AD_DATA:010000000,IDLE:000001000,INV_WRITE_DATA:001000000,INV_READ_ADDRESS:000000100,INV_READ_DATA:000010000,INV_WRITE_ADDRESS:100000000";
  attribute FSM_ENCODED_STATES of \FSM_onehot_current_state_reg[3]\ : label is "READ_ADDRESS:000000001,READ_DATA:000000010,WRITE_RESP:000100000,WRITE_AD_DATA:010000000,IDLE:000001000,INV_WRITE_DATA:001000000,INV_READ_ADDRESS:000000100,INV_READ_DATA:000010000,INV_WRITE_ADDRESS:100000000";
  attribute FSM_ENCODED_STATES of \FSM_onehot_current_state_reg[4]\ : label is "READ_ADDRESS:000000001,READ_DATA:000000010,WRITE_RESP:000100000,WRITE_AD_DATA:010000000,IDLE:000001000,INV_WRITE_DATA:001000000,INV_READ_ADDRESS:000000100,INV_READ_DATA:000010000,INV_WRITE_ADDRESS:100000000";
  attribute FSM_ENCODED_STATES of \FSM_onehot_current_state_reg[5]\ : label is "READ_ADDRESS:000000001,READ_DATA:000000010,WRITE_RESP:000100000,WRITE_AD_DATA:010000000,IDLE:000001000,INV_WRITE_DATA:001000000,INV_READ_ADDRESS:000000100,INV_READ_DATA:000010000,INV_WRITE_ADDRESS:100000000";
  attribute FSM_ENCODED_STATES of \FSM_onehot_current_state_reg[6]\ : label is "READ_ADDRESS:000000001,READ_DATA:000000010,WRITE_RESP:000100000,WRITE_AD_DATA:010000000,IDLE:000001000,INV_WRITE_DATA:001000000,INV_READ_ADDRESS:000000100,INV_READ_DATA:000010000,INV_WRITE_ADDRESS:100000000";
  attribute FSM_ENCODED_STATES of \FSM_onehot_current_state_reg[7]\ : label is "READ_ADDRESS:000000001,READ_DATA:000000010,WRITE_RESP:000100000,WRITE_AD_DATA:010000000,IDLE:000001000,INV_WRITE_DATA:001000000,INV_READ_ADDRESS:000000100,INV_READ_DATA:000010000,INV_WRITE_ADDRESS:100000000";
  attribute FSM_ENCODED_STATES of \FSM_onehot_current_state_reg[8]\ : label is "READ_ADDRESS:000000001,READ_DATA:000000010,WRITE_RESP:000100000,WRITE_AD_DATA:010000000,IDLE:000001000,INV_WRITE_DATA:001000000,INV_READ_ADDRESS:000000100,INV_READ_DATA:000010000,INV_WRITE_ADDRESS:100000000";
  attribute SOFT_HLUTNM of \NO_FIX_WT.avm_waitrequest_i_i_2\ : label is "soft_lutpair10";
  attribute SOFT_HLUTNM of \NO_FIX_WT.fix_wait_count[1]_i_1\ : label is "soft_lutpair10";
  attribute SOFT_HLUTNM of \NO_FIX_WT.fix_wait_count[2]_i_1\ : label is "soft_lutpair11";
  attribute SOFT_HLUTNM of \NO_FIX_WT.fix_wait_count[3]_i_1\ : label is "soft_lutpair11";
  attribute SOFT_HLUTNM of \NO_FIX_WT.fix_wait_count[4]_i_1\ : label is "soft_lutpair5";
  attribute SOFT_HLUTNM of \NO_FIX_WT.fix_wait_count[6]_i_1\ : label is "soft_lutpair3";
  attribute SOFT_HLUTNM of \NO_FIX_WT.fix_wait_count[7]_i_2\ : label is "soft_lutpair3";
  attribute SOFT_HLUTNM of \NO_FIX_WT.fix_wait_count[7]_i_3\ : label is "soft_lutpair5";
  attribute SOFT_HLUTNM of \avm_address[31]_i_3\ : label is "soft_lutpair0";
  attribute SOFT_HLUTNM of avm_read_i_1 : label is "soft_lutpair1";
  attribute SOFT_HLUTNM of \s_axi_bresp[1]_i_2\ : label is "soft_lutpair16";
  attribute SOFT_HLUTNM of s_axi_bvalid_i_1 : label is "soft_lutpair14";
  attribute SOFT_HLUTNM of \s_axi_rdata[31]_i_3\ : label is "soft_lutpair16";
  attribute SOFT_HLUTNM of \s_axi_rdata[31]_i_4\ : label is "soft_lutpair15";
  attribute SOFT_HLUTNM of \s_axi_rresp[1]_i_2\ : label is "soft_lutpair2";
  attribute SOFT_HLUTNM of start_i_2 : label is "soft_lutpair2";
  attribute SOFT_HLUTNM of start_i_7 : label is "soft_lutpair0";
  attribute SOFT_HLUTNM of start_i_8 : label is "soft_lutpair8";
  attribute SOFT_HLUTNM of \tout_counter[2]_i_1\ : label is "soft_lutpair12";
  attribute SOFT_HLUTNM of \tout_counter[3]_i_1\ : label is "soft_lutpair12";
  attribute SOFT_HLUTNM of \tout_counter[4]_i_1\ : label is "soft_lutpair6";
  attribute SOFT_HLUTNM of \tout_counter[6]_i_1\ : label is "soft_lutpair4";
  attribute SOFT_HLUTNM of \tout_counter[7]_i_1\ : label is "soft_lutpair4";
  attribute SOFT_HLUTNM of \tout_counter[8]_i_3\ : label is "soft_lutpair6";
begin
  avm_read <= \^avm_read\;
  avm_write <= \^avm_write\;
  s_axi_arready <= \^s_axi_arready\;
  s_axi_awready <= \^s_axi_awready\;
  s_axi_bresp(0) <= \^s_axi_bresp\(0);
  s_axi_bvalid_reg_0 <= \^s_axi_bvalid_reg_0\;
  s_axi_rresp(0) <= \^s_axi_rresp\(0);
  s_axi_rvalid_reg_0 <= \^s_axi_rvalid_reg_0\;
  s_axi_wready <= \^s_axi_wready\;
\FIX_RL.avm_readdata_id_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(0),
      Q => avm_readdata_id(0),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[10]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(10),
      Q => avm_readdata_id(10),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[11]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(11),
      Q => avm_readdata_id(11),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[12]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(12),
      Q => avm_readdata_id(12),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[13]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(13),
      Q => avm_readdata_id(13),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[14]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(14),
      Q => avm_readdata_id(14),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[15]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(15),
      Q => avm_readdata_id(15),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[16]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(16),
      Q => avm_readdata_id(16),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[17]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(17),
      Q => avm_readdata_id(17),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[18]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(18),
      Q => avm_readdata_id(18),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[19]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(19),
      Q => avm_readdata_id(19),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(1),
      Q => avm_readdata_id(1),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[20]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(20),
      Q => avm_readdata_id(20),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[21]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(21),
      Q => avm_readdata_id(21),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[22]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(22),
      Q => avm_readdata_id(22),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[23]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(23),
      Q => avm_readdata_id(23),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[24]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(24),
      Q => avm_readdata_id(24),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[25]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(25),
      Q => avm_readdata_id(25),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[26]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(26),
      Q => avm_readdata_id(26),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[27]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(27),
      Q => avm_readdata_id(27),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[28]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(28),
      Q => avm_readdata_id(28),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[29]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(29),
      Q => avm_readdata_id(29),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(2),
      Q => avm_readdata_id(2),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[30]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(30),
      Q => avm_readdata_id(30),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[31]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(31),
      Q => avm_readdata_id(31),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(3),
      Q => avm_readdata_id(3),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(4),
      Q => avm_readdata_id(4),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(5),
      Q => avm_readdata_id(5),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(6),
      Q => avm_readdata_id(6),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(7),
      Q => avm_readdata_id(7),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[8]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(8),
      Q => avm_readdata_id(8),
      R => '0'
    );
\FIX_RL.avm_readdata_id_reg[9]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata(9),
      Q => avm_readdata_id(9),
      R => '0'
    );
\FIX_RL.avm_readdatavalid_ii_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"00000001"
    )
        port map (
      I0 => \FIX_RL.rd_lat_count_reg\(2),
      I1 => \FIX_RL.rd_lat_count_reg\(3),
      I2 => \FIX_RL.rd_lat_count_reg\(4),
      I3 => \FIX_RL.rd_lat_count_reg\(5),
      I4 => \FIX_RL.avm_readdatavalid_ii_i_2_n_0\,
      O => avm_readdatavalid_ii10_out
    );
\FIX_RL.avm_readdatavalid_ii_i_2\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FFDF"
    )
        port map (
      I0 => \FIX_RL.rd_lat_count_reg\(0),
      I1 => \FIX_RL.rd_lat_count_reg\(1),
      I2 => p_0_in17_in,
      I3 => \^s_axi_rvalid_reg_0\,
      O => \FIX_RL.avm_readdatavalid_ii_i_2_n_0\
    );
\FIX_RL.avm_readdatavalid_ii_reg\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdatavalid_ii10_out,
      Q => \FIX_RL.avm_readdatavalid_ii_reg_n_0\,
      R => s_axi_awready_i_1_n_0
    );
\FIX_RL.avm_resp_id[1]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"70FFFFFF70707070"
    )
        port map (
      I0 => \^s_axi_rvalid_reg_0\,
      I1 => s_axi_rready,
      I2 => avm_readdata_i,
      I3 => \^s_axi_bvalid_reg_0\,
      I4 => s_axi_bready,
      I5 => p_0_in18_in,
      O => avm_resp_i1
    );
\FIX_RL.avm_resp_id_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_resp_i1,
      Q => avm_resp_id(1),
      R => '0'
    );
\FIX_RL.current_state_d[0]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FFFE"
    )
        port map (
      I0 => p_0_in18_in,
      I1 => \FSM_onehot_current_state_reg_n_0_[8]\,
      I2 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I3 => \FSM_onehot_current_state_reg_n_0_[7]\,
      O => current_state_reg(0)
    );
\FIX_RL.current_state_d[1]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FFFE"
    )
        port map (
      I0 => p_0_in18_in,
      I1 => \FSM_onehot_current_state_reg_n_0_[5]\,
      I2 => \FSM_onehot_current_state_reg_n_0_[2]\,
      I3 => \FSM_onehot_current_state_reg_n_0_[0]\,
      O => current_state_reg(1)
    );
\FIX_RL.current_state_d[2]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FFFE"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[2]\,
      I1 => p_0_in17_in,
      I2 => p_0_in18_in,
      I3 => \FSM_onehot_current_state_reg_n_0_[8]\,
      O => current_state_reg(2)
    );
\FIX_RL.current_state_d_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => current_state_reg(0),
      Q => current_state_d(0),
      R => '0'
    );
\FIX_RL.current_state_d_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => current_state_reg(1),
      Q => current_state_d(1),
      R => '0'
    );
\FIX_RL.current_state_d_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => current_state_reg(2),
      Q => current_state_d(2),
      R => '0'
    );
\FIX_RL.current_state_d_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_readdata_i,
      Q => current_state_d(3),
      R => '0'
    );
\FIX_RL.rd_lat_count[0]_i_1\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \FIX_RL.rd_lat_count_reg\(0),
      O => \FIX_RL.rd_lat_count[0]_i_1_n_0\
    );
\FIX_RL.rd_lat_count[1]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"6"
    )
        port map (
      I0 => \FIX_RL.rd_lat_count_reg\(1),
      I1 => \FIX_RL.rd_lat_count_reg\(0),
      O => \p_0_in__1\(1)
    );
\FIX_RL.rd_lat_count[2]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"6A"
    )
        port map (
      I0 => \FIX_RL.rd_lat_count_reg\(2),
      I1 => \FIX_RL.rd_lat_count_reg\(1),
      I2 => \FIX_RL.rd_lat_count_reg\(0),
      O => \p_0_in__1\(2)
    );
\FIX_RL.rd_lat_count[3]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"7F80"
    )
        port map (
      I0 => \FIX_RL.rd_lat_count_reg\(0),
      I1 => \FIX_RL.rd_lat_count_reg\(1),
      I2 => \FIX_RL.rd_lat_count_reg\(2),
      I3 => \FIX_RL.rd_lat_count_reg\(3),
      O => \p_0_in__1\(3)
    );
\FIX_RL.rd_lat_count[4]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"6AAAAAAA"
    )
        port map (
      I0 => \FIX_RL.rd_lat_count_reg\(4),
      I1 => \FIX_RL.rd_lat_count_reg\(0),
      I2 => \FIX_RL.rd_lat_count_reg\(1),
      I3 => \FIX_RL.rd_lat_count_reg\(2),
      I4 => \FIX_RL.rd_lat_count_reg\(3),
      O => \p_0_in__1\(4)
    );
\FIX_RL.rd_lat_count[5]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"DF"
    )
        port map (
      I0 => p_0_in17_in,
      I1 => \^s_axi_rvalid_reg_0\,
      I2 => s_axi_aresetn,
      O => \FIX_RL.rd_lat_count[5]_i_1_n_0\
    );
\FIX_RL.rd_lat_count[5]_i_2\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"6AAAAAAAAAAAAAAA"
    )
        port map (
      I0 => \FIX_RL.rd_lat_count_reg\(5),
      I1 => \FIX_RL.rd_lat_count_reg\(3),
      I2 => \FIX_RL.rd_lat_count_reg\(2),
      I3 => \FIX_RL.rd_lat_count_reg\(1),
      I4 => \FIX_RL.rd_lat_count_reg\(0),
      I5 => \FIX_RL.rd_lat_count_reg\(4),
      O => \p_0_in__1\(5)
    );
\FIX_RL.rd_lat_count_reg[0]\: unisim.vcomponents.FDSE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \FIX_RL.rd_lat_count[0]_i_1_n_0\,
      Q => \FIX_RL.rd_lat_count_reg\(0),
      S => \FIX_RL.rd_lat_count[5]_i_1_n_0\
    );
\FIX_RL.rd_lat_count_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \p_0_in__1\(1),
      Q => \FIX_RL.rd_lat_count_reg\(1),
      R => \FIX_RL.rd_lat_count[5]_i_1_n_0\
    );
\FIX_RL.rd_lat_count_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \p_0_in__1\(2),
      Q => \FIX_RL.rd_lat_count_reg\(2),
      R => \FIX_RL.rd_lat_count[5]_i_1_n_0\
    );
\FIX_RL.rd_lat_count_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \p_0_in__1\(3),
      Q => \FIX_RL.rd_lat_count_reg\(3),
      R => \FIX_RL.rd_lat_count[5]_i_1_n_0\
    );
\FIX_RL.rd_lat_count_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \p_0_in__1\(4),
      Q => \FIX_RL.rd_lat_count_reg\(4),
      R => \FIX_RL.rd_lat_count[5]_i_1_n_0\
    );
\FIX_RL.rd_lat_count_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \p_0_in__1\(5),
      Q => \FIX_RL.rd_lat_count_reg\(5),
      R => \FIX_RL.rd_lat_count[5]_i_1_n_0\
    );
\FSM_onehot_current_state[0]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"F888"
    )
        port map (
      I0 => s_axi_arvalid,
      I1 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I2 => s_axi_awready_i_3_n_0,
      I3 => \FSM_onehot_current_state_reg_n_0_[0]\,
      O => \FSM_onehot_current_state[0]_i_1_n_0\
    );
\FSM_onehot_current_state[1]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"FFFFFFFF70707040"
    )
        port map (
      I0 => s_axi_rready,
      I1 => \^s_axi_rvalid_reg_0\,
      I2 => p_0_in17_in,
      I3 => \avm_address[31]_i_3_n_0\,
      I4 => avm_readdatavalid_i,
      I5 => \FSM_onehot_current_state[1]_i_2_n_0\,
      O => \FSM_onehot_current_state[1]_i_1_n_0\
    );
\FSM_onehot_current_state[1]_i_2\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"2"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => \NO_FIX_WT.avm_waitrequest_i_reg_n_0\,
      O => \FSM_onehot_current_state[1]_i_2_n_0\
    );
\FSM_onehot_current_state[2]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"08"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => \NO_FIX_WT.avm_waitrequest_i_reg_n_0\,
      I2 => \avm_address[31]_i_3_n_0\,
      O => \FSM_onehot_current_state[2]_i_1_n_0\
    );
\FSM_onehot_current_state[3]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"FFFFEEEAEEEAEEEA"
    )
        port map (
      I0 => \FSM_onehot_current_state[3]_i_2_n_0\,
      I1 => \s_axi_bresp[1]_i_3_n_0\,
      I2 => p_0_in18_in,
      I3 => \FSM_onehot_current_state_reg_n_0_[5]\,
      I4 => avm_readdata_i,
      I5 => \s_axi_rdata[31]_i_3_n_0\,
      O => \FSM_onehot_current_state[3]_i_1_n_0\
    );
\FSM_onehot_current_state[3]_i_2\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"88888F888F888F88"
    )
        port map (
      I0 => \s_axi_rdata[31]_i_3_n_0\,
      I1 => p_0_in17_in,
      I2 => s_axi_arvalid,
      I3 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I4 => s_axi_awvalid,
      I5 => s_axi_wvalid,
      O => \FSM_onehot_current_state[3]_i_2_n_0\
    );
\FSM_onehot_current_state[4]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"FFFFFFFFFFFF0100"
    )
        port map (
      I0 => avm_readdatavalid_i,
      I1 => \avm_address[31]_i_3_n_0\,
      I2 => \^s_axi_rvalid_reg_0\,
      I3 => p_0_in17_in,
      I4 => \s_axi_bresp[1]_i_2_n_0\,
      I5 => \FSM_onehot_current_state_reg_n_0_[2]\,
      O => \FSM_onehot_current_state[4]_i_1_n_0\
    );
\FSM_onehot_current_state[5]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"70FF7070"
    )
        port map (
      I0 => \^s_axi_bvalid_reg_0\,
      I1 => s_axi_bready,
      I2 => \FSM_onehot_current_state_reg_n_0_[5]\,
      I3 => \NO_FIX_WT.avm_waitrequest_i_reg_n_0\,
      I4 => \FSM_onehot_current_state_reg_n_0_[7]\,
      O => \FSM_onehot_current_state[5]_i_1_n_0\
    );
\FSM_onehot_current_state[6]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"BFAA"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[8]\,
      I1 => \^s_axi_bvalid_reg_0\,
      I2 => s_axi_bready,
      I3 => p_0_in18_in,
      O => \FSM_onehot_current_state[6]_i_1_n_0\
    );
\FSM_onehot_current_state[7]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"8F88888888888888"
    )
        port map (
      I0 => s_axi_awready_i_3_n_0,
      I1 => \FSM_onehot_current_state_reg_n_0_[7]\,
      I2 => s_axi_arvalid,
      I3 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I4 => s_axi_awvalid,
      I5 => s_axi_wvalid,
      O => \FSM_onehot_current_state[7]_i_1_n_0\
    );
\FSM_onehot_current_state[8]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"08"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[7]\,
      I1 => \NO_FIX_WT.avm_waitrequest_i_reg_n_0\,
      I2 => \avm_address[31]_i_3_n_0\,
      O => \FSM_onehot_current_state[8]_i_1_n_0\
    );
\FSM_onehot_current_state_reg[0]\: unisim.vcomponents.FDRE
    generic map(
      INIT => '0'
    )
        port map (
      C => s_axi_aclk,
      CE => '1',
      D => \FSM_onehot_current_state[0]_i_1_n_0\,
      Q => \FSM_onehot_current_state_reg_n_0_[0]\,
      R => s_axi_awready_i_1_n_0
    );
\FSM_onehot_current_state_reg[1]\: unisim.vcomponents.FDRE
    generic map(
      INIT => '0'
    )
        port map (
      C => s_axi_aclk,
      CE => '1',
      D => \FSM_onehot_current_state[1]_i_1_n_0\,
      Q => p_0_in17_in,
      R => s_axi_awready_i_1_n_0
    );
\FSM_onehot_current_state_reg[2]\: unisim.vcomponents.FDRE
    generic map(
      INIT => '0'
    )
        port map (
      C => s_axi_aclk,
      CE => '1',
      D => \FSM_onehot_current_state[2]_i_1_n_0\,
      Q => \FSM_onehot_current_state_reg_n_0_[2]\,
      R => s_axi_awready_i_1_n_0
    );
\FSM_onehot_current_state_reg[3]\: unisim.vcomponents.FDSE
    generic map(
      INIT => '1'
    )
        port map (
      C => s_axi_aclk,
      CE => '1',
      D => \FSM_onehot_current_state[3]_i_1_n_0\,
      Q => \FSM_onehot_current_state_reg_n_0_[3]\,
      S => s_axi_awready_i_1_n_0
    );
\FSM_onehot_current_state_reg[4]\: unisim.vcomponents.FDRE
    generic map(
      INIT => '0'
    )
        port map (
      C => s_axi_aclk,
      CE => '1',
      D => \FSM_onehot_current_state[4]_i_1_n_0\,
      Q => avm_readdata_i,
      R => s_axi_awready_i_1_n_0
    );
\FSM_onehot_current_state_reg[5]\: unisim.vcomponents.FDRE
    generic map(
      INIT => '0'
    )
        port map (
      C => s_axi_aclk,
      CE => '1',
      D => \FSM_onehot_current_state[5]_i_1_n_0\,
      Q => \FSM_onehot_current_state_reg_n_0_[5]\,
      R => s_axi_awready_i_1_n_0
    );
\FSM_onehot_current_state_reg[6]\: unisim.vcomponents.FDRE
    generic map(
      INIT => '0'
    )
        port map (
      C => s_axi_aclk,
      CE => '1',
      D => \FSM_onehot_current_state[6]_i_1_n_0\,
      Q => p_0_in18_in,
      R => s_axi_awready_i_1_n_0
    );
\FSM_onehot_current_state_reg[7]\: unisim.vcomponents.FDRE
    generic map(
      INIT => '0'
    )
        port map (
      C => s_axi_aclk,
      CE => '1',
      D => \FSM_onehot_current_state[7]_i_1_n_0\,
      Q => \FSM_onehot_current_state_reg_n_0_[7]\,
      R => s_axi_awready_i_1_n_0
    );
\FSM_onehot_current_state_reg[8]\: unisim.vcomponents.FDRE
    generic map(
      INIT => '0'
    )
        port map (
      C => s_axi_aclk,
      CE => '1',
      D => \FSM_onehot_current_state[8]_i_1_n_0\,
      Q => \FSM_onehot_current_state_reg_n_0_[8]\,
      R => s_axi_awready_i_1_n_0
    );
\NO_FIX_WT.avm_waitrequest_i_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"FFFFFFFFFFFFFFFE"
    )
        port map (
      I0 => \NO_FIX_WT.fix_wait_count[7]_i_1_n_0\,
      I1 => \NO_FIX_WT.avm_waitrequest_i_i_2_n_0\,
      I2 => \NO_FIX_WT.fix_wait_count_reg\(7),
      I3 => \NO_FIX_WT.fix_wait_count_reg\(3),
      I4 => \NO_FIX_WT.fix_wait_count_reg\(2),
      I5 => \NO_FIX_WT.fix_wait_count_reg\(1),
      O => \NO_FIX_WT.avm_waitrequest_i_i_1_n_0\
    );
\NO_FIX_WT.avm_waitrequest_i_i_2\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FFFD"
    )
        port map (
      I0 => \NO_FIX_WT.fix_wait_count_reg\(0),
      I1 => \NO_FIX_WT.fix_wait_count_reg\(5),
      I2 => \NO_FIX_WT.fix_wait_count_reg\(6),
      I3 => \NO_FIX_WT.fix_wait_count_reg\(4),
      O => \NO_FIX_WT.avm_waitrequest_i_i_2_n_0\
    );
\NO_FIX_WT.avm_waitrequest_i_reg\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \NO_FIX_WT.avm_waitrequest_i_i_1_n_0\,
      Q => \NO_FIX_WT.avm_waitrequest_i_reg_n_0\,
      R => '0'
    );
\NO_FIX_WT.fix_wait_count[0]_i_1\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => \NO_FIX_WT.fix_wait_count_reg\(0),
      O => \NO_FIX_WT.fix_wait_count[0]_i_1_n_0\
    );
\NO_FIX_WT.fix_wait_count[1]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"6"
    )
        port map (
      I0 => \NO_FIX_WT.fix_wait_count_reg\(1),
      I1 => \NO_FIX_WT.fix_wait_count_reg\(0),
      O => p_0_in(1)
    );
\NO_FIX_WT.fix_wait_count[2]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"6A"
    )
        port map (
      I0 => \NO_FIX_WT.fix_wait_count_reg\(2),
      I1 => \NO_FIX_WT.fix_wait_count_reg\(1),
      I2 => \NO_FIX_WT.fix_wait_count_reg\(0),
      O => p_0_in(2)
    );
\NO_FIX_WT.fix_wait_count[3]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"7F80"
    )
        port map (
      I0 => \NO_FIX_WT.fix_wait_count_reg\(0),
      I1 => \NO_FIX_WT.fix_wait_count_reg\(1),
      I2 => \NO_FIX_WT.fix_wait_count_reg\(2),
      I3 => \NO_FIX_WT.fix_wait_count_reg\(3),
      O => p_0_in(3)
    );
\NO_FIX_WT.fix_wait_count[4]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"6AAAAAAA"
    )
        port map (
      I0 => \NO_FIX_WT.fix_wait_count_reg\(4),
      I1 => \NO_FIX_WT.fix_wait_count_reg\(0),
      I2 => \NO_FIX_WT.fix_wait_count_reg\(1),
      I3 => \NO_FIX_WT.fix_wait_count_reg\(2),
      I4 => \NO_FIX_WT.fix_wait_count_reg\(3),
      O => p_0_in(4)
    );
\NO_FIX_WT.fix_wait_count[5]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"6AAAAAAAAAAAAAAA"
    )
        port map (
      I0 => \NO_FIX_WT.fix_wait_count_reg\(5),
      I1 => \NO_FIX_WT.fix_wait_count_reg\(3),
      I2 => \NO_FIX_WT.fix_wait_count_reg\(2),
      I3 => \NO_FIX_WT.fix_wait_count_reg\(1),
      I4 => \NO_FIX_WT.fix_wait_count_reg\(0),
      I5 => \NO_FIX_WT.fix_wait_count_reg\(4),
      O => p_0_in(5)
    );
\NO_FIX_WT.fix_wait_count[6]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"6AAA"
    )
        port map (
      I0 => \NO_FIX_WT.fix_wait_count_reg\(6),
      I1 => \NO_FIX_WT.fix_wait_count_reg\(4),
      I2 => \NO_FIX_WT.fix_wait_count[7]_i_3_n_0\,
      I3 => \NO_FIX_WT.fix_wait_count_reg\(5),
      O => p_0_in(6)
    );
\NO_FIX_WT.fix_wait_count[7]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"1F"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => \FSM_onehot_current_state_reg_n_0_[7]\,
      I2 => s_axi_aresetn,
      O => \NO_FIX_WT.fix_wait_count[7]_i_1_n_0\
    );
\NO_FIX_WT.fix_wait_count[7]_i_2\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"6AAAAAAA"
    )
        port map (
      I0 => \NO_FIX_WT.fix_wait_count_reg\(7),
      I1 => \NO_FIX_WT.fix_wait_count_reg\(5),
      I2 => \NO_FIX_WT.fix_wait_count[7]_i_3_n_0\,
      I3 => \NO_FIX_WT.fix_wait_count_reg\(4),
      I4 => \NO_FIX_WT.fix_wait_count_reg\(6),
      O => p_0_in(7)
    );
\NO_FIX_WT.fix_wait_count[7]_i_3\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"8000"
    )
        port map (
      I0 => \NO_FIX_WT.fix_wait_count_reg\(3),
      I1 => \NO_FIX_WT.fix_wait_count_reg\(2),
      I2 => \NO_FIX_WT.fix_wait_count_reg\(1),
      I3 => \NO_FIX_WT.fix_wait_count_reg\(0),
      O => \NO_FIX_WT.fix_wait_count[7]_i_3_n_0\
    );
\NO_FIX_WT.fix_wait_count_reg[0]\: unisim.vcomponents.FDSE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \NO_FIX_WT.fix_wait_count[0]_i_1_n_0\,
      Q => \NO_FIX_WT.fix_wait_count_reg\(0),
      S => \NO_FIX_WT.fix_wait_count[7]_i_1_n_0\
    );
\NO_FIX_WT.fix_wait_count_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => p_0_in(1),
      Q => \NO_FIX_WT.fix_wait_count_reg\(1),
      R => \NO_FIX_WT.fix_wait_count[7]_i_1_n_0\
    );
\NO_FIX_WT.fix_wait_count_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => p_0_in(2),
      Q => \NO_FIX_WT.fix_wait_count_reg\(2),
      R => \NO_FIX_WT.fix_wait_count[7]_i_1_n_0\
    );
\NO_FIX_WT.fix_wait_count_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => p_0_in(3),
      Q => \NO_FIX_WT.fix_wait_count_reg\(3),
      R => \NO_FIX_WT.fix_wait_count[7]_i_1_n_0\
    );
\NO_FIX_WT.fix_wait_count_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => p_0_in(4),
      Q => \NO_FIX_WT.fix_wait_count_reg\(4),
      R => \NO_FIX_WT.fix_wait_count[7]_i_1_n_0\
    );
\NO_FIX_WT.fix_wait_count_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => p_0_in(5),
      Q => \NO_FIX_WT.fix_wait_count_reg\(5),
      R => \NO_FIX_WT.fix_wait_count[7]_i_1_n_0\
    );
\NO_FIX_WT.fix_wait_count_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => p_0_in(6),
      Q => \NO_FIX_WT.fix_wait_count_reg\(6),
      R => \NO_FIX_WT.fix_wait_count[7]_i_1_n_0\
    );
\NO_FIX_WT.fix_wait_count_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => p_0_in(7),
      Q => \NO_FIX_WT.fix_wait_count_reg\(7),
      R => \NO_FIX_WT.fix_wait_count[7]_i_1_n_0\
    );
\avm_address[0]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(0),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(0),
      O => \avm_address[0]_i_1_n_0\
    );
\avm_address[10]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(10),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(10),
      O => \avm_address[10]_i_1_n_0\
    );
\avm_address[11]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(11),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(11),
      O => \avm_address[11]_i_1_n_0\
    );
\avm_address[12]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(12),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(12),
      O => \avm_address[12]_i_1_n_0\
    );
\avm_address[13]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(13),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(13),
      O => \avm_address[13]_i_1_n_0\
    );
\avm_address[14]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(14),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(14),
      O => \avm_address[14]_i_1_n_0\
    );
\avm_address[15]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(15),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(15),
      O => \avm_address[15]_i_1_n_0\
    );
\avm_address[16]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(16),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(16),
      O => \avm_address[16]_i_1_n_0\
    );
\avm_address[17]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(17),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(17),
      O => \avm_address[17]_i_1_n_0\
    );
\avm_address[18]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(18),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(18),
      O => \avm_address[18]_i_1_n_0\
    );
\avm_address[19]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(19),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(19),
      O => \avm_address[19]_i_1_n_0\
    );
\avm_address[1]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(1),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(1),
      O => \avm_address[1]_i_1_n_0\
    );
\avm_address[20]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(20),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(20),
      O => \avm_address[20]_i_1_n_0\
    );
\avm_address[21]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(21),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(21),
      O => \avm_address[21]_i_1_n_0\
    );
\avm_address[22]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(22),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(22),
      O => \avm_address[22]_i_1_n_0\
    );
\avm_address[23]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(23),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(23),
      O => \avm_address[23]_i_1_n_0\
    );
\avm_address[24]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(24),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(24),
      O => \avm_address[24]_i_1_n_0\
    );
\avm_address[25]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(25),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(25),
      O => \avm_address[25]_i_1_n_0\
    );
\avm_address[26]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(26),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(26),
      O => \avm_address[26]_i_1_n_0\
    );
\avm_address[27]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(27),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(27),
      O => \avm_address[27]_i_1_n_0\
    );
\avm_address[28]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(28),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(28),
      O => \avm_address[28]_i_1_n_0\
    );
\avm_address[29]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(29),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(29),
      O => \avm_address[29]_i_1_n_0\
    );
\avm_address[2]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(2),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(2),
      O => \avm_address[2]_i_1_n_0\
    );
\avm_address[30]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(30),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(30),
      O => \avm_address[30]_i_1_n_0\
    );
\avm_address[31]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"FFFFF380"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[7]\,
      I1 => \NO_FIX_WT.avm_waitrequest_i_reg_n_0\,
      I2 => \avm_address[31]_i_3_n_0\,
      I3 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I4 => \FSM_onehot_current_state_reg_n_0_[3]\,
      O => \avm_address[31]_i_1_n_0\
    );
\avm_address[31]_i_2\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(31),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(31),
      O => \avm_address[31]_i_2_n_0\
    );
\avm_address[31]_i_3\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"FFFFFEFF"
    )
        port map (
      I0 => tout_counter_reg(1),
      I1 => tout_counter_reg(7),
      I2 => tout_counter_reg(2),
      I3 => start_reg_n_0,
      I4 => s_axi_awready_i_4_n_0,
      O => \avm_address[31]_i_3_n_0\
    );
\avm_address[31]_i_4\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"0000BFFF"
    )
        port map (
      I0 => s_axi_arvalid,
      I1 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I2 => s_axi_awvalid,
      I3 => s_axi_wvalid,
      I4 => \FSM_onehot_current_state_reg_n_0_[7]\,
      O => \avm_address[31]_i_4_n_0\
    );
\avm_address[3]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(3),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(3),
      O => \avm_address[3]_i_1_n_0\
    );
\avm_address[4]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(4),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(4),
      O => \avm_address[4]_i_1_n_0\
    );
\avm_address[5]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(5),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(5),
      O => \avm_address[5]_i_1_n_0\
    );
\avm_address[6]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(6),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(6),
      O => \avm_address[6]_i_1_n_0\
    );
\avm_address[7]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(7),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(7),
      O => \avm_address[7]_i_1_n_0\
    );
\avm_address[8]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(8),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(8),
      O => \avm_address[8]_i_1_n_0\
    );
\avm_address[9]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"EA00FFFFEA00EA00"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I1 => s_axi_arvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_araddr(9),
      I4 => \avm_address[31]_i_4_n_0\,
      I5 => s_axi_awaddr(9),
      O => \avm_address[9]_i_1_n_0\
    );
\avm_address_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[0]_i_1_n_0\,
      Q => avm_address(0),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[10]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[10]_i_1_n_0\,
      Q => avm_address(10),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[11]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[11]_i_1_n_0\,
      Q => avm_address(11),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[12]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[12]_i_1_n_0\,
      Q => avm_address(12),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[13]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[13]_i_1_n_0\,
      Q => avm_address(13),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[14]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[14]_i_1_n_0\,
      Q => avm_address(14),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[15]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[15]_i_1_n_0\,
      Q => avm_address(15),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[16]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[16]_i_1_n_0\,
      Q => avm_address(16),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[17]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[17]_i_1_n_0\,
      Q => avm_address(17),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[18]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[18]_i_1_n_0\,
      Q => avm_address(18),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[19]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[19]_i_1_n_0\,
      Q => avm_address(19),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[1]_i_1_n_0\,
      Q => avm_address(1),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[20]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[20]_i_1_n_0\,
      Q => avm_address(20),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[21]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[21]_i_1_n_0\,
      Q => avm_address(21),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[22]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[22]_i_1_n_0\,
      Q => avm_address(22),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[23]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[23]_i_1_n_0\,
      Q => avm_address(23),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[24]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[24]_i_1_n_0\,
      Q => avm_address(24),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[25]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[25]_i_1_n_0\,
      Q => avm_address(25),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[26]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[26]_i_1_n_0\,
      Q => avm_address(26),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[27]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[27]_i_1_n_0\,
      Q => avm_address(27),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[28]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[28]_i_1_n_0\,
      Q => avm_address(28),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[29]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[29]_i_1_n_0\,
      Q => avm_address(29),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[2]_i_1_n_0\,
      Q => avm_address(2),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[30]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[30]_i_1_n_0\,
      Q => avm_address(30),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[31]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[31]_i_2_n_0\,
      Q => avm_address(31),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[3]_i_1_n_0\,
      Q => avm_address(3),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[4]_i_1_n_0\,
      Q => avm_address(4),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[5]_i_1_n_0\,
      Q => avm_address(5),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[6]_i_1_n_0\,
      Q => avm_address(6),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[7]_i_1_n_0\,
      Q => avm_address(7),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[8]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[8]_i_1_n_0\,
      Q => avm_address(8),
      R => s_axi_awready_i_1_n_0
    );
\avm_address_reg[9]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_address[31]_i_1_n_0\,
      D => \avm_address[9]_i_1_n_0\,
      Q => avm_address(9),
      R => s_axi_awready_i_1_n_0
    );
avm_read_i_1: unisim.vcomponents.LUT5
    generic map(
      INIT => X"EACFEAC0"
    )
        port map (
      I0 => s_axi_arvalid,
      I1 => s_axi_awready_i_3_n_0,
      I2 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I3 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I4 => \^avm_read\,
      O => avm_read_i_1_n_0
    );
avm_read_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_read_i_1_n_0,
      Q => \^avm_read\,
      R => s_axi_awready_i_1_n_0
    );
avm_write_i_1: unisim.vcomponents.LUT5
    generic map(
      INIT => X"B3BFB3B0"
    )
        port map (
      I0 => s_axi_awready_i_3_n_0,
      I1 => avm_write_i_2_n_0,
      I2 => \FSM_onehot_current_state_reg_n_0_[7]\,
      I3 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I4 => \^avm_write\,
      O => avm_write_i_1_n_0
    );
avm_write_i_2: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FF7F"
    )
        port map (
      I0 => s_axi_wvalid,
      I1 => s_axi_awvalid,
      I2 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I3 => s_axi_arvalid,
      O => avm_write_i_2_n_0
    );
avm_write_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => avm_write_i_1_n_0,
      Q => \^avm_write\,
      R => s_axi_awready_i_1_n_0
    );
\avm_writedata[31]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"05001100"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I1 => avm_write_i_2_n_0,
      I2 => \NO_FIX_WT.avm_waitrequest_i_reg_n_0\,
      I3 => s_axi_aresetn,
      I4 => \FSM_onehot_current_state_reg_n_0_[7]\,
      O => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata[31]_i_2\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"8088CCCC"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[7]\,
      I1 => s_axi_aresetn,
      I2 => \avm_address[31]_i_3_n_0\,
      I3 => \NO_FIX_WT.avm_waitrequest_i_reg_n_0\,
      I4 => avm_write_i_2_n_0,
      O => \avm_writedata[31]_i_2_n_0\
    );
\avm_writedata_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(0),
      Q => avm_writedata(0),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[10]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(10),
      Q => avm_writedata(10),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[11]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(11),
      Q => avm_writedata(11),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[12]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(12),
      Q => avm_writedata(12),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[13]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(13),
      Q => avm_writedata(13),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[14]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(14),
      Q => avm_writedata(14),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[15]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(15),
      Q => avm_writedata(15),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[16]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(16),
      Q => avm_writedata(16),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[17]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(17),
      Q => avm_writedata(17),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[18]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(18),
      Q => avm_writedata(18),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[19]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(19),
      Q => avm_writedata(19),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(1),
      Q => avm_writedata(1),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[20]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(20),
      Q => avm_writedata(20),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[21]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(21),
      Q => avm_writedata(21),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[22]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(22),
      Q => avm_writedata(22),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[23]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(23),
      Q => avm_writedata(23),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[24]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(24),
      Q => avm_writedata(24),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[25]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(25),
      Q => avm_writedata(25),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[26]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(26),
      Q => avm_writedata(26),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[27]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(27),
      Q => avm_writedata(27),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[28]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(28),
      Q => avm_writedata(28),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[29]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(29),
      Q => avm_writedata(29),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(2),
      Q => avm_writedata(2),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[30]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(30),
      Q => avm_writedata(30),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[31]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(31),
      Q => avm_writedata(31),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(3),
      Q => avm_writedata(3),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(4),
      Q => avm_writedata(4),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(5),
      Q => avm_writedata(5),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(6),
      Q => avm_writedata(6),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(7),
      Q => avm_writedata(7),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[8]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(8),
      Q => avm_writedata(8),
      R => \avm_writedata[31]_i_1_n_0\
    );
\avm_writedata_reg[9]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => \avm_writedata[31]_i_2_n_0\,
      D => s_axi_wdata(9),
      Q => avm_writedata(9),
      R => \avm_writedata[31]_i_1_n_0\
    );
s_axi_arready_i_1: unisim.vcomponents.LUT6
    generic map(
      INIT => X"5050505350505050"
    )
        port map (
      I0 => s_axi_awready_i_3_n_0,
      I1 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I2 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I3 => \FSM_onehot_current_state_reg_n_0_[2]\,
      I4 => p_0_in17_in,
      I5 => \^s_axi_arready\,
      O => s_axi_arready_i_1_n_0
    );
s_axi_arready_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => s_axi_arready_i_1_n_0,
      Q => \^s_axi_arready\,
      R => s_axi_awready_i_1_n_0
    );
s_axi_awready_i_1: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => s_axi_aresetn,
      O => s_axi_awready_i_1_n_0
    );
s_axi_awready_i_2: unisim.vcomponents.LUT6
    generic map(
      INIT => X"5050505350505050"
    )
        port map (
      I0 => s_axi_awready_i_3_n_0,
      I1 => \FSM_onehot_current_state_reg_n_0_[8]\,
      I2 => \FSM_onehot_current_state_reg_n_0_[7]\,
      I3 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I4 => \FSM_onehot_current_state_reg_n_0_[5]\,
      I5 => \^s_axi_awready\,
      O => s_axi_awready_i_2_n_0
    );
s_axi_awready_i_3: unisim.vcomponents.LUT6
    generic map(
      INIT => X"FFFFFFFB00000000"
    )
        port map (
      I0 => s_axi_awready_i_4_n_0,
      I1 => start_reg_n_0,
      I2 => tout_counter_reg(2),
      I3 => tout_counter_reg(7),
      I4 => tout_counter_reg(1),
      I5 => \NO_FIX_WT.avm_waitrequest_i_reg_n_0\,
      O => s_axi_awready_i_3_n_0
    );
s_axi_awready_i_4: unisim.vcomponents.LUT6
    generic map(
      INIT => X"FFFFFFFFFFFFFFFD"
    )
        port map (
      I0 => tout_counter_reg(8),
      I1 => tout_counter_reg(6),
      I2 => tout_counter_reg(0),
      I3 => tout_counter_reg(4),
      I4 => tout_counter_reg(3),
      I5 => tout_counter_reg(5),
      O => s_axi_awready_i_4_n_0
    );
s_axi_awready_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => s_axi_awready_i_2_n_0,
      Q => \^s_axi_awready\,
      R => s_axi_awready_i_1_n_0
    );
\s_axi_bresp[1]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"00FB000000F80000"
    )
        port map (
      I0 => \s_axi_bresp[1]_i_2_n_0\,
      I1 => \FSM_onehot_current_state_reg_n_0_[5]\,
      I2 => p_0_in18_in,
      I3 => \s_axi_bresp[1]_i_3_n_0\,
      I4 => s_axi_aresetn,
      I5 => \^s_axi_bresp\(0),
      O => \s_axi_bresp[1]_i_1_n_0\
    );
\s_axi_bresp[1]_i_2\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"2A"
    )
        port map (
      I0 => avm_readdata_i,
      I1 => s_axi_rready,
      I2 => \^s_axi_rvalid_reg_0\,
      O => \s_axi_bresp[1]_i_2_n_0\
    );
\s_axi_bresp[1]_i_3\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \^s_axi_bvalid_reg_0\,
      I1 => s_axi_bready,
      O => \s_axi_bresp[1]_i_3_n_0\
    );
\s_axi_bresp_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \s_axi_bresp[1]_i_1_n_0\,
      Q => \^s_axi_bresp\(0),
      R => '0'
    );
s_axi_bvalid_i_1: unisim.vcomponents.LUT4
    generic map(
      INIT => X"7774"
    )
        port map (
      I0 => s_axi_bready,
      I1 => \^s_axi_bvalid_reg_0\,
      I2 => p_0_in18_in,
      I3 => \FSM_onehot_current_state_reg_n_0_[5]\,
      O => s_axi_bvalid_i_1_n_0
    );
s_axi_bvalid_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => s_axi_bvalid_i_1_n_0,
      Q => \^s_axi_bvalid_reg_0\,
      R => s_axi_awready_i_1_n_0
    );
\s_axi_rdata[31]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"FFFF7F557F557F55"
    )
        port map (
      I0 => s_axi_aresetn,
      I1 => \FIX_RL.avm_readdatavalid_ii_reg_n_0\,
      I2 => p_0_in17_in,
      I3 => \s_axi_rdata[31]_i_3_n_0\,
      I4 => avm_readdatavalid_i,
      I5 => avm_readdata_i,
      O => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata[31]_i_2\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"00100000FFFFFFFF"
    )
        port map (
      I0 => current_state_d(1),
      I1 => current_state_d(0),
      I2 => current_state_d(3),
      I3 => current_state_d(2),
      I4 => \s_axi_bresp[1]_i_2_n_0\,
      I5 => \s_axi_rdata[31]_i_4_n_0\,
      O => avm_readdatavalid_i
    );
\s_axi_rdata[31]_i_3\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"8"
    )
        port map (
      I0 => \^s_axi_rvalid_reg_0\,
      I1 => s_axi_rready,
      O => \s_axi_rdata[31]_i_3_n_0\
    );
\s_axi_rdata[31]_i_4\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"7"
    )
        port map (
      I0 => p_0_in17_in,
      I1 => \FIX_RL.avm_readdatavalid_ii_reg_n_0\,
      O => \s_axi_rdata[31]_i_4_n_0\
    );
\s_axi_rdata_reg[0]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(0),
      Q => s_axi_rdata(0),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[10]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(10),
      Q => s_axi_rdata(10),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[11]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(11),
      Q => s_axi_rdata(11),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[12]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(12),
      Q => s_axi_rdata(12),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[13]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(13),
      Q => s_axi_rdata(13),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[14]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(14),
      Q => s_axi_rdata(14),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[15]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(15),
      Q => s_axi_rdata(15),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[16]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(16),
      Q => s_axi_rdata(16),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[17]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(17),
      Q => s_axi_rdata(17),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[18]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(18),
      Q => s_axi_rdata(18),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[19]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(19),
      Q => s_axi_rdata(19),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(1),
      Q => s_axi_rdata(1),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[20]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(20),
      Q => s_axi_rdata(20),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[21]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(21),
      Q => s_axi_rdata(21),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[22]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(22),
      Q => s_axi_rdata(22),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[23]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(23),
      Q => s_axi_rdata(23),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[24]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(24),
      Q => s_axi_rdata(24),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[25]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(25),
      Q => s_axi_rdata(25),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[26]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(26),
      Q => s_axi_rdata(26),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[27]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(27),
      Q => s_axi_rdata(27),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[28]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(28),
      Q => s_axi_rdata(28),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[29]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(29),
      Q => s_axi_rdata(29),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(2),
      Q => s_axi_rdata(2),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[30]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(30),
      Q => s_axi_rdata(30),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[31]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(31),
      Q => s_axi_rdata(31),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(3),
      Q => s_axi_rdata(3),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(4),
      Q => s_axi_rdata(4),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(5),
      Q => s_axi_rdata(5),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(6),
      Q => s_axi_rdata(6),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(7),
      Q => s_axi_rdata(7),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[8]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(8),
      Q => s_axi_rdata(8),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rdata_reg[9]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => avm_readdatavalid_i,
      D => avm_readdata_id(9),
      Q => s_axi_rdata(9),
      R => \s_axi_rdata[31]_i_1_n_0\
    );
\s_axi_rresp[1]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"00E2"
    )
        port map (
      I0 => \^s_axi_rresp\(0),
      I1 => avm_readdatavalid_i,
      I2 => avm_resp_id(1),
      I3 => \s_axi_rresp[1]_i_2_n_0\,
      O => \s_axi_rresp[1]_i_1_n_0\
    );
\s_axi_rresp[1]_i_2\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"0888FFFF"
    )
        port map (
      I0 => \^s_axi_rvalid_reg_0\,
      I1 => s_axi_rready,
      I2 => p_0_in17_in,
      I3 => \FIX_RL.avm_readdatavalid_ii_reg_n_0\,
      I4 => s_axi_aresetn,
      O => \s_axi_rresp[1]_i_2_n_0\
    );
\s_axi_rresp_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \s_axi_rresp[1]_i_1_n_0\,
      Q => \^s_axi_rresp\(0),
      R => '0'
    );
s_axi_rvalid_i_1: unisim.vcomponents.LUT3
    generic map(
      INIT => X"F4"
    )
        port map (
      I0 => s_axi_rready,
      I1 => \^s_axi_rvalid_reg_0\,
      I2 => avm_readdatavalid_i,
      O => s_axi_rvalid_i_1_n_0
    );
s_axi_rvalid_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => s_axi_rvalid_i_1_n_0,
      Q => \^s_axi_rvalid_reg_0\,
      R => s_axi_awready_i_1_n_0
    );
s_axi_wready_i_1: unisim.vcomponents.LUT6
    generic map(
      INIT => X"5050505350505050"
    )
        port map (
      I0 => s_axi_awready_i_3_n_0,
      I1 => \FSM_onehot_current_state_reg_n_0_[8]\,
      I2 => \FSM_onehot_current_state_reg_n_0_[7]\,
      I3 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I4 => \FSM_onehot_current_state_reg_n_0_[5]\,
      I5 => \^s_axi_wready\,
      O => s_axi_wready_i_1_n_0
    );
s_axi_wready_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => s_axi_wready_i_1_n_0,
      Q => \^s_axi_wready\,
      R => s_axi_awready_i_1_n_0
    );
start_i_1: unisim.vcomponents.LUT6
    generic map(
      INIT => X"FF54FFFFFF540000"
    )
        port map (
      I0 => start_i_2_n_0,
      I1 => \^s_axi_rvalid_reg_0\,
      I2 => start_i_3_n_0,
      I3 => start_i_4_n_0,
      I4 => start,
      I5 => start_reg_n_0,
      O => start_i_1_n_0
    );
start_i_2: unisim.vcomponents.LUT3
    generic map(
      INIT => X"8F"
    )
        port map (
      I0 => s_axi_rready,
      I1 => \^s_axi_rvalid_reg_0\,
      I2 => p_0_in17_in,
      O => start_i_2_n_0
    );
start_i_3: unisim.vcomponents.LUT6
    generic map(
      INIT => X"FFFFFFFFFFFF88F8"
    )
        port map (
      I0 => p_0_in17_in,
      I1 => \FIX_RL.avm_readdatavalid_ii_reg_n_0\,
      I2 => \s_axi_bresp[1]_i_2_n_0\,
      I3 => start_i_6_n_0,
      I4 => s_axi_awready_i_4_n_0,
      I5 => start_i_7_n_0,
      O => start_i_3_n_0
    );
start_i_4: unisim.vcomponents.LUT6
    generic map(
      INIT => X"FEFEFEAAAAAAAAAA"
    )
        port map (
      I0 => start_i_8_n_0,
      I1 => \FSM_onehot_current_state_reg_n_0_[7]\,
      I2 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I3 => s_axi_awready_i_4_n_0,
      I4 => start_i_7_n_0,
      I5 => \NO_FIX_WT.avm_waitrequest_i_reg_n_0\,
      O => start_i_4_n_0
    );
start_i_5: unisim.vcomponents.LUT6
    generic map(
      INIT => X"FFFFFFFFFFFFFF80"
    )
        port map (
      I0 => \^s_axi_rvalid_reg_0\,
      I1 => s_axi_rready,
      I2 => avm_readdata_i,
      I3 => p_0_in17_in,
      I4 => \FSM_onehot_current_state_reg_n_0_[0]\,
      I5 => start_i_9_n_0,
      O => start
    );
start_i_6: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FFEF"
    )
        port map (
      I0 => current_state_d(1),
      I1 => current_state_d(0),
      I2 => current_state_d(3),
      I3 => current_state_d(2),
      O => start_i_6_n_0
    );
start_i_7: unisim.vcomponents.LUT4
    generic map(
      INIT => X"FFFD"
    )
        port map (
      I0 => start_reg_n_0,
      I1 => tout_counter_reg(2),
      I2 => tout_counter_reg(7),
      I3 => tout_counter_reg(1),
      O => start_i_7_n_0
    );
start_i_8: unisim.vcomponents.LUT3
    generic map(
      INIT => X"2A"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[5]\,
      I1 => s_axi_bready,
      I2 => \^s_axi_bvalid_reg_0\,
      O => start_i_8_n_0
    );
start_i_9: unisim.vcomponents.LUT3
    generic map(
      INIT => X"FE"
    )
        port map (
      I0 => \FSM_onehot_current_state_reg_n_0_[5]\,
      I1 => \FSM_onehot_current_state_reg_n_0_[3]\,
      I2 => \FSM_onehot_current_state_reg_n_0_[7]\,
      O => start_i_9_n_0
    );
start_reg: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => start_i_1_n_0,
      Q => start_reg_n_0,
      R => s_axi_awready_i_1_n_0
    );
\tout_counter[0]_i_1\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => tout_counter_reg(0),
      O => \p_0_in__0\(0)
    );
\tout_counter[1]_i_1\: unisim.vcomponents.LUT2
    generic map(
      INIT => X"6"
    )
        port map (
      I0 => tout_counter_reg(0),
      I1 => tout_counter_reg(1),
      O => \p_0_in__0\(1)
    );
\tout_counter[2]_i_1\: unisim.vcomponents.LUT3
    generic map(
      INIT => X"6A"
    )
        port map (
      I0 => tout_counter_reg(2),
      I1 => tout_counter_reg(0),
      I2 => tout_counter_reg(1),
      O => \p_0_in__0\(2)
    );
\tout_counter[3]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"7F80"
    )
        port map (
      I0 => tout_counter_reg(1),
      I1 => tout_counter_reg(0),
      I2 => tout_counter_reg(2),
      I3 => tout_counter_reg(3),
      O => \p_0_in__0\(3)
    );
\tout_counter[4]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"6AAAAAAA"
    )
        port map (
      I0 => tout_counter_reg(4),
      I1 => tout_counter_reg(1),
      I2 => tout_counter_reg(0),
      I3 => tout_counter_reg(2),
      I4 => tout_counter_reg(3),
      O => \p_0_in__0\(4)
    );
\tout_counter[5]_i_1\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"6AAAAAAAAAAAAAAA"
    )
        port map (
      I0 => tout_counter_reg(5),
      I1 => tout_counter_reg(3),
      I2 => tout_counter_reg(2),
      I3 => tout_counter_reg(0),
      I4 => tout_counter_reg(1),
      I5 => tout_counter_reg(4),
      O => \p_0_in__0\(5)
    );
\tout_counter[6]_i_1\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"6AAA"
    )
        port map (
      I0 => tout_counter_reg(6),
      I1 => tout_counter_reg(4),
      I2 => \tout_counter[8]_i_3_n_0\,
      I3 => tout_counter_reg(5),
      O => \p_0_in__0\(6)
    );
\tout_counter[7]_i_1\: unisim.vcomponents.LUT5
    generic map(
      INIT => X"6AAAAAAA"
    )
        port map (
      I0 => tout_counter_reg(7),
      I1 => tout_counter_reg(5),
      I2 => \tout_counter[8]_i_3_n_0\,
      I3 => tout_counter_reg(4),
      I4 => tout_counter_reg(6),
      O => \p_0_in__0\(7)
    );
\tout_counter[8]_i_1\: unisim.vcomponents.LUT1
    generic map(
      INIT => X"1"
    )
        port map (
      I0 => start_reg_n_0,
      O => clear
    );
\tout_counter[8]_i_2\: unisim.vcomponents.LUT6
    generic map(
      INIT => X"6AAAAAAAAAAAAAAA"
    )
        port map (
      I0 => tout_counter_reg(8),
      I1 => tout_counter_reg(6),
      I2 => tout_counter_reg(4),
      I3 => \tout_counter[8]_i_3_n_0\,
      I4 => tout_counter_reg(5),
      I5 => tout_counter_reg(7),
      O => \p_0_in__0\(8)
    );
\tout_counter[8]_i_3\: unisim.vcomponents.LUT4
    generic map(
      INIT => X"8000"
    )
        port map (
      I0 => tout_counter_reg(3),
      I1 => tout_counter_reg(2),
      I2 => tout_counter_reg(0),
      I3 => tout_counter_reg(1),
      O => \tout_counter[8]_i_3_n_0\
    );
\tout_counter_reg[0]\: unisim.vcomponents.FDSE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \p_0_in__0\(0),
      Q => tout_counter_reg(0),
      S => clear
    );
\tout_counter_reg[1]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \p_0_in__0\(1),
      Q => tout_counter_reg(1),
      R => clear
    );
\tout_counter_reg[2]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \p_0_in__0\(2),
      Q => tout_counter_reg(2),
      R => clear
    );
\tout_counter_reg[3]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \p_0_in__0\(3),
      Q => tout_counter_reg(3),
      R => clear
    );
\tout_counter_reg[4]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \p_0_in__0\(4),
      Q => tout_counter_reg(4),
      R => clear
    );
\tout_counter_reg[5]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \p_0_in__0\(5),
      Q => tout_counter_reg(5),
      R => clear
    );
\tout_counter_reg[6]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \p_0_in__0\(6),
      Q => tout_counter_reg(6),
      R => clear
    );
\tout_counter_reg[7]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \p_0_in__0\(7),
      Q => tout_counter_reg(7),
      R => clear
    );
\tout_counter_reg[8]\: unisim.vcomponents.FDRE
     port map (
      C => s_axi_aclk,
      CE => '1',
      D => \p_0_in__0\(8),
      Q => tout_counter_reg(8),
      R => clear
    );
end STRUCTURE;
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top is
  port (
    s_axi_aclk : in STD_LOGIC;
    s_axi_aresetn : in STD_LOGIC;
    s_axi_awid : in STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_awaddr : in STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_awlen : in STD_LOGIC_VECTOR ( 7 downto 0 );
    s_axi_awsize : in STD_LOGIC_VECTOR ( 2 downto 0 );
    s_axi_awburst : in STD_LOGIC_VECTOR ( 1 downto 0 );
    s_axi_awvalid : in STD_LOGIC;
    s_axi_awready : out STD_LOGIC;
    s_axi_wdata : in STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_wstrb : in STD_LOGIC_VECTOR ( 3 downto 0 );
    s_axi_wlast : in STD_LOGIC;
    s_axi_wvalid : in STD_LOGIC;
    s_axi_wready : out STD_LOGIC;
    s_axi_bid : out STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_bresp : out STD_LOGIC_VECTOR ( 1 downto 0 );
    s_axi_bvalid : out STD_LOGIC;
    s_axi_bready : in STD_LOGIC;
    s_axi_arid : in STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_araddr : in STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_arlen : in STD_LOGIC_VECTOR ( 7 downto 0 );
    s_axi_arsize : in STD_LOGIC_VECTOR ( 2 downto 0 );
    s_axi_arburst : in STD_LOGIC_VECTOR ( 1 downto 0 );
    s_axi_arvalid : in STD_LOGIC;
    s_axi_arready : out STD_LOGIC;
    s_axi_rid : out STD_LOGIC_VECTOR ( 0 to 0 );
    s_axi_rdata : out STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_rresp : out STD_LOGIC_VECTOR ( 1 downto 0 );
    s_axi_rlast : out STD_LOGIC;
    s_axi_rvalid : out STD_LOGIC;
    s_axi_rready : in STD_LOGIC;
    avm_address : out STD_LOGIC_VECTOR ( 31 downto 0 );
    avm_write : out STD_LOGIC;
    avm_read : out STD_LOGIC;
    avm_byteenable : out STD_LOGIC_VECTOR ( 3 downto 0 );
    avm_writedata : out STD_LOGIC_VECTOR ( 31 downto 0 );
    avm_readdata : in STD_LOGIC_VECTOR ( 31 downto 0 );
    avm_resp : in STD_LOGIC_VECTOR ( 1 downto 0 );
    avm_readdatavalid : in STD_LOGIC;
    avm_burstcount : out STD_LOGIC_VECTOR ( 0 to 0 );
    avm_beginbursttransfer : out STD_LOGIC;
    avm_writeresponsevalid : in STD_LOGIC;
    avm_waitrequest : in STD_LOGIC
  );
  attribute All_zero : string;
  attribute All_zero of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is "1'b0";
  attribute C_ADDRESS_MODE : integer;
  attribute C_ADDRESS_MODE of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 0;
  attribute C_AVM_BURST_WIDTH : integer;
  attribute C_AVM_BURST_WIDTH of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 1;
  attribute C_AXI_LOCK_WIDTH : integer;
  attribute C_AXI_LOCK_WIDTH of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 1;
  attribute C_BASE1_ADDR : string;
  attribute C_BASE1_ADDR of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is "64'b0000000000000000000000000000000000000000000000000000000000000000";
  attribute C_BASE2_ADDR : string;
  attribute C_BASE2_ADDR of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is "64'b0000000000000000000000000000000000000000000000000000000000000100";
  attribute C_BASE3_ADDR : string;
  attribute C_BASE3_ADDR of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is "64'b0000000000000000000000000000000000000000000000000000000000001000";
  attribute C_BASE4_ADDR : string;
  attribute C_BASE4_ADDR of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is "64'b0000000000000000000000000000000000000000000000000000000000001100";
  attribute C_BURST_LENGTH : integer;
  attribute C_BURST_LENGTH of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 1;
  attribute C_DPHASE_TIMEOUT : integer;
  attribute C_DPHASE_TIMEOUT of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 256;
  attribute C_FAMILY : string;
  attribute C_FAMILY of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is "zynq";
  attribute C_FIXED_READ_WAIT : integer;
  attribute C_FIXED_READ_WAIT of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 1;
  attribute C_FIXED_WRITE_WAIT : integer;
  attribute C_FIXED_WRITE_WAIT of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 1;
  attribute C_HAS_FIXED_READ_LATENCY : integer;
  attribute C_HAS_FIXED_READ_LATENCY of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 1;
  attribute C_HAS_FIXED_WAIT : integer;
  attribute C_HAS_FIXED_WAIT of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 1;
  attribute C_HAS_RESPONSE : integer;
  attribute C_HAS_RESPONSE of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 0;
  attribute C_HIGH1_ADDR : string;
  attribute C_HIGH1_ADDR of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is "64'b0000000000000000000000000000000000000000000000000000000000000011";
  attribute C_HIGH2_ADDR : string;
  attribute C_HIGH2_ADDR of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is "64'b0000000000000000000000000000000000000000000000000000000000000101";
  attribute C_HIGH3_ADDR : string;
  attribute C_HIGH3_ADDR of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is "64'b0000000000000000000000000000000000000000000000000000000000001001";
  attribute C_HIGH4_ADDR : string;
  attribute C_HIGH4_ADDR of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is "64'b0000000000000000000000000000000000000000000000000000000000001111";
  attribute C_NUM_ADDRESS_RANGES : integer;
  attribute C_NUM_ADDRESS_RANGES of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 0;
  attribute C_NUM_OUTSTANDING : integer;
  attribute C_NUM_OUTSTANDING of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 2;
  attribute C_PROTOCOL : integer;
  attribute C_PROTOCOL of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 0;
  attribute C_READ_LATENCY : integer;
  attribute C_READ_LATENCY of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 1;
  attribute C_S_AXI_ADDR_WIDTH : integer;
  attribute C_S_AXI_ADDR_WIDTH of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 32;
  attribute C_S_AXI_DATA_WIDTH : integer;
  attribute C_S_AXI_DATA_WIDTH of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 32;
  attribute C_S_AXI_ID_WIDTH : integer;
  attribute C_S_AXI_ID_WIDTH of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 1;
  attribute C_USE_WSTRB : integer;
  attribute C_USE_WSTRB of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is 0;
  attribute DowngradeIPIdentifiedWarnings : string;
  attribute DowngradeIPIdentifiedWarnings of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is "yes";
  attribute ORIG_REF_NAME : string;
  attribute ORIG_REF_NAME of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top : entity is "axi_amm_bridge_v1_0_17_top";
end design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top;

architecture STRUCTURE of design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top is
  signal \<const0>\ : STD_LOGIC;
  signal \^s_axi_bresp\ : STD_LOGIC_VECTOR ( 1 to 1 );
  signal \^s_axi_rresp\ : STD_LOGIC_VECTOR ( 1 to 1 );
begin
  avm_beginbursttransfer <= \<const0>\;
  avm_burstcount(0) <= \<const0>\;
  avm_byteenable(3) <= \<const0>\;
  avm_byteenable(2) <= \<const0>\;
  avm_byteenable(1) <= \<const0>\;
  avm_byteenable(0) <= \<const0>\;
  s_axi_bid(0) <= \<const0>\;
  s_axi_bresp(1) <= \^s_axi_bresp\(1);
  s_axi_bresp(0) <= \<const0>\;
  s_axi_rid(0) <= \<const0>\;
  s_axi_rlast <= \<const0>\;
  s_axi_rresp(1) <= \^s_axi_rresp\(1);
  s_axi_rresp(0) <= \<const0>\;
\AXI_LITE.I_AVA_MASTER_LITE\: entity work.design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_lite
     port map (
      avm_address(31 downto 0) => avm_address(31 downto 0),
      avm_read => avm_read,
      avm_readdata(31 downto 0) => avm_readdata(31 downto 0),
      avm_write => avm_write,
      avm_writedata(31 downto 0) => avm_writedata(31 downto 0),
      s_axi_aclk => s_axi_aclk,
      s_axi_araddr(31 downto 0) => s_axi_araddr(31 downto 0),
      s_axi_aresetn => s_axi_aresetn,
      s_axi_arready => s_axi_arready,
      s_axi_arvalid => s_axi_arvalid,
      s_axi_awaddr(31 downto 0) => s_axi_awaddr(31 downto 0),
      s_axi_awready => s_axi_awready,
      s_axi_awvalid => s_axi_awvalid,
      s_axi_bready => s_axi_bready,
      s_axi_bresp(0) => \^s_axi_bresp\(1),
      s_axi_bvalid_reg_0 => s_axi_bvalid,
      s_axi_rdata(31 downto 0) => s_axi_rdata(31 downto 0),
      s_axi_rready => s_axi_rready,
      s_axi_rresp(0) => \^s_axi_rresp\(1),
      s_axi_rvalid_reg_0 => s_axi_rvalid,
      s_axi_wdata(31 downto 0) => s_axi_wdata(31 downto 0),
      s_axi_wready => s_axi_wready,
      s_axi_wvalid => s_axi_wvalid
    );
GND: unisim.vcomponents.GND
     port map (
      G => \<const0>\
    );
end STRUCTURE;
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity design_1_axi_amm_bridge_0_0 is
  port (
    s_axi_aclk : in STD_LOGIC;
    s_axi_aresetn : in STD_LOGIC;
    s_axi_awaddr : in STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_awvalid : in STD_LOGIC;
    s_axi_awready : out STD_LOGIC;
    s_axi_wdata : in STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_wstrb : in STD_LOGIC_VECTOR ( 3 downto 0 );
    s_axi_wvalid : in STD_LOGIC;
    s_axi_wready : out STD_LOGIC;
    s_axi_bresp : out STD_LOGIC_VECTOR ( 1 downto 0 );
    s_axi_bvalid : out STD_LOGIC;
    s_axi_bready : in STD_LOGIC;
    s_axi_araddr : in STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_arvalid : in STD_LOGIC;
    s_axi_arready : out STD_LOGIC;
    s_axi_rdata : out STD_LOGIC_VECTOR ( 31 downto 0 );
    s_axi_rresp : out STD_LOGIC_VECTOR ( 1 downto 0 );
    s_axi_rvalid : out STD_LOGIC;
    s_axi_rready : in STD_LOGIC;
    avm_address : out STD_LOGIC_VECTOR ( 31 downto 0 );
    avm_write : out STD_LOGIC;
    avm_read : out STD_LOGIC;
    avm_writedata : out STD_LOGIC_VECTOR ( 31 downto 0 );
    avm_readdata : in STD_LOGIC_VECTOR ( 31 downto 0 )
  );
  attribute NotValidForBitStream : boolean;
  attribute NotValidForBitStream of design_1_axi_amm_bridge_0_0 : entity is true;
  attribute CHECK_LICENSE_TYPE : string;
  attribute CHECK_LICENSE_TYPE of design_1_axi_amm_bridge_0_0 : entity is "design_1_axi_amm_bridge_0_0,axi_amm_bridge_v1_0_17_top,{}";
  attribute DowngradeIPIdentifiedWarnings : string;
  attribute DowngradeIPIdentifiedWarnings of design_1_axi_amm_bridge_0_0 : entity is "yes";
  attribute X_CORE_INFO : string;
  attribute X_CORE_INFO of design_1_axi_amm_bridge_0_0 : entity is "axi_amm_bridge_v1_0_17_top,Vivado 2022.2";
end design_1_axi_amm_bridge_0_0;

architecture STRUCTURE of design_1_axi_amm_bridge_0_0 is
  signal \<const0>\ : STD_LOGIC;
  signal \^s_axi_bresp\ : STD_LOGIC_VECTOR ( 1 to 1 );
  signal \^s_axi_rresp\ : STD_LOGIC_VECTOR ( 1 to 1 );
  signal NLW_inst_avm_beginbursttransfer_UNCONNECTED : STD_LOGIC;
  signal NLW_inst_s_axi_rlast_UNCONNECTED : STD_LOGIC;
  signal NLW_inst_avm_burstcount_UNCONNECTED : STD_LOGIC_VECTOR ( 0 to 0 );
  signal NLW_inst_avm_byteenable_UNCONNECTED : STD_LOGIC_VECTOR ( 3 downto 0 );
  signal NLW_inst_s_axi_bid_UNCONNECTED : STD_LOGIC_VECTOR ( 0 to 0 );
  signal NLW_inst_s_axi_bresp_UNCONNECTED : STD_LOGIC_VECTOR ( 0 to 0 );
  signal NLW_inst_s_axi_rid_UNCONNECTED : STD_LOGIC_VECTOR ( 0 to 0 );
  signal NLW_inst_s_axi_rresp_UNCONNECTED : STD_LOGIC_VECTOR ( 0 to 0 );
  attribute All_zero : string;
  attribute All_zero of inst : label is "1'b0";
  attribute C_ADDRESS_MODE : integer;
  attribute C_ADDRESS_MODE of inst : label is 0;
  attribute C_AVM_BURST_WIDTH : integer;
  attribute C_AVM_BURST_WIDTH of inst : label is 1;
  attribute C_AXI_LOCK_WIDTH : integer;
  attribute C_AXI_LOCK_WIDTH of inst : label is 1;
  attribute C_BASE1_ADDR : string;
  attribute C_BASE1_ADDR of inst : label is "64'b0000000000000000000000000000000000000000000000000000000000000000";
  attribute C_BASE2_ADDR : string;
  attribute C_BASE2_ADDR of inst : label is "64'b0000000000000000000000000000000000000000000000000000000000000100";
  attribute C_BASE3_ADDR : string;
  attribute C_BASE3_ADDR of inst : label is "64'b0000000000000000000000000000000000000000000000000000000000001000";
  attribute C_BASE4_ADDR : string;
  attribute C_BASE4_ADDR of inst : label is "64'b0000000000000000000000000000000000000000000000000000000000001100";
  attribute C_BURST_LENGTH : integer;
  attribute C_BURST_LENGTH of inst : label is 1;
  attribute C_DPHASE_TIMEOUT : integer;
  attribute C_DPHASE_TIMEOUT of inst : label is 256;
  attribute C_FAMILY : string;
  attribute C_FAMILY of inst : label is "zynq";
  attribute C_FIXED_READ_WAIT : integer;
  attribute C_FIXED_READ_WAIT of inst : label is 1;
  attribute C_FIXED_WRITE_WAIT : integer;
  attribute C_FIXED_WRITE_WAIT of inst : label is 1;
  attribute C_HAS_FIXED_READ_LATENCY : integer;
  attribute C_HAS_FIXED_READ_LATENCY of inst : label is 1;
  attribute C_HAS_FIXED_WAIT : integer;
  attribute C_HAS_FIXED_WAIT of inst : label is 1;
  attribute C_HAS_RESPONSE : integer;
  attribute C_HAS_RESPONSE of inst : label is 0;
  attribute C_HIGH1_ADDR : string;
  attribute C_HIGH1_ADDR of inst : label is "64'b0000000000000000000000000000000000000000000000000000000000000011";
  attribute C_HIGH2_ADDR : string;
  attribute C_HIGH2_ADDR of inst : label is "64'b0000000000000000000000000000000000000000000000000000000000000101";
  attribute C_HIGH3_ADDR : string;
  attribute C_HIGH3_ADDR of inst : label is "64'b0000000000000000000000000000000000000000000000000000000000001001";
  attribute C_HIGH4_ADDR : string;
  attribute C_HIGH4_ADDR of inst : label is "64'b0000000000000000000000000000000000000000000000000000000000001111";
  attribute C_NUM_ADDRESS_RANGES : integer;
  attribute C_NUM_ADDRESS_RANGES of inst : label is 0;
  attribute C_NUM_OUTSTANDING : integer;
  attribute C_NUM_OUTSTANDING of inst : label is 2;
  attribute C_PROTOCOL : integer;
  attribute C_PROTOCOL of inst : label is 0;
  attribute C_READ_LATENCY : integer;
  attribute C_READ_LATENCY of inst : label is 1;
  attribute C_S_AXI_ADDR_WIDTH : integer;
  attribute C_S_AXI_ADDR_WIDTH of inst : label is 32;
  attribute C_S_AXI_DATA_WIDTH : integer;
  attribute C_S_AXI_DATA_WIDTH of inst : label is 32;
  attribute C_S_AXI_ID_WIDTH : integer;
  attribute C_S_AXI_ID_WIDTH of inst : label is 1;
  attribute C_USE_WSTRB : integer;
  attribute C_USE_WSTRB of inst : label is 0;
  attribute DowngradeIPIdentifiedWarnings of inst : label is "yes";
  attribute X_INTERFACE_INFO : string;
  attribute X_INTERFACE_INFO of avm_read : signal is "xilinx.com:interface:avalon:1.0 M_AVALON READ";
  attribute X_INTERFACE_INFO of avm_write : signal is "xilinx.com:interface:avalon:1.0 M_AVALON WRITE";
  attribute X_INTERFACE_INFO of s_axi_aclk : signal is "xilinx.com:signal:clock:1.0 s_axi_aclk CLK";
  attribute X_INTERFACE_PARAMETER : string;
  attribute X_INTERFACE_PARAMETER of s_axi_aclk : signal is "XIL_INTERFACENAME s_axi_aclk, ASSOCIATED_BUSIF S_AXI_LITE:S_AXI_FULL:M_AVALON, ASSOCIATED_RESET s_axi_aresetn, FREQ_HZ 100000000, FREQ_TOLERANCE_HZ 0, PHASE 0.0, CLK_DOMAIN design_1_processing_system7_0_0_FCLK_CLK0, INSERT_VIP 0";
  attribute X_INTERFACE_INFO of s_axi_aresetn : signal is "xilinx.com:signal:reset:1.0 s_axi_aresetn RST";
  attribute X_INTERFACE_PARAMETER of s_axi_aresetn : signal is "XIL_INTERFACENAME s_axi_aresetn, POLARITY ACTIVE_LOW, INSERT_VIP 0";
  attribute X_INTERFACE_INFO of s_axi_arready : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE ARREADY";
  attribute X_INTERFACE_INFO of s_axi_arvalid : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE ARVALID";
  attribute X_INTERFACE_INFO of s_axi_awready : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE AWREADY";
  attribute X_INTERFACE_INFO of s_axi_awvalid : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE AWVALID";
  attribute X_INTERFACE_INFO of s_axi_bready : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE BREADY";
  attribute X_INTERFACE_INFO of s_axi_bvalid : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE BVALID";
  attribute X_INTERFACE_INFO of s_axi_rready : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE RREADY";
  attribute X_INTERFACE_PARAMETER of s_axi_rready : signal is "XIL_INTERFACENAME S_AXI_LITE, DATA_WIDTH 32, PROTOCOL AXI4LITE, FREQ_HZ 100000000, ID_WIDTH 0, ADDR_WIDTH 32, AWUSER_WIDTH 0, ARUSER_WIDTH 0, WUSER_WIDTH 0, RUSER_WIDTH 0, BUSER_WIDTH 0, READ_WRITE_MODE READ_WRITE, HAS_BURST 0, HAS_LOCK 0, HAS_PROT 0, HAS_CACHE 0, HAS_QOS 0, HAS_REGION 0, HAS_WSTRB 1, HAS_BRESP 1, HAS_RRESP 1, SUPPORTS_NARROW_BURST 0, NUM_READ_OUTSTANDING 8, NUM_WRITE_OUTSTANDING 8, MAX_BURST_LENGTH 1, PHASE 0.0, CLK_DOMAIN design_1_processing_system7_0_0_FCLK_CLK0, NUM_READ_THREADS 4, NUM_WRITE_THREADS 4, RUSER_BITS_PER_BYTE 0, WUSER_BITS_PER_BYTE 0, INSERT_VIP 0";
  attribute X_INTERFACE_INFO of s_axi_rvalid : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE RVALID";
  attribute X_INTERFACE_INFO of s_axi_wready : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE WREADY";
  attribute X_INTERFACE_INFO of s_axi_wvalid : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE WVALID";
  attribute X_INTERFACE_INFO of avm_address : signal is "xilinx.com:interface:avalon:1.0 M_AVALON ADDRESS";
  attribute X_INTERFACE_INFO of avm_readdata : signal is "xilinx.com:interface:avalon:1.0 M_AVALON READDATA";
  attribute X_INTERFACE_PARAMETER of avm_readdata : signal is "XIL_INTERFACENAME M_AVALON, ADDR_WIDTH 32";
  attribute X_INTERFACE_INFO of avm_writedata : signal is "xilinx.com:interface:avalon:1.0 M_AVALON WRITEDATA";
  attribute X_INTERFACE_INFO of s_axi_araddr : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE ARADDR";
  attribute X_INTERFACE_INFO of s_axi_awaddr : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE AWADDR";
  attribute X_INTERFACE_INFO of s_axi_bresp : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE BRESP";
  attribute X_INTERFACE_INFO of s_axi_rdata : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE RDATA";
  attribute X_INTERFACE_INFO of s_axi_rresp : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE RRESP";
  attribute X_INTERFACE_INFO of s_axi_wdata : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE WDATA";
  attribute X_INTERFACE_INFO of s_axi_wstrb : signal is "xilinx.com:interface:aximm:1.0 S_AXI_LITE WSTRB";
begin
  s_axi_bresp(1) <= \^s_axi_bresp\(1);
  s_axi_bresp(0) <= \<const0>\;
  s_axi_rresp(1) <= \^s_axi_rresp\(1);
  s_axi_rresp(0) <= \<const0>\;
GND: unisim.vcomponents.GND
     port map (
      G => \<const0>\
    );
inst: entity work.design_1_axi_amm_bridge_0_0_axi_amm_bridge_v1_0_17_top
     port map (
      avm_address(31 downto 0) => avm_address(31 downto 0),
      avm_beginbursttransfer => NLW_inst_avm_beginbursttransfer_UNCONNECTED,
      avm_burstcount(0) => NLW_inst_avm_burstcount_UNCONNECTED(0),
      avm_byteenable(3 downto 0) => NLW_inst_avm_byteenable_UNCONNECTED(3 downto 0),
      avm_read => avm_read,
      avm_readdata(31 downto 0) => avm_readdata(31 downto 0),
      avm_readdatavalid => '0',
      avm_resp(1 downto 0) => B"00",
      avm_waitrequest => '0',
      avm_write => avm_write,
      avm_writedata(31 downto 0) => avm_writedata(31 downto 0),
      avm_writeresponsevalid => '0',
      s_axi_aclk => s_axi_aclk,
      s_axi_araddr(31 downto 0) => s_axi_araddr(31 downto 0),
      s_axi_arburst(1 downto 0) => B"00",
      s_axi_aresetn => s_axi_aresetn,
      s_axi_arid(0) => '0',
      s_axi_arlen(7 downto 0) => B"00000000",
      s_axi_arready => s_axi_arready,
      s_axi_arsize(2 downto 0) => B"000",
      s_axi_arvalid => s_axi_arvalid,
      s_axi_awaddr(31 downto 0) => s_axi_awaddr(31 downto 0),
      s_axi_awburst(1 downto 0) => B"00",
      s_axi_awid(0) => '0',
      s_axi_awlen(7 downto 0) => B"00000000",
      s_axi_awready => s_axi_awready,
      s_axi_awsize(2 downto 0) => B"000",
      s_axi_awvalid => s_axi_awvalid,
      s_axi_bid(0) => NLW_inst_s_axi_bid_UNCONNECTED(0),
      s_axi_bready => s_axi_bready,
      s_axi_bresp(1) => \^s_axi_bresp\(1),
      s_axi_bresp(0) => NLW_inst_s_axi_bresp_UNCONNECTED(0),
      s_axi_bvalid => s_axi_bvalid,
      s_axi_rdata(31 downto 0) => s_axi_rdata(31 downto 0),
      s_axi_rid(0) => NLW_inst_s_axi_rid_UNCONNECTED(0),
      s_axi_rlast => NLW_inst_s_axi_rlast_UNCONNECTED,
      s_axi_rready => s_axi_rready,
      s_axi_rresp(1) => \^s_axi_rresp\(1),
      s_axi_rresp(0) => NLW_inst_s_axi_rresp_UNCONNECTED(0),
      s_axi_rvalid => s_axi_rvalid,
      s_axi_wdata(31 downto 0) => s_axi_wdata(31 downto 0),
      s_axi_wlast => '0',
      s_axi_wready => s_axi_wready,
      s_axi_wstrb(3 downto 0) => B"0000",
      s_axi_wvalid => s_axi_wvalid
    );
end STRUCTURE;
