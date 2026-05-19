# VVC usage recipes (for sequencer / harness code)

Examples are taken from / faithful to `doc/source/vvc_framework.rst` and the per-VIP `.rst` files. Substitute `<NAME>` and `<name>` for the VVC under use.

## Identifying a VVC

Every VVC procedure starts with the same identification arguments:

```vhdl
<name>_<operation>(
    signal   VVCT             : inout t_vvc_target_record;  -- always pass <NAME>_VVCT
    constant vvc_instance_idx : in    integer;
    constant channel          : in    t_channel;            -- only for multi-channel VVCs (e.g. UART: TX / RX)
    ...                                                     -- VVC-specific args
    constant msg              : in    string;               -- always last positional arg before optionals
    ...);
```

Broadcast / multicast literals (substitute for the corresponding arg):

- `VVC_BROADCAST` instead of `<NAME>_VVCT` — every VVC of every type
- `ALL_INSTANCES` instead of an integer index — every instance of this VVC
- `ALL_CHANNELS` instead of `TX`/`RX` etc. — every channel of this instance

## Initialisation boilerplate

```vhdl
-- In the test harness architecture body:
i_ti_uvvm_engine : entity uvvm_vvc_framework.ti_uvvm_engine;

-- At the very top of the test sequencer process:
await_uvvm_initialization(VOID);

-- At the end of the test sequencer process:
await_uvvm_completion(1 ms,                       -- timeout
                     TB_WARNING,                  -- alert level on timeout
                     100 us,                      -- scoreboard poll period
                     REPORT_ALERT_COUNTERS,
                     REPORT_SCOREBOARDS,
                     REPORT_VVCS,
                     C_SCOPE);
```

## Issuing a command and waiting for it

Simplest case — issue, then wait for *this VVC* to finish all pending commands:

```vhdl
uart_transmit(UART_VVCT, 1, TX, x"AB", "send 0xAB");
await_completion(UART_VVCT, 1, TX, 1 ms, "wait for UART TX idle");
```

Waiting on a *specific* command (use this when you need to fetch its result):

```vhdl
sbi_read(SBI_VVCT, 1, C_ADDR_FIFO_GET, "Read from FIFO");
v_cmd_idx := get_last_received_cmd_idx(SBI_VVCT, 1);
await_completion(SBI_VVCT, 1, v_cmd_idx, 100 ns, "Wait for sbi_read to finish");
fetch_result(SBI_VVCT, 1, v_cmd_idx, v_data, v_is_ok, "Fetching read-result");
check_value(v_is_ok, ERROR, "Readback OK via fetch_result()");
```

`v_data` is of `t_vvc_result` from the VVC's `vvc_cmd_pkg`. The result type is **always wide enough for the maximum payload** — slice it: `v_data_out := v_data.data_array(0 to N-1);` (or `v_data(7 downto 0)` for scalar-result VVCs like SBI).

## Waiting on multiple VVCs

```vhdl
variable v_vvc_list : t_prot_vvc_list;
...
add_to_vvc_list(SBI_VVCT,  1, v_vvc_list);
add_to_vvc_list(UART_VVCT, 1, ALL_CHANNELS, v_vvc_list);
await_completion(ALL_OF, v_vvc_list, 1 ms, CLEAR_LIST, "wait for all", C_SCOPE);
-- or ANY_OF for first-one-wins
```

Broadcast: `await_completion(ALL_VVCS, 1 ms, "wait for everything", C_SCOPE);`

`vvc_list` is a *protected* type — you need:

```vhdl
use uvvm_vvc_framework.ti_protected_types_pkg.all;
```

## Configuring a VVC at runtime

The VVC config is a shared variable, written directly:

```vhdl
shared_uart_vvc_config(TX, 1).bfm_config.bit_time           := C_BIT_TIME;
shared_axistream_vvc_config(1).bfm_config.check_packet_length := false;
shared_axilite_vvc_config(2).bfm_config.clock_period        := 10 ns;

-- Throttle command queue
shared_<name>_vvc_config(idx).cmd_queue_count_max := 32;

-- Disable unwanted-activity alerts (e.g. multiple slaves sharing a bus)
shared_<name>_vvc_config(idx).unwanted_activity_severity := NO_ALERT;
```

The `bfm_config` sub-record matches `t_<name>_bfm_config` from `<name>_bfm_pkg.vhd`. Always check that file for available fields.

## Logging / verbosity

Global panel (does **not** affect VVCs):

```vhdl
disable_log_msg(ALL_MESSAGES);
enable_log_msg(ID_SEQUENCER);
```

Per-VVC panels:

```vhdl
disable_log_msg(VVC_BROADCAST, ALL_MESSAGES);
enable_log_msg(SBI_VVCT, 1, ID_LOG_BFM, "enabling BFM log on SBI VVC 1");
enable_log_msg(UART_VVCT, 1, TX, ID_BFM_WAIT);
```

Useful message IDs (from `uvvm_util/src/adaptations_pkg.vhd`):

| ID | Meaning |
|---|---|
| `ID_SEQUENCER`, `ID_SEQUENCER_SUB`, `ID_LOG_HDR`, `ID_LOG_HDR_LARGE`, `ID_LOG_HDR_XL` | Test-sequencer logging only |
| `ID_BFM`, `ID_BFM_WAIT`, `ID_BFM_POLL`, `ID_CHANNEL_BFM` | Inside BFM procedures |
| `ID_CMD_INTERPRETER[_WAIT]`, `ID_CMD_EXECUTOR[_WAIT]`, `ID_CHANNEL_EXECUTOR[_WAIT]` | VVC pipeline tracing |
| `ID_IMMEDIATE_CMD[_WAIT]` | Interpreter executing an immediate command |
| `ID_UVVM_SEND_CMD`, `ID_UVVM_CMD_ACK`, `ID_UVVM_CMD_RESULT` | Command transport / fetch_result |
| `ID_AWAIT_COMPLETION[_WAIT/_END]`, `ID_AWAIT_UVVM_COMPLETION` | await_completion family |
| `ID_INSERTED_DELAY` | Executor delay logging |
| `ID_CONSTRUCTOR`, `ID_CONSTRUCTOR_SUB` | VVC startup |
| `ID_MONITOR`, `ID_MONITOR_ERROR` | Monitor entities |
| `ID_POS_ACK` | Default for successful check_value (silenced by default) |
| `ID_DATA`, `ID_CTRL` | Scoreboard |
| `ALL_MESSAGES`, `ID_NEVER` | Bulk gates |

## Transaction-info trigger (for models / scoreboards)

```vhdl
p_dut_model : process
  alias sbi_trig : std_logic is global_sbi_vvc_transaction_trigger(C_SBI_VVC_1);
  alias sbi_bt   : bitvis_vip_sbi.transaction_pkg.t_base_transaction
                 is shared_sbi_vvc_transaction_info(C_SBI_VVC_1).bt;
begin
  loop
    wait until sbi_trig = '1';
    if sbi_bt.operation = WRITE and sbi_bt.transaction_status = IN_PROGRESS then
      UART_VVC_SB.add_expected(sbi_bt.data(C_DATA_WIDTH-1 downto 0));
    end if;
  end loop;
end process;
```

Rules of thumb (from the manual):

- **Operations that drive the bus** (WRITE, TRANSMIT, EXPECT): trigger model on `IN_PROGRESS`.
- **Operations that capture from the bus** (READ, RECEIVE): trigger model on `COMPLETED`.

## Insertion of delay / inter-BFM gap

```vhdl
-- Single ad-hoc delay
insert_delay(SBI_VVCT, 1, 50 ns, "explicit delay", C_SCOPE);
insert_delay(SBI_VVCT, 1, 100, "delay 100 clock cycles", C_SCOPE);  -- needs vvc_config.bfm_config.clock_period

-- Persistent inter-BFM separation
shared_sbi_vvc_config(1).inter_bfm_delay.delay_type    := TIME_START2START;
shared_sbi_vvc_config(1).inter_bfm_delay.delay_in_time := 200 ns;
```

`delay_type` values: `NO_DELAY`, `TIME_START2START`, `TIME_FINISH2START`.

## Terminate / flush

```vhdl
terminate_current_command(SBI_VVCT, 1);            -- abort the BFM call running now
terminate_all_commands(UART_VVCT, 1, ALL_CHANNELS); -- + flush the queue
flush_command_queue(SBI_VVCT, 1);                  -- queue only
```

Only effective for BFMs that support cooperative termination (poll-loops, multi-transaction sequences).

## Scoreboard pattern (per VVC)

Most VVCs ship with a built-in scoreboard, accessible as `<NAME>_VVC_SB`. Use:

```vhdl
SBI_VVC_SB.config(1, C_SB_CONFIG);                 -- per instance, optional
SBI_VVC_SB.enable_all_instances(VOID);
...
SBI_VVC_SB.add_expected(C_SBI_VVC_1, x"AB");       -- from a model
...
sbi_read(SBI_VVCT, 1, C_ADDR, TO_SB, "read & SB"); -- read goes to scoreboard
...
SBI_VVC_SB.report_counters(ALL_INSTANCES);
```

To make a VVC forward the received word to its scoreboard, use the `TO_SB` data-routing arg in the receive/read call. Complex scoreboard element types (e.g. AXI-Stream records) must be declared outside the VVC — see `vvc_framework.rst` Figure 8.

## Randomization / data routing in VVC commands

Standard parameter ordering for randomization-aware VVCs (AXI-Stream, UART, SBI):

```
<verb>(<NAME>_VVCT, <inst>, [channel], <count>, <randomness>, <routing>, [routing_idx], "<msg>")
```

`<randomness>`: `NA` | `RANDOM` | `RANDOM_FAVOUR_EDGES` | user-defined (from `t_randomisation`).
`<routing>` (`t_data_routing` in `types_pkg`): `NA` | `TO_SB` | `FROM_BUFFER` | `TO_BUFFER` (FROM/TO_FILE not implemented).

Example: 4 random-edge transmits, also stored in buffer 5:

```vhdl
uart_transmit(UART_VVCT, 1, TX, 4, RANDOM_FAVOUR_EDGES, TO_BUFFER, 5, "send 4 random");
```

The UVVM "buffer" is a global, indexed FIFO collection — see `fifo_collection.rst`.

## Error injection

The BFM config carries an `error_injection` sub-record with booleans (`parity_bit_error`, `stop_bit_error`, ...). The VVC config carries an `error_injection` sub-record with **probabilities** (`parity_bit_error_prob`, `stop_bit_error_prob`, reals in [0.0, 1.0]). VVC config wins over BFM config. Available for some VVCs only (UART is the canonical example).

## Activity watchdog

Concurrent in the test harness:

```vhdl
activity_watchdog(num_exp_vvc => 3, timeout => 10 ms);
-- num_exp_vvc = 0 disables the count check.
-- Each VVC channel counts as one VVC.
-- Cap is C_MAX_TB_VVC_NUM (default 20 in adaptations_pkg).
```

## Multiple central sequencers

Supported but not recommended. If you use them, manually synchronise with `await_barrier` etc. before calling `await_uvvm_completion()` from a single "owner" sequencer.

## Hierarchical VVCs (HVVC)

For higher-protocol-layer VVCs (Ethernet over GMII, etc.). Instantiate the higher-layer VVC and the physical-layer VVC with the same instance index, and let the bridge connect them:

```vhdl
i1_ethernet_vvc : entity bitvis_vip_ethernet.ethernet_vvc
  generic map(GC_INSTANCE_IDX => C_VVC_ETH, GC_PHY_INTERFACE => GMII, GC_PHY_VVC_INSTANCE_IDX => C_VVC_GMII);

i1_gmii_vvc : entity bitvis_vip_gmii.gmii_vvc
  generic map(GC_INSTANCE_IDX => C_VVC_GMII)
  port map(gmii_vvc_tx_if => gmii_tx_if, gmii_vvc_rx_if => gmii_rx_if);
```

Bridge implementations live under `bitvis_vip_hvvc_to_vvc_bridge/src/`.
