-- File: axistream_uart_uvvm_th.vhd
-- Description: Testharness for axistream_uart

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.env.all;

library uvvm_util;
context uvvm_util.uvvm_util_context;

library uvvm_vvc_framework;
use uvvm_vvc_framework.ti_vvc_framework_support_pkg.all;

library bitvis_vip_clock_generator;
context bitvis_vip_clock_generator.vvc_context;

library bitvis_vip_axistream;
context bitvis_vip_axistream.vvc_context;

library bitvis_vip_uart;
context bitvis_vip_uart.vvc_context;

entity axistream_uart_uvvm_th is
  generic (
    GC_AXIS_VVC_TRANSMIT_IDX : natural;
    GC_AXIS_VVC_RECEIVE_IDX  : natural;
    GC_UART_VVC_IDX          : natural);
  port (
    arst : in  std_logic
  );
end entity axistream_uart_uvvm_th;

architecture th of axistream_uart_uvvm_th is

  constant C_CLK_PERIOD : time    := 20 ns;
  constant C_CLK_FREQ   : natural := 50000000;
  constant C_BAUDRATE   : natural := 115200;
  
  signal clk : std_logic;
  signal dut_rxd : std_logic;
  signal dut_txd : std_logic;

  subtype t_axistream_8b is t_axistream_if(tdata(7 downto 0),
                                           tkeep(0 downto 0),
                                           tuser(0 downto 0),
                                           tstrb(0 downto 0),
                                           tid(0 downto 0),
                                           tdest(0 downto 0)
                                           );

  signal axistream_if_transmit : t_axistream_8b;
  signal axistream_if_receive  : t_axistream_8b;

begin

  -- Unused AXI-Stream signals
  axistream_if_transmit.tkeep <= (others => '1');
  axistream_if_transmit.tuser <= (others => '0');
  axistream_if_transmit.tstrb <= (others => '0');
  axistream_if_transmit.tid   <= (others => '0');
  axistream_if_transmit.tdest <= (others => '0');

  axistream_if_receive.tkeep <= (others => '1');
  axistream_if_receive.tuser <= (others => '0');
  axistream_if_receive.tstrb <= (others => '0');
  axistream_if_receive.tid   <= (others => '0');
  axistream_if_receive.tdest <= (others => '0');

  i_ti_uvvm_engine : entity uvvm_vvc_framework.ti_uvvm_engine;

  i_dut : entity work.axistream_uart
    generic map (
      GC_BAUDRATE => C_BAUDRATE,
      GC_CLK_FREQ => C_CLK_FREQ)
    port map (
      clk                    => clk,
      rst                    => arst,
      rxd                    => dut_rxd,
      txd                    => dut_txd,
      s_axis_transmit_tdata  => axistream_if_transmit.tdata,
      s_axis_transmit_tvalid => axistream_if_transmit.tvalid,
      s_axis_transmit_tready => axistream_if_transmit.tready,
      m_axis_receive_tdata   => axistream_if_receive.tdata,
      m_axis_receive_tvalid  => axistream_if_receive.tvalid,
      m_axis_receive_tready  => axistream_if_receive.tready);

  i_axistream_vvc_transmit : entity bitvis_vip_axistream.axistream_vvc
    generic map (
      GC_VVC_IS_MASTER => true,
      GC_DATA_WIDTH    => 8,
      GC_USER_WIDTH    => 1,
      GC_ID_WIDTH      => 1,
      GC_DEST_WIDTH    => 1,
      GC_INSTANCE_IDX  => GC_AXIS_VVC_TRANSMIT_IDX
    )
    port map (
      clk              => clk,
      axistream_vvc_if => axistream_if_transmit
      );

  i_axistream_vvc_receive : entity bitvis_vip_axistream.axistream_vvc
    generic map (
      GC_VVC_IS_MASTER => false,
      GC_DATA_WIDTH    => 8,
      GC_USER_WIDTH    => 1,
      GC_ID_WIDTH      => 1,
      GC_DEST_WIDTH    => 1,
      GC_INSTANCE_IDX  => GC_AXIS_VVC_RECEIVE_IDX
    )
    port map (
      clk              => clk,
      axistream_vvc_if => axistream_if_receive
      );

  i_uart_vvc : entity bitvis_vip_uart.uart_vvc
    generic map (
      GC_DATA_WIDTH   => 8,
      GC_INSTANCE_IDX => GC_UART_VVC_IDX)
    port map (
      uart_vvc_rx => dut_txd,
      uart_vvc_tx => dut_rxd);

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

end architecture th;
