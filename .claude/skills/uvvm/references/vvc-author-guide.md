# Authoring or modifying a VVC

Use this when you're adding a new VVC, fixing an existing one, or porting a third-party BFM into the UVVM VVC framework.

## When to use the generator vs. when to clone

`uvvm_vvc_framework/script/vvc_generator/vvc_generator.py` (Python 3) produces a skeleton given a name, channel layout, and feature flags (scoreboard, transaction info, unwanted-activity detection, multiple executors per channel). Use it to seed a *new* VVC. To extend an existing one, edit the existing files directly — the generator does not merge.

VVC-name length is capped by `C_LOG_SCOPE_WIDTH` (default 20) in `uvvm_util/src/adaptations_pkg.vhd`. New channel names other than `TX`/`RX` must be added to the `t_channel` type in that file.

## Files the generator produces

| File | Purpose | What you must fill in |
|---|---|---|
| `<name>_bfm_pkg.vhd` | BFM skeleton only — no actual BFM procedures. Used standalone by `compile_src.do`. | The BFM itself: a `t_<name>_if` signal record, a `t_<name>_bfm_config` config record + `C_<NAME>_BFM_CONFIG_DEFAULT` constant, the BFM procedures (write/read/transmit/receive/check) that toggle interface signals. **If you want `INSERT_DELAY` to work, the config must include a `clock_period` field**. |
| `vvc_cmd_pkg.vhd` | `t_operation`, `t_vvc_cmd_record` (transports a command from sequencer to interpreter), `C_VVC_CMD_DEFAULT`, `shared_vvc_cmd`, `t_vvc_result`, `shared_vvc_response`, `C_VVC_CMD_DATA_MAX_*` size constants. | Add VVC-specific operation enums (e.g. `WRITE`, `READ`, `TRANSMIT`). Add VVC-specific fields to `t_vvc_cmd_record` (data, address, count, …). Set max sizes. If `fetch_result()` returns something other than `std_logic_vector`, redefine `t_vvc_result` (record). |
| `vvc_methods_pkg.vhd` | The procedures the sequencer calls (e.g. `<name>_transmit`). `C_VVC_NAME`. `<NAME>_VVCT` target signal. `t_vvc_config` (with embedded `bfm_config`), `t_vvc_status`, `shared_<name>_vvc_config`, `shared_<name>_vvc_status`. | One sequencer-facing procedure per BFM operation. Bodies follow a fixed pattern (see below). |
| `transaction_pkg.vhd` *(if you enabled transaction info)* | `t_base_transaction`, `t_transaction_group`, `global_<name>_vvc_transaction_trigger`, `shared_<name>_vvc_transaction_info`. | Add data fields to `t_base_transaction`. |
| `vvc_context.vhd` | Context unit `vvc_context` that re-exports `transaction_pkg`, `vvc_methods_pkg`, `td_vvc_framework_common_methods_pkg`, the BFM types. | Usually nothing — just re-export. |
| `<name>_vvc.vhd` | The entity + behave architecture: constructor, `cmd_interpreter`, `cmd_executor`, `cmd_terminator`. | The `case v_cmd.operation is …` block inside `cmd_executor` that maps each VVC operation to a BFM call, plus result storage with `store_result(result_queue, cmd_idx, v_result)` for read/receive. |
| `vvc_sb_pkg.vhd` *(if scoreboard enabled)* | The per-VVC scoreboard instance. | Tune the scoreboard element type if not `std_logic_vector`. |
| `<name>_<channel>_vvc.vhd`, `<name>_vvc.vhd` *(if multi-channel)* | Per-channel leaf VVCs + wrapper. | Wire BFM-direction restrictions (a TX leaf must reject RECEIVE, etc.). |

The generator marks every required spot with the literal token `--<USER_INPUT>` plus instructions in a comment block. Grep for that token after generation.

## The fixed pattern of a sequencer-facing procedure

(from `vvc_framework.rst` — VVC methods package section)

```vhdl
procedure <name>_<op>(
    signal   VVCT             : inout t_vvc_target_record;
    constant vvc_instance_idx : in    integer;
    constant <op_args>        : in    <types>;
    constant msg              : in    string;
    constant scope            : in    string         := C_VVC_CMD_SCOPE_DEFAULT;
    constant parent_msg_id_panel : in t_msg_id_panel := C_UNUSED_MSG_ID_PANEL  -- HVVC only
) is
    constant proc_name : string := "<name>_<op>";
    constant proc_call : string := proc_name & "(...)";  -- include args via to_string
begin
    -- 1. Reset shared_vvc_cmd to default.
    shared_vvc_cmd := C_VVC_CMD_DEFAULT;
    -- 2. Set common target/command fields.
    set_general_target_and_command_fields(VVCT, vvc_instance_idx, proc_call, msg, QUEUED, <op_enum>);
    -- 3. Set VVC-specific fields.
    shared_vvc_cmd.<field> := <value>;
    ...
    -- 4. Dispatch.
    send_command_to_vvc(VVCT, scope => scope, msg_id_panel => shared_msg_id_panel);
end procedure;
```

Multi-channel VVCs add a `constant channel : in t_channel;` parameter (after `vvc_instance_idx`) and use the overload of `set_general_target_and_command_fields` that takes a channel.

Use `QUEUED` for normal BFM-bound commands. Use `IMMEDIATE` for commands the interpreter must execute synchronously (`AWAIT_COMPLETION`, `ENABLE_LOG_MSG`, `FETCH_RESULT`, `FLUSH_COMMAND_QUEUE`, `TERMINATE_CURRENT_COMMAND`, `INSERT_DELAY`); these are already implemented by the common methods and you usually shouldn't add new immediate ones.

## The `cmd_executor` `case` block

Each operation maps to a BFM call. Surround BFM-access cases with transaction-info updates:

```vhdl
when TRANSMIT =>
    set_global_vvc_transaction_info(vvc_transaction_info_trigger, vvc_transaction_info,
                                    v_cmd, vvc_config, IN_PROGRESS, C_SCOPE);
    <name>_transmit(
        data_array  => v_cmd.data_array(0 to v_cmd.data_array_length - 1),
        msg         => format_msg(v_cmd),
        clk         => clk,
        <name>_if   => <name>_vvc_if,
        scope       => C_SCOPE,
        msg_id_panel => v_msg_id_panel,
        config      => vvc_config.bfm_config);
    set_global_vvc_transaction_info(vvc_transaction_info_trigger, vvc_transaction_info,
                                    v_cmd, vvc_config, COMPLETED, C_SCOPE);

when RECEIVE =>
    set_global_vvc_transaction_info(... IN_PROGRESS ...);
    <name>_receive(... data_array => v_result.data_array, data_length => v_result.data_length, ... config => vvc_config.bfm_config);
    if v_cmd.data_routing = TO_SB then
        <NAME>_VVC_SB.check_received(GC_INSTANCE_IDX, v_result.data_array(...));
    else
        store_result(result_queue, v_cmd.cmd_idx, v_result);
    end if;
    set_global_vvc_transaction_info(... v_result COMPLETED ...);
```

Mandatory at the end of every executor iteration: `last_cmd_idx_executed <= v_cmd.cmd_idx;` and `reset_vvc_transaction_info(vvc_transaction_info, v_cmd);`.

Inter-BFM delay handling is done by `insert_inter_bfm_delay_if_requested(...)` at the *start* of the iteration — the generator already inserts that call.

Activity-register update is done by `update_vvc_activity_register(...)` before and after the fetch — also inserted by the generator.

## Compile order

Add a `script/compile_src.do` patterned on existing VIPs. The compile order inside one VVC is always:

1. `transaction_pkg.vhd` (if present)
2. `<name>_bfm_pkg.vhd`
3. The framework target-dependent packages, compiled *into this VVC's library* (`td_*.vhd` from `uvvm_vvc_framework/src_target_dependent/`)
4. `vvc_cmd_pkg.vhd`
5. `vvc_methods_pkg.vhd`
6. `vvc_context.vhd`
7. `<name>_vvc.vhd` (or per-channel leaf + wrapper for multi-channel)

The `td_*` files appear in *every* VVC library — that's the whole point of "target dependent": the same source compiled per-VVC with that VVC's `vvc_cmd_pkg` and `vvc_methods_pkg` in scope, producing VVC-specific signatures for the common methods.

Library dependencies (every VVC needs them):

1. `uvvm_util`
2. `uvvm_vvc_framework`
3. `bitvis_vip_scoreboard` (most VVCs ship with one)

## Constants tuned per VVC (in `adaptations_pkg.vhd`)

Per-VVC instance-count cap: `C_<NAME>_VVC_MAX_INSTANCE_NUM` (defaults to `C_MAX_VVC_INSTANCE_NUM` = 10).

Per-VVC data sizes (example for AXI-Stream):
- `C_AXISTREAM_VVC_CMD_DATA_MAX_BYTES` (default 16 384)
- `C_AXISTREAM_BFM_MAX_TUSER_BITS`, `C_AXISTREAM_BFM_MAX_TSTRB_BITS`, `C_AXISTREAM_BFM_MAX_TID_BITS`, `C_AXISTREAM_BFM_MAX_TDEST_BITS`
- `C_AXISTREAM_VVC_CMD_MAX_WORD_LENGTH`, `C_AXISTREAM_VVC_CMD_STRING_MAX_LENGTH`

To customise, copy `adaptations_pkg.vhd` into the project, edit, and compile the project's copy into `uvvm_util` *instead of* the default.

## Local sequencers (multi-BFM-call VVC commands)

If a VVC command translates into more than one BFM access (e.g. `sbi_poll_until`, "transmit N random bytes"), put the loop **inside the VVC executor**, not inside the BFM, so that:

- Transaction info can be updated for each leaf transaction.
- The terminate flag (`terminate_current_cmd.is_active = '1'`) can be checked between iterations and bail out cleanly.

## Common pitfalls

- `vvc_constructor` will `TB_FAILURE` if `GC_INSTANCE_IDX >= C_<NAME>_VVC_MAX_INSTANCE_NUM` or if UVVM hasn't been initialised — make sure `ti_uvvm_engine` is in the harness.
- Forgetting `acknowledge_cmd(global_vvc_ack, v_local_vvc_cmd.cmd_idx)` at the end of every interpreter iteration will hang the sequencer at `send_command_to_vvc`. The generated code handles this — don't remove it.
- BFMs that randomise should only randomise behaviour the DUT cannot detect (e.g. inter-cycle gaps). Anything that affects the DUT-visible stimulus must be set explicitly via BFM-config fields.
- For multi-channel VVCs: each leaf VVC must handle **one** independent DUT-comm thread. Don't try to multiplex TX and RX through one executor.
