---
name: uvvm
description: Work with the UVVM (Universal VHDL Verification Methodology) library — its Utility Library, VVC Framework, BFMs, and the standard VIPs (UART, AXI-Stream, AXI-Lite, SBI, I2C, SPI, etc.). Use when reading or writing testbenches that use UVVM, when integrating a VVC into the cosim layer, when looking up BFM/VVC procedure signatures, or when authoring or modifying a VVC.
---

# UVVM skill

This project depends on the UVVM library, vendored at `thirdparty/uvvm/`. UVVM has two complexity levels:

1. **Utility Library + BFMs** — logging, alerts, checks, simple data conversion, and bus-functional models. Usable on its own for simple testbenches.
2. **VVC Framework** — adds VHDL Verification Components on top of BFMs. The framework provides a sequencer → interpreter → executor → BFM command pipeline that lets a single test sequencer command multiple parallel "VVCs" without micromanaging clocks. This is what this cosim project bridges to external clients.

Everything below describes the framework as found in the vendored copy.

## Terminology

When the UVVM documentation talks about **"the testbench sequencer"** (sometimes "central test sequencer", "test sequencer", or just "sequencer") it does **not** mean a UVVM component — it means a plain VHDL `process` in *your* testbench file. By convention this process is named `p_sequencer` and contains the linear, software-like sequence of actions the test performs: start the clocks, pulse reset, configure the BFM/VVC, issue VVC commands like `axistream_transmit(...)` or `uart_expect(...)`, check status registers, call `await_completion(...)`, report counters, and `std.env.stop`. There is normally exactly one such process per testbench (multiple are supported but not recommended; see `vvc_framework.rst` "Multiple Central Sequencers").

The other concurrent things in a UVVM testbench are *not* sequencers: the VVC entities run their own interpreter+executor processes internally; the clock-generator VVC drives the clock; and `ti_uvvm_engine` is a one-shot init helper. You only write the sequencer process — UVVM owns the rest.

## Repository layout (`thirdparty/uvvm/`)

| Directory | Contains |
|---|---|
| `uvvm_util/src/` | Utility Library: `methods_pkg.vhd` (check_value, log, alert, await_*), `string_methods_pkg.vhd`, `types_pkg.vhd`, `adaptations_pkg.vhd` (all the tunable size/limit constants), `data_queue_pkg.vhd`, `data_fifo_pkg.vhd`, `data_stack_pkg.vhd`, `generic_queue_pkg.vhd`, `protected_types_pkg.vhd`, `rand_pkg.vhd`, `func_cov_pkg.vhd` |
| `uvvm_vvc_framework/src/` | **Target-independent** framework code (compiled once into `uvvm_vvc_framework` library). Files are prefixed `ti_*`. Key: `ti_uvvm_engine.vhd` (the engine entity), `ti_vvc_framework_support_pkg.vhd`, `ti_protected_types_pkg.vhd`, `ti_generic_queue_pkg.vhd`. |
| `uvvm_vvc_framework/src_target_dependent/` | **Target-dependent** code (compiled into *each individual* VVC library). Files are prefixed `td_*`. Key: `td_target_support_pkg.vhd` (defines `t_vvc_target_record`, `send_command_to_vvc`, `acknowledge_cmd`), `td_vvc_entity_support_pkg.vhd` (`vvc_constructor`, `await_cmd_from_sequencer`, `fetch_command_and_prepare_executor`, the `interpreter_*` procedures), `td_vvc_framework_common_methods_pkg.vhd` (the common procedures exposed to the test sequencer: `await_completion`, `fetch_result`, `flush_command_queue`, `insert_delay`, etc.), `td_queue_pkg.vhd`. |
| `bitvis_vip_<name>/src/` | One VIP per bus type. Always contains the same files (see "Anatomy of a VVC" below). Available VIPs: `uart`, `axistream`, `axilite`, `axi`, `avalon_mm`, `avalon_st`, `sbi`, `spi`, `i2c`, `gpio`, `gmii`, `rgmii`, `ethernet`, `clock_generator`, `error_injection`, `scoreboard`, `spec_cov`, `hvvc_to_vvc_bridge`. |
| `bitvis_irqc/`, `bitvis_uart/` | Two complete DUT-and-testbench examples. Useful when looking for end-to-end usage patterns. |
| `doc/source/*.rst` | The user-facing manual (Sphinx). Prefer these over the PDFs (your tools cap PDFs at 20 pages). Per-VIP files are `vip_<name>.rst`. The main framework reference is `vvc_framework.rst` (~2.2 k lines — read with `offset`/`limit`). |
| `script/compile_all.do` | Master compile script. Each VIP also has its own `script/compile_src.do`. Compile order from `compile_order.txt` files. |

"bitvis" is the historical name of the company that originally developed UVVM — that's why so many things are still prefixed with it.

## Anatomy of a VVC (the same for every `bitvis_vip_<name>`)

```
bitvis_vip_<name>/src/
├── <name>_bfm_pkg.vhd         BFM: signal record t_<name>_if, config record t_<name>_bfm_config (with C_<NAME>_BFM_CONFIG_DEFAULT), low-level BFM procedures
├── transaction_pkg.vhd        t_operation enum (TRANSMIT, RECEIVE, EXPECT, READ, WRITE, …), t_base_transaction record, t_transaction_group, global_<name>_vvc_transaction_trigger, shared_<name>_vvc_transaction_info — for distributing transaction info to models/scoreboards
├── vvc_cmd_pkg.vhd            t_vvc_cmd_record (the command record put on the VVC's queue), shared_vvc_cmd, t_vvc_result (return payload for read/receive), shared_vvc_response, C_VVC_CMD_DATA_MAX_* constants
├── vvc_methods_pkg.vhd        The procedures the test sequencer actually calls (e.g. axistream_transmit, uart_receive), the <NAME>_VVCT target signal, t_vvc_config, t_vvc_status, C_VVC_NAME, shared_<name>_vvc_config and shared_<name>_vvc_status arrays
├── vvc_context.vhd            Context unit; pulled in with `context bitvis_vip_<name>.vvc_context;`
└── <name>_vvc.vhd             The entity. Architecture has three concurrent things: vvc_constructor, cmd_interpreter process, cmd_executor process, cmd_terminator. Multi-channel VVCs split this into per-channel leaf VVCs plus a wrapper (see uart_rx_vvc / uart_tx_vvc / uart_vvc).
```

When reading a VVC for the first time, start at `vvc_methods_pkg.vhd` (user-facing procedures) and `vvc_cmd_pkg.vhd` (the record type carrying everything across the queue). Then check what fields the executor pulls out of `v_cmd` in `<name>_vvc.vhd`.

## Command flow

```
test sequencer  ──axistream_transmit(...)──►  vvc_methods_pkg procedure
                                                  │
                                                  │ sets fields in shared_vvc_cmd,
                                                  │ then send_command_to_vvc(VVCT)
                                                  ▼
                                          AXISTREAM_VVCT  (t_vvc_target_record signal)
                                                  │  ▲
                                                  ▼  │ ACK on global_vvc_ack
                              cmd_interpreter process (inside the VVC entity)
                                                  │
                                                  │ if QUEUED → put_command_on_queue
                                                  │ if IMMEDIATE → execute now (await_completion, flush, fetch_result, …)
                                                  ▼
                                              command_queue
                                                  │
                                                  ▼
                              cmd_executor process (inside the VVC entity)
                                                  │
                                                  │ calls the BFM procedure (axistream_transmit, …)
                                                  │ updates global_<name>_vvc_transaction_trigger + shared_<name>_vvc_transaction_info
                                                  │ for read/receive: store_result(result_queue, cmd_idx, v_result)
                                                  ▼
                                                 DUT
```

Each command has a free-running `cmd_idx`. After `axistream_receive(...)` returns to the sequencer, the sequencer can:

1. Get the index back with `get_last_received_cmd_idx(AXISTREAM_VVCT, idx, [channel])`.
2. Block until execution finishes with `await_completion(AXISTREAM_VVCT, idx, cmd_idx, timeout, msg)`.
3. Pull the actual payload with `fetch_result(AXISTREAM_VVCT, idx, cmd_idx, v_result, …)`.

Alternative for monitor/model code: wait on `global_<name>_vvc_transaction_trigger`, then inspect `shared_<name>_vvc_transaction_info(idx).bt` — operation, data, `transaction_status` (`IN_PROGRESS` / `COMPLETED` for VVCs; monitors also produce `FAILED` / `SUCCEEDED`).

## Mandatory testbench setup

```vhdl
library uvvm_util;
context uvvm_util.uvvm_util_context;

library uvvm_vvc_framework;
use uvvm_vvc_framework.ti_vvc_framework_support_pkg.all;

library bitvis_vip_<name>;
context bitvis_vip_<name>.vvc_context;
```

And in the test-harness architecture:

```vhdl
i_ti_uvvm_engine : entity uvvm_vvc_framework.ti_uvvm_engine;
```

In the sequencer, before issuing any VVC command:

```vhdl
await_uvvm_initialization(VOID);
```

At the end of the sequencer:

```vhdl
await_uvvm_completion(1 ms);  -- waits for all VVCs idle AND all scoreboards empty
```

## Common shared variables and signals

Each VVC exports these (substitute the VVC name, e.g. `uart`, `axistream`, `axilite`):

| Symbol | Type | Purpose |
|---|---|---|
| `<NAME>_VVCT` | `t_vvc_target_record` signal | First arg to every VVC procedure; identifies which VVC library's overload to use. |
| `shared_<name>_vvc_config(idx)` *(or `(channel, idx)` for multi-channel)* | record | Live config: `bfm_config` (sub-record matching the BFM), `inter_bfm_delay`, `cmd_queue_count_max`, `msg_id_panel`, `unwanted_activity_severity`. Mutated directly from the sequencer. |
| `shared_<name>_vvc_status(idx)` | record | `current_cmd_idx`, `previous_cmd_idx`, `pending_cmd_cnt`. Read-only from the sequencer's point of view. |
| `shared_<name>_vvc_transaction_info(idx)` | record (`t_transaction_group`) | `.bt` (base transaction): operation, data fields, `vvc_meta` (msg + cmd_idx), `transaction_status`. |
| `global_<name>_vvc_transaction_trigger(idx)` | std_logic signal | Pulsed at the start *and* end of each transaction. Wait on this in models / scoreboard-support processes. |

`C_<NAME>_VVC_MAX_INSTANCE_NUM` (in `uvvm_util/src/adaptations_pkg.vhd`) sets the per-VVC instance-count limit (default 10 via `C_MAX_VVC_INSTANCE_NUM`). To raise it, copy `adaptations_pkg.vhd` into the project and compile your copy in place of the default.

## Cosim-relevant pattern (read this before touching `src/vhdl/uvvm_cosim_*_ctrl.vhd`)

The cosim ctrl entities are the bridge from the C++ server to the VVCs. They run a per-VVC process that:

1. Pops a request from the cosim queue (via the foreign VHPI/FLI bindings).
2. Calls the VVC's `_transmit` / `_receive` / `_read` / `_write` procedure.
3. Calls `get_last_received_cmd_idx(<NAME>_VVCT, GC_VVC_IDX, [channel], C_SCOPE)` to capture the cmd index.
4. For read/receive: polls `vvc_status.previous_cmd_idx` to detect completion *without blocking the dispatch side*, then `fetch_result(...)` to pull the data and push it to the response side.

`uvvm_cosim_uart_vvc_ctrl.vhd` and `uvvm_cosim_axis_vvc_ctrl.vhd` are the canonical examples — `uvvm_cosim_axilite_vvc_ctrl.vhd` is **work-in-progress** and is not a good reference yet.

A subtle gotcha for cosim:
- AXI-Stream packet vs byte mode is selected by the BFM config's `check_packet_length` (true → packet API with `axistream_transmit_packet` / `axistream_receive_packet`; false → byte API with `axistream_transmit_bytes` / `axistream_receive_bytes`).
- UART RX needs `timeout = 0 ns` (infinite) in the BFM config when used in cosim mode, otherwise `uart_receive` errors out before the external client gets a chance to inject a byte.

## Where to look next

- **Writing a UVVM testbench from scratch** (test harness `_th.vhd` + testbench `_tb.vhd` + `p_sequencer` process + `run.py` for HDLRegression + `GC_TESTCASE` dispatch) — see `references/testbench-authoring.md`. **Start here** if the task is "add a testbench for this DUT".
- **Authoring testbench code that uses an existing VVC** — open `vvc_methods_pkg.vhd` of the VIP for the procedure list, and `<name>_bfm_pkg.vhd` for the `t_<name>_bfm_config` record fields that need to match the DUT.
- **Authoring a new VVC** — see `references/vvc-author-guide.md` for the structure the `vvc_generator.py` script produces and the hooks you must fill in.
- **Recipes for sequencer code** (await_completion, fetch_result, multicast, broadcasting, transaction-info models, scoreboards, randomization, error injection) — see `references/vvc-usage-recipes.md`.
- **Detailed call signatures, message IDs, alert levels, fundamental types** — `references/library-map.md`, then drill into the relevant `doc/source/*.rst` file with `Read` (use `offset`/`limit`; `vvc_framework.rst` and `utility_library.rst` are >2 k lines).
- **Web docs are not authoritative for this checkout** — always cross-check against the vendored copy. Versions drift.

## Build tool

Ignore the `compile_all.do` / `compile_src.do` TCL scripts that UVVM's own docs describe — this project does **not** invoke the simulator through those. We use **HDLRegression** (vendored at `thirdparty/hdlregression/`) driven by a `run.py` at the project / example root. HDLRegression takes a Python list of `(file-glob, library-name)` pairs, figures out compile order, runs the simulator (NVC or Modelsim/Questa, picked from environment), and supports multiple test cases per testbench via the `GC_TESTCASE` generic. See `references/testbench-authoring.md` for the full pattern; the canonical `run.py` examples are `/run.py` (project root) and `examples/axistream_uart/scripts/run.py`.
