library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library uvvm_util;
context uvvm_util.uvvm_util_context;

library uvvm_vvc_framework;
use uvvm_vvc_framework.ti_vvc_framework_support_pkg.all;

library bitvis_vip_axistream;
context bitvis_vip_axistream.vvc_context;

library uvvm_cosim_lib;
use uvvm_cosim_lib.uvvm_cosim_utils_pkg.all;
use uvvm_cosim_lib.uvvm_cosim_foreign_pkg.all;

entity uvvm_cosim_axis_vvc_ctrl is
  generic (
    GC_VVC_IDX : natural);
  port (
    clk            : in std_logic;
    vvc_idx_in_use : in std_logic;
    init_done      : in std_logic);
end entity uvvm_cosim_axis_vvc_ctrl;


architecture func of uvvm_cosim_axis_vvc_ctrl is

  constant C_SCOPE    : string := "UVVM_COSIM_AXIS_VVC_CTRL";
  constant C_VVC_TYPE : string := "AXISTREAM_VVC";

begin

  -- Note:
  -- The GC_VVC_IS_MASTER parameter to the axistream_vvc entity is not exposed
  -- or stored anywhere, so it's not possible to know if a given axistream VVC
  -- instance is a transmitter or receiver.
  -- (Technically, maybe it can be done if we write some fancy VHPI code to
  -- search the simulation hierarchy, but that's not worth it).
  -- Instead, both transmit and receive processes are active for any axistream
  -- VVC. Which is not a problem, the user will just get an alert/error from
  -- the VVC if it tries to issue transmit to a receiver instance and vice versa.

  p_transmit : process
    alias vvc_status         : t_vvc_status is shared_axistream_vvc_status(GC_VVC_IDX);
    constant C_CMD_QUEUE_MAX : natural := 32;
    variable v_data          : t_slv_array(0 to C_AXISTREAM_VVC_CMD_DATA_MAX_BYTES-1)(7 downto 0);
    variable v_byte_idx      : integer range 0 to C_AXISTREAM_VVC_CMD_DATA_MAX_BYTES;
  begin

    wait until init_done = '1';
    wait until rising_edge(clk);

    -- Do nothing if no VVC was registered for this index
    if vvc_idx_in_use = '0' then
      wait;
    end if;

    log(ID_SEQUENCER, "Cosim for AXISTREAM VVC " & to_string(GC_VVC_IDX) & " ENABLED.", C_SCOPE);

    loop
      wait until rising_edge(clk);

      v_byte_idx := 0;

      -- Fetch bytes from cosim transmit queue
      while uvvm_cosim_foreign_transmit_byte_queue_empty(C_VVC_TYPE, GC_VVC_IDX) = 0 loop

        if vvc_status.pending_cmd_cnt >= C_CMD_QUEUE_MAX then
          -- Prevent command queue from overflowing (causes UVVM sim error)
          -- Note that C_CMD_QUEUE_COUNT_THRESHOLD would probably be a
          -- reasonable threshold, but since each AXI-Stream VVC command entry
          -- has a max-sized data buffer this will consume tons of memory.
          exit;
        end if;

        if v_byte_idx = C_AXISTREAM_VVC_CMD_DATA_MAX_BYTES then
          exit;
        end if;

        -- TODO:
        -- For packet based transmit use transmit_pkt_queue_get function (not
        -- implemented yet) and collect data in an slv_array buffer
        -- until the 9th bit in v_data (end of packet) is set.
        v_data(v_byte_idx) := std_logic_vector(to_unsigned(uvvm_cosim_foreign_transmit_byte_queue_get(C_VVC_TYPE, GC_VVC_IDX), v_data(0)'length));

        v_byte_idx := v_byte_idx + 1;

        if uvvm_cosim_foreign_transmit_byte_queue_empty(C_VVC_TYPE, GC_VVC_IDX) = 1 then
          log(ID_SEQUENCER, "Transmit queue now empty for VVC index " & to_string(GC_VVC_IDX), C_SCOPE);
        end if;

      end loop;

      -- Transmit any bytes we got from cosim buffer
      if v_byte_idx > 0 then
        log(ID_SEQUENCER, "Got " & to_string(v_byte_idx) & " bytes to transmit on VVC " & to_string(GC_VVC_IDX), C_SCOPE);
        axistream_transmit(AXISTREAM_VVCT, GC_VVC_IDX, v_data(0 to v_byte_idx-1),
                           "Transmit " & to_string(v_byte_idx) & " bytes from uvvm_cosim_axis_vvc_ctrl");
      end if;

    end loop;

  end process p_transmit;


  -- This process monitors AXI-Stream VVC by repeatedly calling axistream_receive(),
  -- retrieving the data and putting it in a buffer for cosim using a foreign VHPI procedure.
  p_receive : process
    alias vvc_transaction_info_trigger : std_logic is global_axistream_vvc_transaction_trigger(GC_VVC_IDX);
    alias vvc_transaction_info         : t_transaction_group is shared_axistream_vvc_transaction_info(GC_VVC_IDX);
    alias bfm_config                   : t_axistream_bfm_config is shared_axistream_vvc_config(GC_VVC_IDX).bfm_config;
    variable v_cmd_idx                 : integer;
    variable v_result_data             : bitvis_vip_axistream.vvc_cmd_pkg.t_vvc_result;

    variable v_start_new_transaction   : boolean := true;

    impure function listen_enable (void : t_void) return boolean is
    begin
      return uvvm_cosim_foreign_vvc_listen_enable("AXISTREAM_VVC", GC_VVC_IDX) = 1;
    end function listen_enable;

    procedure check_bfm_config (void : t_void) is
    begin
      if bfm_config.max_wait_cycles_severity /= NO_ALERT then
        alert(TB_ERROR, "AXISTREAM VVC " & to_string(GC_VVC_IDX) & ": Max wait cycles severity (timeout) should be set to NO_ALERT for cosim", C_SCOPE);
      end if;

      if bfm_config.check_packet_length then
        alert(TB_ERROR, "AXISTREAM VVC = " & to_string(GC_VVC_IDX) & ": Packet length (tlast) is not supported for cosim yet", C_SCOPE);
      end if;
    end procedure check_bfm_config;

  begin

    -- Wait at least for first clock edge before starting
    -- BFM is not configured before that...
    wait until init_done = '1';
    wait until rising_edge(clk);

    -- Do nothing if no VVC was registered for this index
    if vvc_idx_in_use = '0' then
      wait;
    end if;

    loop

      wait until rising_edge(clk);

      v_start_new_transaction := true;

      while listen_enable(void) loop

        -- Check BFM config when listen is enabled
        check_bfm_config(VOID);

        if v_start_new_transaction then
          v_start_new_transaction := false;
          axistream_receive(AXISTREAM_VVCT, GC_VVC_IDX, TO_BUFFER, "Receive data to cosim buffer");
        end if;

        v_cmd_idx := get_last_received_cmd_idx(AXISTREAM_VVCT, GC_VVC_IDX, NA, C_SCOPE);

        wait until rising_edge(vvc_transaction_info_trigger);

        if vvc_transaction_info.bt.transaction_status = COMPLETED then

          fetch_result(AXISTREAM_VVCT, GC_VVC_IDX, v_cmd_idx, v_result_data, "Fetch received data on AXISTREAM VVC " & to_string(GC_VVC_IDX), TB_ERROR, C_SCOPE);

          if v_result_data.data_length = 0 then
            --log(ID_SEQUENCER, "AXISTREAM VVC " & to_string(GC_VVC_IDX) & " transaction timed out", C_SCOPE);
            null;
          else
            log(ID_SEQUENCER, "AXISTREAM VVC " & to_string(GC_VVC_IDX) & ": Transaction completed. Data: " & to_string(v_result_data.data_array(0 to v_result_data.data_length-1), HEX), C_SCOPE);

            for byte_num in 0 to v_result_data.data_length-1 loop
              uvvm_cosim_foreign_receive_byte_queue_put(C_VVC_TYPE, GC_VVC_IDX,
                                                        to_integer(unsigned(v_result_data.data_array(byte_num)))
                                                        );
            -- TODO:
            -- For packet based receive set the 9th bith (end of packet) in the
            -- integer argument to uvvm_cosim_foreign_receive_pkt_queue_put on the last byte.
            end loop;

          end if;

          v_start_new_transaction := true;

        elsif vvc_transaction_info.bt.transaction_status = INACTIVE then
          -- Transaction is inactive - start a new one if listen is still enabled
          v_start_new_transaction := true;

        elsif vvc_transaction_info.bt.transaction_status = IN_PROGRESS then
          -- Transaction is in progress - don't do anything
          null;

        else
          -- Other transaction statuses shouldn't be relevant for the axistream VVC
          alert(TB_ERROR, "AXISTREAM VVC " & to_string(GC_VVC_IDX) & ": Got unexpected transaction status " & to_string(vvc_transaction_info.bt.transaction_status), C_SCOPE);
        end if;

      end loop;

    end loop;

  end process p_receive;

end architecture func;
