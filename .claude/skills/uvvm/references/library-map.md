# Library map — where things live in the vendored UVVM checkout

Paths are all under `thirdparty/uvvm/`. Use the table to jump directly to the file before going to the `.rst` docs.

## Utility Library (`uvvm_util/src/`)

| File | What you get from it |
|---|---|
| `methods_pkg.vhd` | `check_value`, `check_value_in_range`, `check_stable`, `await_value`, `await_stable`, `await_change_to_value`, `randomize`, alert raising (`alert`, `tb_note`, `tb_warning`, `tb_error`, `tb_failure`, `note`/`warning`/`error`/`failure`), `log`, `report_alert_counters`, `set_alert_stop_limit`, `enable_log_msg`, `disable_log_msg`, `await_barrier`, `await_unblock_flag` |
| `string_methods_pkg.vhd` | `to_string` (huge set of overloads), `pad_string`, `replace`, `fill_string`, `format_command_idx`, hex/dec/bin conversions |
| `types_pkg.vhd` | Basic enums and types: `t_channel` (NA, RX, TX, …), `t_alert_level` (NOTE, WARNING, MANUAL_CHECK, NOTE_TO_MANUAL_CHECK, TB_NOTE, TB_WARNING, ERROR, TB_ERROR, FAILURE, TB_FAILURE, NO_ALERT), `t_msg_id` (huge enum — see adaptations_pkg.vhd lines 80–185), `t_msg_id_panel`, `t_radix`, `t_format_zeros`, `t_match_strictness` (MATCH_EXACT / MATCH_STD), `t_byte_endianness`, `t_data_routing`, `t_randomisation`, `t_slv_array`, `t_unsigned_array`, `t_signed_array`, `t_byte_array`, `t_inter_bfm_delay`, `t_bfm_sync`, `t_immediate_or_queued`, `t_quietness`, `t_void`, `t_vvc_select`, `t_list_action`, `t_uvvm_state`, `t_transaction_status` (INACTIVE / IN_PROGRESS / FAILED / SUCCEEDED / COMPLETED) |
| `adaptations_pkg.vhd` | **All the tunable constants.** Copy this file into your project and modify it to override defaults. Key sections: log layout (`C_LOG_*`), MSG_ID panel defaults, alert handling (`C_DEFAULT_ALERT_ATTENTION`, `C_DEFAULT_STOP_LIMIT`), `C_MAX_VVC_INSTANCE_NUM` (default 10), `C_CMD_QUEUE_COUNT_MAX` (default 1000), per-VVC instance/size constants (`C_<NAME>_VVC_*`) |
| `data_queue_pkg.vhd`, `data_fifo_pkg.vhd`, `data_stack_pkg.vhd` | The UVVM "buffer collection" — a global indexed FIFO/queue/stack the VVCs reference with `TO_BUFFER`/`FROM_BUFFER` |
| `generic_queue_pkg.vhd` | Reusable element-type-parameterised queue (used as `td_cmd_queue_pkg`, `td_result_queue_pkg` after target-dependent compilation) |
| `protected_types_pkg.vhd` | `t_protected_semaphore` and friends |
| `rand_pkg.vhd`, `func_cov_pkg.vhd` | Constrained randomization and functional coverage |
| `alert_hierarchy_pkg.vhd`, `hierarchy_linked_list_pkg.vhd` | Hierarchical alert reporting (`C_ENABLE_HIERARCHICAL_ALERTS` in adaptations) |
| `bfm_common_pkg.vhd` | `wait_until_given_time_after_rising_edge`, `wait_num_rising_edge`, etc. — shared BFM building blocks |
| `global_signals_and_shared_variables_pkg.vhd` | `shared_uvvm_state`, `shared_uvvm_status`, `shared_msg_id_panel`, the global "buffer" shared variable, etc. |
| `uvvm_util_context.vhd` | The `uvvm_util_context` context — opens up everything in this dir |

## VVC framework target-independent (`uvvm_vvc_framework/src/`)

Files prefixed `ti_*`. Compiled once into library `uvvm_vvc_framework`.

| File | Purpose |
|---|---|
| `ti_uvvm_engine.vhd` | The `ti_uvvm_engine` entity (drives `shared_uvvm_state` through IDLE → PHASE_A → PHASE_B → INIT_COMPLETED). Instantiate exactly once in the harness. |
| `ti_vvc_framework_support_pkg.vhd` | `await_uvvm_initialization`, `await_uvvm_completion`, `await_completion`, `await_any_completion`, `flag_handler`, `t_flag_record`, `reset_flag`, `update_vvc_activity_register`, the activity-watchdog procedure, `set_global_vvc_transaction_info`, `reset_vvc_transaction_info`, `VVC_BROADCAST` signal, `global_trigger_vvc_activity_register`, `shared_vvc_activity_register` |
| `ti_protected_types_pkg.vhd` | `t_prot_vvc_list` (used by `add_to_vvc_list` / `await_completion(vvc_list,...)`) |
| `ti_generic_queue_pkg.vhd`, `ti_data_*_pkg.vhd` | The framework's queue/data-structure packages |
| `ti_vvc_framework_context.vhd` | The `ti_vvc_framework_context` context unit |

## VVC framework target-dependent (`uvvm_vvc_framework/src_target_dependent/`)

Files prefixed `td_*`. Compiled into **each VVC's library** (as `work.td_*`), so they pick up that VVC's `vvc_cmd_pkg` and `vvc_methods_pkg`.

| File | Purpose |
|---|---|
| `td_target_support_pkg.vhd` | `t_vvc_target_record` (the `<NAME>_VVCT` signal type), `set_vvc_target_defaults`, `send_command_to_vvc`, `acknowledge_cmd`, `set_general_target_and_command_fields`, `format_command_idx`, `get_vvc_index_in_activity_register` |
| `td_vvc_entity_support_pkg.vhd` | `t_vvc_labels`, `assign_vvc_labels`, `format_msg`, `vvc_constructor`, `initialize_interpreter`, `initialize_executor`, `await_cmd_from_sequencer`, `put_command_on_queue`, `fetch_command_and_prepare_executor`, `store_result`, `insert_inter_bfm_delay_if_requested`, and the `interpreter_*` procedures (`interpreter_await_completion`, `interpreter_flush_command_queue`, `interpreter_terminate_current_command`, `interpreter_fetch_result`, …) |
| `td_vvc_framework_common_methods_pkg.vhd` | The procedures called from the *sequencer*: `await_completion`, `await_any_completion`, `enable_log_msg`, `disable_log_msg`, `flush_command_queue`, `fetch_result`, `insert_delay`, `terminate_current_command`, `terminate_all_commands`, `get_last_received_cmd_idx`, `add_to_vvc_list` |
| `td_queue_pkg.vhd` | `td_cmd_queue_pkg`, `td_result_queue_pkg` — instantiations of `ti_generic_queue_pkg` for the command and result queues |

## Per-VVC (`bitvis_vip_<name>/src/`)

Every VVC has the *same* file set — see SKILL.md "Anatomy of a VVC". Key files per common VVC:

- **UART**: `uart_bfm_pkg.vhd`, multi-channel: `uart_rx_vvc.vhd` + `uart_tx_vvc.vhd` + `uart_vvc.vhd` (wrapper). The BFM config has `bit_time`, parity/stop-bit settings, `timeout` (set `0 ns` to disable).
- **AXI-Stream**: `axistream_bfm_pkg.vhd` with `t_axistream_if` (tdata/tkeep/tuser/tvalid/tlast/tready/tstrb/tid/tdest). `check_packet_length` field in BFM config picks packet vs byte mode. Single-channel.
- **AXI-Lite**: `axilite_bfm_pkg.vhd` with `t_axilite_if` (5-channel record). Single-channel VVC. `expected_response`/`expected_response_severity` + `protection_setting` in config.
- **SBI**: trivial parallel bus, single-channel, supports `sbi_poll_until` compound transaction.
- **Clock generator**: `bitvis_vip_clock_generator` — must be in the activity-register count but should not be watched by activity-watchdog (it stays active forever).
- **HVVC-to-VVC bridge**: for hierarchical VVCs (Ethernet-over-GMII etc.). Per-PHY bridge implementations under `bitvis_vip_hvvc_to_vvc_bridge/src/`.
- **Scoreboard**: `bitvis_vip_scoreboard` — a generic-package–based scoreboard, instantiated per VVC.

## Sphinx documentation (`doc/source/`)

| File | Best for |
|---|---|
| `index.rst` | Top-level TOC |
| `uvvm_intro.rst` | Why VVCs, list of VIPs |
| `uvvm_getting_started.rst` | Installation, library inclusion, compile order |
| `utility_library.rst` (~2.7 k lines) | `check_*`, `await_*`, `log`, `alert`, string handling, shared variables, basic randomization |
| `rand_pkg_overview.rst`, `rand_pkg_t_rand.rst`, `rand_pkg_types.rst`, `optimized_rand.rst` | Advanced constrained randomization |
| `func_cov_pkg_*.rst` | Functional coverage |
| `vvc_framework.rst` (~2.2 k lines) | **The reference** for framework methods (`await_uvvm_completion`, `await_completion`, `add_to_vvc_list`, `fetch_result`, `insert_delay`, `terminate_*`, broadcast/multicast), essential mechanisms, activity watchdog, unwanted-activity detection, transaction-info distribution, hierarchical VVCs, randomization-in-VVCs, the VVC implementation guide |
| `fifo_collection.rst`, `generic_queue.rst` | UVVM buffer / generic queue API |
| `vip_<name>.rst` | Per-VIP reference (BFM config fields, all procedure overloads, examples). UART's `.rst` is just a thin wrapper that points at the `.pdf` QuickRefs. |
| `tool_compatibility.rst` | Known issues per simulator |

Always `Read` these with `offset`/`limit` (`vvc_framework.rst` and `utility_library.rst` exceed 25 k tokens; reading the whole file fails).

## Compile/run scripts

| Script | Function |
|---|---|
| `script/compile_all.do` | Compile everything (one-shot for all VIPs). Args: `<script-dir> [target-dir [component-list.txt]]`. |
| `<vip>/script/compile_src.do` | Compile one VIP. Args: `<source-dir> [target-dir]`. |
| `<vip>/script/compile_order.txt` | Authoritative file ordering for that VIP. |
| `uvvm_vvc_framework/script/vvc_generator/vvc_generator.py` | Python 3 VVC skeleton generator. |

## Project-side bridges (`src/vhdl/`)

For this cosim project specifically:

- `uvvm_cosim.vhd` — top-level entity wrapped around the testbench, instantiates the per-VVC ctrl entities, holds the clock loop driving them.
- `uvvm_cosim_uart_vvc_ctrl.vhd` — canonical example of cosim ctrl. Per-VVC dispatch + harvest.
- `uvvm_cosim_axis_vvc_ctrl.vhd` — same pattern for AXI-Stream (handles both packet and byte modes).
- `uvvm_cosim_axilite_vvc_ctrl.vhd` — **work-in-progress**; don't use as a reference yet.
- `uvvm_cosim_utils_pkg.vhd` — `strcmp`, `bfm_cfg_to_string`, per-VVC `C_MAX_*` aliases.
- `uvvm_cosim_foreign_pkg_{vhpi,fli}.vhd` — `attribute foreign` declarations binding VHDL procedures to C symbols in `libuvvm_cosim_{vhpi,fli}.so`.
