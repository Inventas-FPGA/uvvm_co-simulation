library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library uvvm_util;
context uvvm_util.uvvm_util_context;

library uvvm_vvc_framework;
use uvvm_vvc_framework.ti_vvc_framework_support_pkg.all;

library bitvis_vip_axilite;
context bitvis_vip_axilite.vvc_context;

library uvvm_cosim_lib;
use uvvm_cosim_lib.uvvm_cosim_utils_pkg.all;
use uvvm_cosim_lib.uvvm_cosim_foreign_pkg.all;

entity uvvm_cosim_axilite_vvc_ctrl is
  generic (
    GC_VVC_IDX : natural);
  port (
    clk            : in std_logic;
    vvc_idx_in_use : in std_logic;
    init_done      : in std_logic);
end entity uvvm_cosim_axilite_vvc_ctrl;


architecture func of uvvm_cosim_axilite_vvc_ctrl is

  constant C_SCOPE    : string := "UVVM_COSIM_AXILITE_VVC_CTRL";
  constant C_VVC_TYPE : string := "AXILITE_VVC";

begin

  -- AXI-Lite is purely request-driven: every transaction starts from a client
  -- request received on the cosim request queue. There is no "listen" mode.
  --
  -- A single process handles both directions:
  --
  --   Phase 1 (dispatch): drain the cosim request queue into the VVC's command
  --   queue (non-blocking; axilite_write/read just enqueue).
  --
  --   Phase 2 (harvest): for every dispatched command whose cmd_idx the VVC
  --   activity register reports as executed, fetch the result if it was a
  --   read, then push the corresponding response (write-done or read data)
  --   back to the cosim response queue.
  --
  -- The local pending_ops FIFO records each in-flight VVC command in dispatch
  -- order. Because the AXI-Lite VVC executes commands strictly in order, the
  -- front of the FIFO is always the next command we expect to complete.
  --
  -- Note on completion detection: shared_axilite_vvc_status.previous_cmd_idx
  -- is NOT safe to use here. The AXI-Lite VVC has five per-channel executors
  -- in addition to the main cmd_executor, and they all write previous_cmd_idx
  -- concurrently. The main cmd_executor also overwrites it to the cmd_idx of
  -- the just-dispatched command *before* the channel executors have actually
  -- driven the bus or stored the read result. We use the activity register's
  -- priv_is_cmd_idx_executed() instead — the same primitive await_completion()
  -- uses — which consults the per-VVC pending_cmd_idx_list populated by the
  -- AXI-Lite VVC at the end of each main-executor iteration.
  p_ctrl : process
    alias vvc_status : t_vvc_status is shared_axilite_vvc_status(GC_VVC_IDX);

    -- Throttle dispatch when the VVC's command queue is near full, otherwise
    -- UVVM raises an error and the simulation aborts.
    constant C_CMD_QUEUE_MAX  : natural := 32;
    -- Sized to match how many in-flight ops we'll ever realistically need
    -- (capped by C_CMD_QUEUE_MAX above).
    constant C_PENDING_MAX    : natural := 64;

    type t_pending_op is record
      cmd_idx   : natural;
      is_read   : boolean;
      num_bytes : natural;
    end record;
    type t_pending_op_array is array (natural range <>) of t_pending_op;

    variable v_pending : t_pending_op_array(0 to C_PENDING_MAX - 1);
    variable v_head    : natural := 0;  -- next index to pop
    variable v_tail    : natural := 0;  -- next index to push
    variable v_count   : natural := 0;

    variable v_is_write    : integer;
    variable v_addr_int    : integer;
    variable v_num_bytes   : integer;
    variable v_byte        : integer;
    variable v_cmd_idx     : natural;
    variable v_activity_idx : integer;

    variable v_addr   : unsigned(C_VVC_CMD_ADDR_MAX_LENGTH - 1 downto 0);
    variable v_data   : std_logic_vector(C_VVC_CMD_DATA_MAX_LENGTH - 1 downto 0);
    variable v_result : bitvis_vip_axilite.vvc_cmd_pkg.t_vvc_result;
    variable v_op     : t_pending_op;

    procedure push_pending (constant op : in t_pending_op) is
    begin
      v_pending(v_tail) := op;
      v_tail            := (v_tail + 1) mod C_PENDING_MAX;
      v_count           := v_count + 1;
    end procedure;

    procedure pop_pending (variable op : out t_pending_op) is
    begin
      op      := v_pending(v_head);
      v_head  := (v_head + 1) mod C_PENDING_MAX;
      v_count := v_count - 1;
    end procedure;

  begin

    wait until init_done = '1';
    wait until rising_edge(clk);

    if vvc_idx_in_use = '0' then
      wait;
    end if;

    -- Look up this VVC's entry in the activity register. Used by the harvest
    -- loop to check command completion via priv_is_cmd_idx_executed().
    -- The string literal needs a qualified expression because the protected
    -- type has two priv_get_vvc_idx overloads that NVC can't disambiguate
    -- without it.
    v_activity_idx := shared_vvc_activity_register.priv_get_vvc_idx(
                        name     => string'("AXILITE_VVC"),
                        instance => GC_VVC_IDX,
                        channel  => NA);
    if v_activity_idx < 0 then
      alert(TB_ERROR, "AXILITE VVC " & to_string(GC_VVC_IDX) &
            " not found in activity register", C_SCOPE);
      wait;
    end if;

    log(ID_SEQUENCER, "Cosim for AXILITE VVC " & to_string(GC_VVC_IDX) & " ENABLED.", C_SCOPE);

    loop
      wait until rising_edge(clk);

      ----------------------------------------------------------------------
      -- Phase 1: dispatch new requests to the VVC
      ----------------------------------------------------------------------
      while uvvm_cosim_foreign_mm_request_empty(C_VVC_TYPE, GC_VVC_IDX) = 0
            and vvc_status.pending_cmd_cnt < C_CMD_QUEUE_MAX
            and v_count < C_PENDING_MAX
      loop
        uvvm_cosim_foreign_mm_request_fetch(C_VVC_TYPE, GC_VVC_IDX);

        v_is_write  := uvvm_cosim_foreign_mm_request_is_write(C_VVC_TYPE, GC_VVC_IDX);
        v_addr_int  := uvvm_cosim_foreign_mm_request_get_addr(C_VVC_TYPE, GC_VVC_IDX);
        v_num_bytes := uvvm_cosim_foreign_mm_request_get_num_bytes(C_VVC_TYPE, GC_VVC_IDX);

        v_addr := unsigned(std_logic_vector(to_signed(v_addr_int, v_addr'length)));

        if v_is_write = 1 then
          -- Reconstruct the write data word from per-byte fetches. Unused
          -- upper bits are left at zero; UVVM's normalize_and_check drops
          -- them to the VVC's configured data width.
          v_data := (others => '0');
          for byte_idx in 0 to v_num_bytes - 1 loop
            v_byte := uvvm_cosim_foreign_mm_request_get_byte(C_VVC_TYPE, GC_VVC_IDX, byte_idx);
            v_data(byte_idx*8 + 7 downto byte_idx*8) :=
              std_logic_vector(to_unsigned(v_byte, 8));
          end loop;

          axilite_write(AXILITE_VVCT, GC_VVC_IDX, v_addr, v_data,
                        "Write from uvvm_cosim_axilite_vvc_ctrl");
          v_cmd_idx := get_last_received_cmd_idx(AXILITE_VVCT, GC_VVC_IDX, NA, C_SCOPE);
          push_pending((cmd_idx => v_cmd_idx, is_read => false, num_bytes => 0));

          log(ID_SEQUENCER, "Queued AXILITE write: addr=0x" & to_string(v_addr, HEX) &
              " num_bytes=" & to_string(v_num_bytes) & " cmd_idx=" & to_string(v_cmd_idx),
              C_SCOPE);
        else
          axilite_read(AXILITE_VVCT, GC_VVC_IDX, v_addr, TO_BUFFER,
                       "Read from uvvm_cosim_axilite_vvc_ctrl");
          v_cmd_idx := get_last_received_cmd_idx(AXILITE_VVCT, GC_VVC_IDX, NA, C_SCOPE);
          push_pending((cmd_idx => v_cmd_idx, is_read => true, num_bytes => v_num_bytes));

          log(ID_SEQUENCER, "Queued AXILITE read: addr=0x" & to_string(v_addr, HEX) &
              " num_bytes=" & to_string(v_num_bytes) & " cmd_idx=" & to_string(v_cmd_idx),
              C_SCOPE);
        end if;
      end loop;

      ----------------------------------------------------------------------
      -- Phase 2: harvest completions (in FIFO order)
      ----------------------------------------------------------------------
      while v_count > 0
            and shared_vvc_activity_register.priv_is_cmd_idx_executed(
                  v_activity_idx, v_pending(v_head).cmd_idx)
      loop
        pop_pending(v_op);

        if v_op.is_read then
          fetch_result(AXILITE_VVCT, GC_VVC_IDX, v_op.cmd_idx, v_result,
                       "Fetch AXILITE read result (cmd_idx=" & to_string(v_op.cmd_idx) & ")",
                       TB_ERROR, C_SCOPE);

          log(ID_SEQUENCER, "AXILITE read result (cmd_idx=" & to_string(v_op.cmd_idx) & "): 0x" &
              to_string(v_result, HEX), C_SCOPE);

          for byte_idx in 0 to v_op.num_bytes - 1 loop
            v_byte := to_integer(unsigned(v_result(byte_idx*8 + 7 downto byte_idx*8)));
            uvvm_cosim_foreign_mm_response_put_byte(C_VVC_TYPE, GC_VVC_IDX, v_byte);
          end loop;
          uvvm_cosim_foreign_mm_response_read_done(C_VVC_TYPE, GC_VVC_IDX);
        else
          uvvm_cosim_foreign_mm_response_write_done(C_VVC_TYPE, GC_VVC_IDX);

          log(ID_SEQUENCER, "AXILITE write completed (cmd_idx=" & to_string(v_op.cmd_idx) & ")",
              C_SCOPE);
        end if;
      end loop;

    end loop;

  end process p_ctrl;

end architecture func;
