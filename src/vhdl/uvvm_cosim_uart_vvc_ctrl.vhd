library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library uvvm_util;
context uvvm_util.uvvm_util_context;

library uvvm_vvc_framework;
use uvvm_vvc_framework.ti_vvc_framework_support_pkg.all;

library bitvis_vip_uart;
context bitvis_vip_uart.vvc_context;

library work;
use work.uvvm_cosim_utils_pkg.all;
use work.vhpi_cosim_methods_pkg.all;

entity uvvm_cosim_uart_vvc_ctrl is
  generic (
    GC_VVC_IDX : natural);
  port (
    clk               : in std_logic;
    tx_vvc_idx_in_use : in std_logic;
    rx_vvc_idx_in_use : in std_logic;
    init_done         : in std_logic);
end entity uvvm_cosim_uart_vvc_ctrl;


architecture func of uvvm_cosim_uart_vvc_ctrl is

  constant C_SCOPE    : string := "UVVM_COSIM_UART_VVC_CTRL";
  constant C_VVC_TYPE : string := "UART_VVC";

begin

  p_transmit : process
    alias vvc_status         : t_vvc_status is shared_uart_vvc_status(TX, GC_VVC_IDX);
    constant C_CMD_QUEUE_MAX : natural := 1000;
    variable v_data          : std_logic_vector(7 downto 0);
  begin

    wait until init_done = '1';
    wait until rising_edge(clk);

    -- Do nothing if no VVC was registered for TX channel on this VVC index
    if tx_vvc_idx_in_use = '0' then
      wait;
    end if;

    log(ID_SEQUENCER, "Cosim for UART TX VVC " & to_string(GC_VVC_IDX) & " ENABLED.", C_SCOPE);

    loop
      wait until rising_edge(clk);

      -- Schedule VVC transmit commands
      while vhpi_cosim_transmit_queue_empty(C_VVC_TYPE, GC_VVC_IDX) = 0 loop

        if vvc_status.pending_cmd_cnt >= C_CMD_QUEUE_COUNT_THRESHOLD then
          exit;
        end if;

        v_data := std_logic_vector(to_unsigned(vhpi_cosim_transmit_queue_get(C_VVC_TYPE, GC_VVC_IDX), v_data'length));

        log(ID_SEQUENCER, "Got byte to transmit: " & to_string(v_data, HEX), C_SCOPE);

        -- Note that UART VVC can't queue up more than one byte at a time,
        -- so there's no point in gathering up a longer sequence of bytes
        -- like we do for the AXI-Stream VVC
        uart_transmit(UART_VVCT, GC_VVC_IDX, TX, v_data, "Transmit from uvvm_cosim_uart_vvc_ctrl");

        if vhpi_cosim_transmit_queue_empty(C_VVC_TYPE, GC_VVC_IDX) = 1 then
          log(ID_SEQUENCER, "Transmit queue now empty for VVC index " & to_string(GC_VVC_IDX), C_SCOPE);
        end if;

      end loop;
    end loop;

  end process p_transmit;


  -- This process monitors UART Rx VVC by repeatedly calling uart_receive(),
  -- retrieving the data and putting it in a buffer for cosim using a foreign VHPI procedure.
  p_receive : process
    alias vvc_transaction_info_trigger : std_logic is global_uart_vvc_transaction_trigger(RX, GC_VVC_IDX);
    alias vvc_transaction_info         : t_transaction_group is shared_uart_vvc_transaction_info(RX, GC_VVC_IDX);
    alias bfm_config                   : t_uart_bfm_config is shared_uart_vvc_config(RX, GC_VVC_IDX).bfm_config;
    variable v_cmd_idx                 : integer;
    variable v_result_data             : bitvis_vip_uart.vvc_cmd_pkg.t_vvc_result;

    variable v_start_new_transaction   : boolean := true;

    function listen_enable (void : t_void) return boolean is
    begin
      if vhpi_cosim_vvc_listen_enable("UART_VVC", GC_VVC_IDX) then
        return true;
      else
        return false;
      end if;
    end function listen_enable;

    procedure check_bfm_config (void : t_void) is
    begin
      if bfm_config.timeout > 0 ns then
        alert(TB_ERROR, "UART RX VVC " & to_string(GC_VVC_IDX) & ": timeout = " & to_string(bfm_config.timeout) & ", must be zero (infinite) for cosim to work properly", C_SCOPE);
      end if;
    end procedure check_bfm_config;

  begin

    -- Wait at least for first clock edge before starting
    -- BFM is not configured before that...
    wait until init_done = '1';
    wait until rising_edge(clk);

    -- Do nothing if no VVC was registered for RX channel on this VVC index
    if rx_vvc_idx_in_use = '0' then
      wait;
    end if;

    log(ID_SEQUENCER, "Cosim for UART RX VVC " & to_string(GC_VVC_IDX) & " ENABLED.", C_SCOPE);

    loop

      wait until rising_edge(clk);

      v_start_new_transaction := true;

      while listen_enable(void) loop

        -- Check BFM config when listen is enabled
        check_bfm_config(VOID);

        if v_start_new_transaction then
          v_start_new_transaction := false;
          uart_receive(UART_VVCT, GC_VVC_IDX, RX, TO_BUFFER, "Receive data to cosim buffer");
        end if;

        v_cmd_idx := get_last_received_cmd_idx(UART_VVCT, GC_VVC_IDX, RX, C_SCOPE);

        wait until rising_edge(vvc_transaction_info_trigger);

        if vvc_transaction_info.bt.transaction_status = COMPLETED then
          -- Technically, the received byte in v_result is also available in
          -- vvc_transaction_info. But we still HAVE to fetch the result
          -- because otherwise the VVC's result queue will keep growing until
          -- it's full and trigger a fatal alart.
          fetch_result(UART_VVCT, GC_VVC_IDX, RX, v_cmd_idx, v_result_data, "Fetch received data on UART RX VVC " & to_string(GC_VVC_IDX), TB_ERROR, C_SCOPE);

          if v_result_data = x"XX" then
            --log(ID_SEQUENCER, "UART RX VVC " & to_string(GC_VVC_IDX) & " transaction timed out", C_SCOPE);
            null;
          else
            log(ID_SEQUENCER, "UART RX VVC " & to_string(GC_VVC_IDX) & ": Transaction completed. Data: " & to_string(v_result_data, HEX), C_SCOPE);

            -- Last argument is end of packet flag which is not used
            -- for UART (so it's always false)
            vhpi_cosim_receive_queue_put(C_VVC_TYPE, GC_VVC_IDX,
                                         to_integer(unsigned(v_result_data)),
                                         0 -- end_of_packet=false (not used)
                                         );
          end if;

          v_start_new_transaction := true;

        elsif vvc_transaction_info.bt.transaction_status = INACTIVE then
          -- Transaction is inactive - start a new one if listen is still enabled
          v_start_new_transaction := true;

        elsif vvc_transaction_info.bt.transaction_status = IN_PROGRESS then
          -- Transaction is in progress - don't do anything
          null;

        else
          -- Other transaction statuses shouldn't be relevant for the UART VVC
          alert(TB_ERROR, "UART RX VVC " & to_string(GC_VVC_IDX) & ": Got unexpected transaction status " & to_string(vvc_transaction_info.bt.transaction_status), C_SCOPE);
        end if;

      end loop;

    end loop;

  end process p_receive;

end architecture func;
