library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.textio.all;

library uvvm_util;
context uvvm_util.uvvm_util_context;

library uvvm_vvc_framework;
use uvvm_vvc_framework.ti_vvc_framework_support_pkg.all;

library bitvis_vip_uart;
context bitvis_vip_uart.vvc_context;

library bitvis_vip_axistream;
context bitvis_vip_axistream.vvc_context;

library work;
use work.uvvm_cosim_utils_pkg.all;
use work.vhpi_cosim_methods_pkg.all;

entity uvvm_cosim is
  generic (
    GC_SIM_RUN_CTRL_EN : boolean := false);
  port (
    clk : in std_logic);
end entity uvvm_cosim;


architecture func of uvvm_cosim is

  constant C_SCOPE : string    := "UVVM_COSIM";
  signal init_done : std_logic := '0';

  signal uart_rx_vvc_indexes_in_use : std_logic_vector(0 to C_UART_VVC_MAX_INSTANCE_NUM-1)      := (others => '0');
  signal uart_tx_vvc_indexes_in_use : std_logic_vector(0 to C_UART_VVC_MAX_INSTANCE_NUM-1)      := (others => '0');
  signal axis_vvc_indexes_in_use    : std_logic_vector(0 to C_AXISTREAM_VVC_MAX_INSTANCE_NUM-1) := (others => '0');

begin

  p_uvvm_cosim_init : process
    variable vvc_channel     : t_channel;
    variable vvc_instance_id : integer;
    variable vvc_cfg         : line :=  null;
  begin

    await_uvvm_initialization(VOID);

    log(ID_SEQUENCER, "UVVM initialization complete!", C_SCOPE);

    if GC_SIM_RUN_CTRL_EN then
      log(ID_SEQUENCER, "Waiting to start simulation", C_SCOPE);

      vhpi_cosim_start_sim; -- Blocks until user says sim should start

      log(ID_SEQUENCER, "Starting simulation", C_SCOPE);
    else
      -- Co-sim disabled
      wait;
    end if;

    -- Check which VVCs were registered in this testbench
    for idx in 0 to shared_vvc_activity_register.priv_get_num_registered_vvcs(VOID)-1 loop

      vvc_channel     := shared_vvc_activity_register.priv_get_vvc_channel(idx);
      vvc_instance_id := shared_vvc_activity_register.priv_get_vvc_instance(idx);

      -- Note:
      -- Can't compare strings directly with = because that compares the full
      -- range of the string. The string returned from priv_get_vvc_name always
      -- has range 1 to 20 (probably a fixed size for VVC type name in UVVM),
      -- while the literal has whatever size is needed to hold the characters.
      if strcmp("UART_VVC", shared_vvc_activity_register.priv_get_vvc_name(idx)) then

        -- Mark instance id as in use
        if vvc_channel = TX then
          uart_tx_vvc_indexes_in_use(vvc_instance_id) <= '1';
        elsif vvc_channel = RX then
          uart_rx_vvc_indexes_in_use(vvc_instance_id) <= '1';
        end if;

        -- Comma-separated string with VVC config
        vvc_cfg := bfm_cfg_to_string(shared_uart_vvc_config(vvc_channel, vvc_instance_id).bfm_config);

      elsif strcmp("AXISTREAM_VVC", shared_vvc_activity_register.priv_get_vvc_name(idx)) then
        -- Mark instance id as in use
        axis_vvc_indexes_in_use(vvc_instance_id) <= '1';

        -- Comma-separated string with VVC config
        vvc_cfg := bfm_cfg_to_string(shared_axistream_vvc_config(vvc_instance_id).bfm_config);
      else
        -- Unsupported VVC
        vvc_cfg := bfm_cfg_to_string(VOID);
      end if;

      -- Todo:
      -- Rename to uvvm_cosim_vhpi_report_vvc_instance??

      -- Report VVC info to cosim server via VHPI
      vhpi_cosim_report_vvc_info(
        shared_vvc_activity_register.priv_get_vvc_name(idx),
        to_string(vvc_channel),
        vvc_instance_id,
        vvc_cfg.all
        );

      deallocate(vvc_cfg);

    end loop;

    init_done <= '1';

    wait;
  end process p_uvvm_cosim_init;

  g_uart_vvc_ctrl: for vvc_idx in 0 to C_UART_VVC_MAX_INSTANCE_NUM-1 generate

    inst_uart_vvc_ctrl : entity work.uvvm_cosim_uart_vvc_ctrl
      generic map (
        GC_VVC_IDX => vvc_idx)
      port map (
        clk               => clk,
        tx_vvc_idx_in_use => uart_tx_vvc_indexes_in_use(vvc_idx),
        rx_vvc_idx_in_use => uart_rx_vvc_indexes_in_use(vvc_idx),
        init_done         => init_done);

  end generate g_uart_vvc_ctrl;

  g_axis_vvc_ctrl: for vvc_idx in 0 to C_AXISTREAM_VVC_MAX_INSTANCE_NUM-1 generate

    inst_axis_vvc_ctrl: entity work.uvvm_cosim_axis_vvc_ctrl
      generic map (
        GC_VVC_IDX => vvc_idx)
      port map (
        clk            => clk,
        vvc_idx_in_use => axis_vvc_indexes_in_use(vvc_idx),
        init_done      => init_done);

  end generate g_axis_vvc_ctrl;

end architecture func;
