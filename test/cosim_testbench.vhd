--hdlregression:tb
library ieee;
use ieee.std_logic_1164.all;

library uvvm_util;
context uvvm_util.uvvm_util_context;

library uvvm_vvc_framework;
use uvvm_vvc_framework.ti_vvc_framework_support_pkg.all;

library uvvm_cosim_lib;
use uvvm_cosim_lib.all;

library bitvis_vip_uart;
context bitvis_vip_uart.vvc_context;

library bitvis_vip_axistream;
context bitvis_vip_axistream.vvc_context;

library bitvis_vip_clock_generator;
context bitvis_vip_clock_generator.vvc_context;

entity tb is
end entity tb;

architecture sim of tb is

  signal clk             : std_logic := '0';
  signal vvc_config_done : std_logic := '0';

  signal uart0_rx : std_logic;
  signal uart0_tx : std_logic := '1';
  signal uart1_rx : std_logic;
  signal uart1_tx : std_logic := '1';

  constant C_CLK_PERIOD       : time    := 20 ns;
  constant C_CLK_FREQ         : natural := 50000000;
  constant C_BAUDRATE         : natural := 1000000;

  subtype t_axistream_8b is t_axistream_if(tdata(7 downto 0),
                                           tkeep(0 downto 0),
                                           tuser(0 downto 0),
                                           tstrb(0 downto 0),
                                           tid(0 downto 0),
                                           tdest(0 downto 0)
                                           );

  -- Interfaces used between non-packet based AXI-Stream VVCs
  signal axistream_if_byte_transmit : t_axistream_8b;
  signal axistream_if_byte_receive  : t_axistream_8b;

  -- Interfaces used between packet based AXI-Stream VVCs
  signal axistream_if_packet_transmit : t_axistream_8b;
  signal axistream_if_packet_receive  : t_axistream_8b;

begin

  uart0_rx <= uart1_tx after 10 ns;
  uart1_rx <= uart0_tx after 10 ns;

  -- Unused AXI-Stream signals
  axistream_if_byte_transmit.tkeep <= (others => '1');
  axistream_if_byte_transmit.tuser <= (others => '0');
  axistream_if_byte_transmit.tstrb <= (others => '0');
  axistream_if_byte_transmit.tid   <= (others => '0');
  axistream_if_byte_transmit.tdest <= (others => '0');

  axistream_if_byte_receive.tkeep <= (others => '1');
  axistream_if_byte_receive.tuser <= (others => '0');
  axistream_if_byte_receive.tstrb <= (others => '0');
  axistream_if_byte_receive.tid   <= (others => '0');
  axistream_if_byte_receive.tdest <= (others => '0');

  axistream_if_byte_receive.tdata   <= axistream_if_byte_transmit.tdata;
  axistream_if_byte_receive.tvalid  <= axistream_if_byte_transmit.tvalid;
  axistream_if_byte_receive.tlast   <= axistream_if_byte_transmit.tlast;
  axistream_if_byte_transmit.tready <= axistream_if_byte_receive.tready;

  axistream_if_packet_transmit.tkeep <= (others => '1');
  axistream_if_packet_transmit.tuser <= (others => '0');
  axistream_if_packet_transmit.tstrb <= (others => '0');
  axistream_if_packet_transmit.tid   <= (others => '0');
  axistream_if_packet_transmit.tdest <= (others => '0');

  axistream_if_packet_receive.tkeep <= (others => '1');
  axistream_if_packet_receive.tuser <= (others => '0');
  axistream_if_packet_receive.tstrb <= (others => '0');
  axistream_if_packet_receive.tid   <= (others => '0');
  axistream_if_packet_receive.tdest <= (others => '0');

  axistream_if_packet_receive.tdata   <= axistream_if_packet_transmit.tdata;
  axistream_if_packet_receive.tvalid  <= axistream_if_packet_transmit.tvalid;
  axistream_if_packet_receive.tlast   <= axistream_if_packet_transmit.tlast;
  axistream_if_packet_transmit.tready <= axistream_if_packet_receive.tready;

  inst_uvvm_cosim: entity uvvm_cosim_lib.uvvm_cosim
    generic map (
      GC_COSIM_EN => true)
    port map (
      clk             => clk,
      vvc_config_done => vvc_config_done);

  inst_ti_uvvm_engine : entity uvvm_vvc_framework.ti_uvvm_engine;

  inst_clk_vvc : entity bitvis_vip_clock_generator.clock_generator_vvc
    generic map (
      GC_INSTANCE_IDX    => 0,
      GC_CLOCK_NAME      => "Clock",
      GC_CLOCK_PERIOD    => C_CLK_PERIOD,
      GC_CLOCK_HIGH_TIME => C_CLK_PERIOD/2
      )
    port map (
      clk => clk
      );

  inst_uart0_vvc : entity bitvis_vip_uart.uart_vvc
    generic map (
      GC_DATA_WIDTH   => 8,
      GC_INSTANCE_IDX => 0)
    port map (
      uart_vvc_rx => uart0_rx,
      uart_vvc_tx => uart0_tx);

  inst_uart1_vvc : entity bitvis_vip_uart.uart_vvc
    generic map (
      GC_DATA_WIDTH   => 8,
      GC_INSTANCE_IDX => 1)
    port map (
      uart_vvc_rx => uart1_rx,
      uart_vvc_tx => uart1_tx);

  i_axistream_vvc0_byte_transmit : entity bitvis_vip_axistream.axistream_vvc
    generic map (
      GC_VVC_IS_MASTER => true,
      GC_DATA_WIDTH    => 8,
      GC_USER_WIDTH    => 1,
      GC_ID_WIDTH      => 1,
      GC_DEST_WIDTH    => 1,
      GC_INSTANCE_IDX  => 0
      )
    port map (
      clk              => clk,
      axistream_vvc_if => axistream_if_byte_transmit
      );

  i_axistream_vvc1_byte_receive : entity bitvis_vip_axistream.axistream_vvc
    generic map (
      GC_VVC_IS_MASTER => false,
      GC_DATA_WIDTH    => 8,
      GC_USER_WIDTH    => 1,
      GC_ID_WIDTH      => 1,
      GC_DEST_WIDTH    => 1,
      GC_INSTANCE_IDX  => 1
      )
    port map (
      clk              => clk,
      axistream_vvc_if => axistream_if_byte_receive
      );

  i_axistream_vvc2_packet_transmit : entity bitvis_vip_axistream.axistream_vvc
    generic map (
      GC_VVC_IS_MASTER => true,
      GC_DATA_WIDTH    => 8,
      GC_USER_WIDTH    => 1,
      GC_ID_WIDTH      => 1,
      GC_DEST_WIDTH    => 1,
      GC_INSTANCE_IDX  => 2
      )
    port map (
      clk              => clk,
      axistream_vvc_if => axistream_if_packet_transmit
      );

  i_axistream_vvc3_packet_receive : entity bitvis_vip_axistream.axistream_vvc
    generic map (
      GC_VVC_IS_MASTER => false,
      GC_DATA_WIDTH    => 8,
      GC_USER_WIDTH    => 1,
      GC_ID_WIDTH      => 1,
      GC_DEST_WIDTH    => 1,
      GC_INSTANCE_IDX  => 3
      )
    port map (
      clk              => clk,
      axistream_vvc_if => axistream_if_packet_receive
      );


  p_test : process
    variable v_uart_bfm_config      : t_uart_bfm_config      := C_UART_BFM_CONFIG_DEFAULT;
    variable v_axistream_bfm_config : t_axistream_bfm_config := C_AXISTREAM_BFM_CONFIG_DEFAULT;
  begin
    -----------------------------------------------------------------------------
    -- Wait for UVVM to finish initialization
    -----------------------------------------------------------------------------
    await_uvvm_initialization(VOID);

    -----------------------------------------------------------------------------
    -- Set UVVM verbosity level
    -----------------------------------------------------------------------------
    disable_log_msg(ALL_MESSAGES);
    enable_log_msg(ID_SEQUENCER);
    enable_log_msg(ID_LOG_HDR);
    enable_log_msg(ID_VVC_ACTIVITY);

    disable_log_msg(UART_VVCT, 0, RX, ALL_MESSAGES);
    disable_log_msg(UART_VVCT, 0, TX, ALL_MESSAGES);
    disable_log_msg(UART_VVCT, 1, RX, ALL_MESSAGES);
    disable_log_msg(UART_VVCT, 1, TX, ALL_MESSAGES);
    enable_log_msg(UART_VVCT, 0, RX, ID_BFM);
    enable_log_msg(UART_VVCT, 0, TX, ID_BFM);
    enable_log_msg(UART_VVCT, 1, RX, ID_BFM);
    enable_log_msg(UART_VVCT, 1, TX, ID_BFM);

    disable_log_msg(AXISTREAM_VVCT, 0, NA, ALL_MESSAGES);
    disable_log_msg(AXISTREAM_VVCT, 1, NA, ALL_MESSAGES);
    enable_log_msg(AXISTREAM_VVCT, 0, NA, ID_BFM);
    enable_log_msg(AXISTREAM_VVCT, 1, NA, ID_BFM);

    -----------------------------------------------------------------------------
    -- UART VVC config
    -----------------------------------------------------------------------------
    v_uart_bfm_config.parity                 := PARITY_NONE;
    v_uart_bfm_config.bit_time               := (1 sec) / C_BAUDRATE;
    shared_uart_vvc_config(RX, 0).bfm_config := v_uart_bfm_config;
    shared_uart_vvc_config(TX, 0).bfm_config := v_uart_bfm_config;
    shared_uart_vvc_config(RX, 1).bfm_config := v_uart_bfm_config;
    shared_uart_vvc_config(TX, 1).bfm_config := v_uart_bfm_config;

    -- Infinite timeout (zero) is the default - but set explicitly here anyway
    -- The cosim monitor will NOT work reliably with finite timeout
    -- because of how timeout is implemented in the BFM.
    shared_uart_vvc_config(RX, 1).bfm_config.timeout          := 0 ns;

    -- Note: Unnecessary to set NO_ALERT since we require infinite timeout
    --shared_uart_vvc_config(RX, 1).bfm_config.timeout_severity := NO_ALERT;

    -----------------------------------------------------------------------------
    -- AXI-Stream VVC config
    -----------------------------------------------------------------------------
    v_axistream_bfm_config.check_packet_length      := false;  -- Disable tlast
    v_axistream_bfm_config.clock_period             := C_CLK_PERIOD;
    v_axistream_bfm_config.ready_default_value      := '1';
    v_axistream_bfm_config.max_wait_cycles          := 100000;
    v_axistream_bfm_config.max_wait_cycles_severity := NO_ALERT;

    shared_axistream_vvc_config(0).bfm_config := v_axistream_bfm_config;
    shared_axistream_vvc_config(1).bfm_config := v_axistream_bfm_config;
    shared_axistream_vvc_config(2).bfm_config := v_axistream_bfm_config;
    shared_axistream_vvc_config(3).bfm_config := v_axistream_bfm_config;

    -- AXI-Stream VVC ID 2 and 3 are packet based
    shared_axistream_vvc_config(2).bfm_config.check_packet_length := true;
    shared_axistream_vvc_config(3).bfm_config.check_packet_length := true;

    -- Indicate to uvvm_cosim that VVCs are now configured
    vvc_config_done <= '1';

    -----------------------------------------------------------------------------
    -- Start clock
    -----------------------------------------------------------------------------
    wait for C_CLK_PERIOD;
    start_clock(CLOCK_GENERATOR_VVCT, 0, "Start clock generator");

    report "Starting test";-- Test

    -----------------------------------------------------------------------------
    -- Wait for a long time to allow cosim control
    -----------------------------------------------------------------------------
    wait for 1000 ms;
    alert(ERROR, "Simulation timeout - was not terminated from cosim", C_SCOPE);
    report_alert_counters(FINAL);
    std.env.stop;
  end process;

end architecture sim;
