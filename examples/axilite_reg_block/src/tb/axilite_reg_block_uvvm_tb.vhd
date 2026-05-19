-- File: axilite_reg_block_uvvm_tb.vhd
-- Description: Testbench for axilite_reg_block
-- hdlregression:tb

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.env.all;

library uvvm_util;
context uvvm_util.uvvm_util_context;

library uvvm_vvc_framework;
use uvvm_vvc_framework.ti_vvc_framework_support_pkg.all;

library bitvis_vip_axilite;
context bitvis_vip_axilite.vvc_context;

library bitvis_vip_clock_generator;
context bitvis_vip_clock_generator.vvc_context;

entity axilite_reg_block_uvvm_tb is
  generic (
    GC_TESTCASE : string);
end entity axilite_reg_block_uvvm_tb;

architecture tb of axilite_reg_block_uvvm_tb is

  constant C_SCOPE      : string := "AXILITE_REG_BLOCK_UVVM_TB";
  constant C_CLK_PERIOD : time   := 20 ns;

  constant C_AXILITE_VVC_IDX : natural := 0;
  constant C_DUT_ADDR_WIDTH  : natural := 8;

  constant C_NUM_SCRATCH : natural := 9;
  constant C_ID_OFFSET   : natural := 16#24#;
  constant C_ID_VALUE    : std_logic_vector(31 downto 0) := x"CAFEC0DE";

  --------------------------------------------------------------------------------
  -- Signal declarations
  --------------------------------------------------------------------------------
  signal clk             : std_logic := '0';
  signal arst            : std_logic := '0';
  signal vvc_config_done : std_logic := '0';

  function reg_addr(constant idx : natural) return unsigned is
  begin
    return to_unsigned(idx * 4, 32);
  end function reg_addr;

begin

  -----------------------------------------------------------------------------
  -- Instantiate test harness
  -----------------------------------------------------------------------------
  i_test_harness : entity work.axilite_reg_block_uvvm_th
    generic map (
      GC_AXILITE_VVC_IDX => C_AXILITE_VVC_IDX,
      GC_DUT_ADDR_WIDTH  => C_DUT_ADDR_WIDTH,
      GC_COSIM_ENABLE    => (GC_TESTCASE = "TC_COSIM"))
    port map (
      arst            => arst,
      vvc_config_done => vvc_config_done
      );

  ------------------------------------------------
  -- Sequencer process
  ------------------------------------------------
  p_sequencer : process is

    variable v_axilite_bfm_config : t_axilite_bfm_config := C_AXILITE_BFM_CONFIG_DEFAULT;

    procedure test_case_write_read (void : t_void) is
      variable v_pattern : std_logic_vector(31 downto 0);
    begin
      -- Check the read-only ID register first
      axilite_check(AXILITE_VVCT, C_AXILITE_VVC_IDX,
                    to_unsigned(C_ID_OFFSET, 32),
                    C_ID_VALUE,
                    "Read ID register");

      -- Walk all 9 scratch regs: write a known pattern, read back, check.
      for idx in 0 to C_NUM_SCRATCH-1 loop
        -- 0xA5A5_0000 | idx — build as slv since the value overflows
        -- VHDL's 32-bit signed integer.
        v_pattern              := x"A5A50000";
        v_pattern(7 downto 0)  := std_logic_vector(to_unsigned(idx, 8));

        axilite_write(AXILITE_VVCT, C_AXILITE_VVC_IDX,
                      reg_addr(idx), v_pattern,
                      "Write scratch reg " & to_string(idx));

        axilite_check(AXILITE_VVCT, C_AXILITE_VVC_IDX,
                      reg_addr(idx), v_pattern,
                      "Read back scratch reg " & to_string(idx));
      end loop;

      await_completion(AXILITE_VVCT, C_AXILITE_VVC_IDX, 1 ms, C_SCOPE);
    end procedure test_case_write_read;

    procedure test_case_random (void : t_void) is
      constant C_NUM_ITERATIONS : integer := 200;
      variable v_rand           : t_rand;
      variable v_idx            : integer;
      variable v_data           : std_logic_vector(31 downto 0);
      type t_shadow is array (0 to C_NUM_SCRATCH-1) of std_logic_vector(31 downto 0);
      variable v_shadow         : t_shadow := (others => (others => '0'));
    begin
      -- ID register stays RO
      axilite_check(AXILITE_VVCT, C_AXILITE_VVC_IDX,
                    to_unsigned(C_ID_OFFSET, 32),
                    C_ID_VALUE,
                    "Read ID register");

      for it in 0 to C_NUM_ITERATIONS-1 loop
        v_idx  := v_rand.rand(0, C_NUM_SCRATCH-1);
        v_data := v_rand.rand(32);

        axilite_write(AXILITE_VVCT, C_AXILITE_VVC_IDX,
                      reg_addr(v_idx), v_data,
                      "Random write reg " & to_string(v_idx));
        v_shadow(v_idx) := v_data;

        -- Read back a random (possibly different) scratch reg and check
        -- against the shadow.
        v_idx := v_rand.rand(0, C_NUM_SCRATCH-1);
        axilite_check(AXILITE_VVCT, C_AXILITE_VVC_IDX,
                      reg_addr(v_idx), v_shadow(v_idx),
                      "Random check reg " & to_string(v_idx));
      end loop;

      await_completion(AXILITE_VVCT, C_AXILITE_VVC_IDX, 10 ms, C_SCOPE);
    end procedure test_case_random;

    procedure test_case_cosim(void : t_void) is
    begin
      -- Indicate to cosim module that VVC configuration is done
      vvc_config_done <= '1';

      -- Start clock for cosim scheduler
      start_clock(CLOCK_GENERATOR_VVCT, 1, "Start cosim clock generator");

      wait for 5000 ms;
    end procedure test_case_cosim;

  begin

    -- Create separate log files for each test case
    set_log_file_name(GC_TESTCASE & "_Log.txt");
    set_alert_file_name(GC_TESTCASE & "_Alert.txt");

    -- Print the configuration to the log
    report_global_ctrl(VOID);
    report_msg_id_panel(VOID);

    enable_log_msg(ALL_MESSAGES);

    -----------------------------------------------------------------------------
    -- Wait for UVVM to finish initialization
    -----------------------------------------------------------------------------
    await_uvvm_initialization(VOID);

    start_clock(CLOCK_GENERATOR_VVCT, 0, "Start clock generator");

    gen_pulse(arst, '1', 10 * C_CLK_PERIOD, "Pulsed reset-signal - active for 10 clock periods");
    wait for C_CLK_PERIOD * 10;

    -----------------------------------------------------------------------------
    -- Verbosity
    -----------------------------------------------------------------------------
    disable_log_msg(ALL_MESSAGES);
    enable_log_msg(ID_SEQUENCER);
    enable_log_msg(ID_LOG_HDR);

    -----------------------------------------------------------------------------
    -- AXI-Lite VVC config
    -----------------------------------------------------------------------------
    v_axilite_bfm_config.clock_period := C_CLK_PERIOD;
    v_axilite_bfm_config.setup_time   := C_CLK_PERIOD / 4;
    v_axilite_bfm_config.hold_time    := C_CLK_PERIOD / 4;

    shared_axilite_vvc_config(C_AXILITE_VVC_IDX).bfm_config := v_axilite_bfm_config;

    -----------------------------------------------------------------------------
    -- Test sequence
    -----------------------------------------------------------------------------
    log(ID_SEQUENCER, "Running testcase: " & GC_TESTCASE, C_SCOPE);

    if GC_TESTCASE = "TC_AXILITE_WRITE_READ" then
      test_case_write_read(VOID);
    elsif GC_TESTCASE = "TC_AXILITE_RANDOM" then
      test_case_random(VOID);
    elsif GC_TESTCASE = "TC_COSIM" then
      test_case_cosim(VOID);
    end if;

    -----------------------------------------------------------------------------
    -- Ending the simulation
    -----------------------------------------------------------------------------
    wait for 1000 ns;
    report_alert_counters(FINAL);
    log(ID_LOG_HDR, "SIMULATION COMPLETED", C_SCOPE);

    std.env.stop;
    wait;

  end process p_sequencer;

end architecture tb;
