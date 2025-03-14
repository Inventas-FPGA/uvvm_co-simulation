library ieee;
use ieee.std_logic_1164.all;

entity axistream_uart is
  generic (
    GC_BAUDRATE : natural;
    GC_CLK_FREQ : natural);
  port (
    clk : in  std_logic;
    rst : in  std_logic;
    rxd : in  std_logic;
    txd : out std_logic;

    -- Transmit interface
    s_axis_transmit_tdata  : in  std_logic_vector(7 downto 0);
    s_axis_transmit_tvalid : in  std_logic;
    s_axis_transmit_tready : out std_logic;

    -- Receive interface
    m_axis_receive_tdata  : out std_logic_vector(7 downto 0);
    m_axis_receive_tvalid : out std_logic;
    m_axis_receive_tready : in  std_logic);

end entity axistream_uart;

architecture rtl of axistream_uart is

begin

  inst_uart_rx: entity work.uart_rx
    generic map (
      GC_BAUDRATE => GC_BAUDRATE,
      GC_CLK_FREQ => GC_CLK_FREQ)
    port map (
      clk                   => clk,
      rst                   => rst,
      rxd                   => rxd,
      m_axis_receive_tdata  => m_axis_receive_tdata,
      m_axis_receive_tvalid => m_axis_receive_tvalid,
      m_axis_receive_tready => m_axis_receive_tready);

  inst_uart_tx: entity work.uart_tx
    generic map (
      GC_BAUDRATE => GC_BAUDRATE,
      GC_CLK_FREQ => GC_CLK_FREQ)
    port map (
      clk                    => clk,
      rst                    => rst,
      txd                    => txd,
      s_axis_transmit_tdata  => s_axis_transmit_tdata,
      s_axis_transmit_tvalid => s_axis_transmit_tvalid,
      s_axis_transmit_tready => s_axis_transmit_tready);

end architecture rtl;
