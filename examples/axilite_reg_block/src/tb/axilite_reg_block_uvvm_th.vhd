-- File: axilite_reg_block_uvvm_th.vhd
-- Description: Testharness for axilite_reg_block

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.env.all;

library uvvm_util;
context uvvm_util.uvvm_util_context;

library uvvm_vvc_framework;
use uvvm_vvc_framework.ti_vvc_framework_support_pkg.all;

library uvvm_cosim_lib;
use uvvm_cosim_lib.all;

library bitvis_vip_clock_generator;
context bitvis_vip_clock_generator.vvc_context;

library bitvis_vip_axilite;
context bitvis_vip_axilite.vvc_context;

entity axilite_reg_block_uvvm_th is
  generic (
    GC_AXILITE_VVC_IDX : natural;
    GC_DUT_ADDR_WIDTH  : natural;
    GC_COSIM_ENABLE    : boolean);
  port (
    arst            : in std_logic;
    vvc_config_done : in std_logic
  );
end entity axilite_reg_block_uvvm_th;

architecture th of axilite_reg_block_uvvm_th is

  constant C_VVC_ADDR_WIDTH : natural := 32;
  constant C_DATA_WIDTH     : natural := 32;

  signal clk       : std_logic;
  signal clk_cosim : std_logic;

  signal axilite_if : t_axilite_if(
    write_address_channel(awaddr(C_VVC_ADDR_WIDTH-1 downto 0)),
    write_data_channel(wdata(C_DATA_WIDTH-1 downto 0),
                       wstrb(C_DATA_WIDTH/8-1 downto 0)),
    read_address_channel(araddr(C_VVC_ADDR_WIDTH-1 downto 0)),
    read_data_channel(rdata(C_DATA_WIDTH-1 downto 0)));

begin

  i_ti_uvvm_engine : entity uvvm_vvc_framework.ti_uvvm_engine;

  i_dut : entity work.axilite_reg_block
    generic map (
      GC_ADDR_WIDTH => GC_DUT_ADDR_WIDTH,
      GC_DATA_WIDTH => C_DATA_WIDTH)
    port map (
      clk => clk,
      rst => arst,

      s_axi_awaddr  => axilite_if.write_address_channel.awaddr(GC_DUT_ADDR_WIDTH-1 downto 0),
      s_axi_awprot  => axilite_if.write_address_channel.awprot,
      s_axi_awvalid => axilite_if.write_address_channel.awvalid,
      s_axi_awready => axilite_if.write_address_channel.awready,

      s_axi_wdata  => axilite_if.write_data_channel.wdata,
      s_axi_wstrb  => axilite_if.write_data_channel.wstrb,
      s_axi_wvalid => axilite_if.write_data_channel.wvalid,
      s_axi_wready => axilite_if.write_data_channel.wready,

      s_axi_bresp  => axilite_if.write_response_channel.bresp,
      s_axi_bvalid => axilite_if.write_response_channel.bvalid,
      s_axi_bready => axilite_if.write_response_channel.bready,

      s_axi_araddr  => axilite_if.read_address_channel.araddr(GC_DUT_ADDR_WIDTH-1 downto 0),
      s_axi_arprot  => axilite_if.read_address_channel.arprot,
      s_axi_arvalid => axilite_if.read_address_channel.arvalid,
      s_axi_arready => axilite_if.read_address_channel.arready,

      s_axi_rdata  => axilite_if.read_data_channel.rdata,
      s_axi_rresp  => axilite_if.read_data_channel.rresp,
      s_axi_rvalid => axilite_if.read_data_channel.rvalid,
      s_axi_rready => axilite_if.read_data_channel.rready);

  i_uvvm_cosim : entity uvvm_cosim_lib.uvvm_cosim
    generic map (
      GC_COSIM_EN => GC_COSIM_ENABLE)
    port map (
      clk             => clk_cosim,
      vvc_config_done => vvc_config_done);

  i_axilite_vvc : entity bitvis_vip_axilite.axilite_vvc
    generic map (
      GC_ADDR_WIDTH   => C_VVC_ADDR_WIDTH,
      GC_DATA_WIDTH   => C_DATA_WIDTH,
      GC_INSTANCE_IDX => GC_AXILITE_VVC_IDX
    )
    port map (
      clk                   => clk,
      axilite_vvc_master_if => axilite_if);

  -- Clock generator 50 MHz
  i1_clock_generator_vvc : entity bitvis_vip_clock_generator.clock_generator_vvc
    generic map (
      GC_INSTANCE_IDX    => 0,
      GC_CLOCK_NAME      => "Clock 1",
      GC_CLOCK_PERIOD    => 20 ns,
      GC_CLOCK_HIGH_TIME => 10 ns
    )
    port map (
      clk => clk
    );

  -- Clock generator 1 MHz for cosim
  i2_clock_generator_vvc : entity bitvis_vip_clock_generator.clock_generator_vvc
    generic map (
      GC_INSTANCE_IDX    => 1,
      GC_CLOCK_NAME      => "Clock cosim",
      GC_CLOCK_PERIOD    => 1000 ns,
      GC_CLOCK_HIGH_TIME => 500 ns
    )
    port map (
      clk => clk_cosim
    );

end architecture th;
