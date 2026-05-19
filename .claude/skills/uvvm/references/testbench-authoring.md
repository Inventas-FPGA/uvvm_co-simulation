# Writing a UVVM testbench from scratch

Use this guide when the task is "add a UVVM-based testbench for this RTL module." The reference example lives at `examples/axistream_uart/` — its testbench, harness, and `run.py` are the canonical patterns for this project. Read them alongside this guide.

## The convention: split into harness (`_th.vhd`) and testbench (`_tb.vhd`)

```
src/tb/
├── <dut>_uvvm_th.vhd    -- "test harness": instantiates the DUT + all VVCs + clock generator + ti_uvvm_engine, wires the signals
└── <dut>_uvvm_tb.vhd    -- "testbench":   instantiates the harness, declares VVC indices, contains the p_sequencer process
```

The split keeps the wiring clutter out of the testbench so the `p_sequencer` process is the most prominent thing in `_tb.vhd`. This is a project convention — not a UVVM requirement — but it's the convention used everywhere in this codebase, so follow it.

Mark the testbench top-level entity for HDLRegression with the comment `-- hdlregression:tb` at the top of the file.

## File 1: the test harness (`<dut>_uvvm_th.vhd`)

Responsibilities:

1. Pull in `uvvm_util`, `uvvm_vvc_framework`, and the contexts of every VVC you'll use.
2. Declare any internal signals between the DUT and the VVCs (clocks, reset, the bus records).
3. Instantiate `ti_uvvm_engine` exactly once.
4. Instantiate the DUT.
5. Instantiate each VVC with its instance index passed in as a generic, and the BFM signal record connected to a local signal that's also wired to the DUT.
6. Instantiate one `clock_generator_vvc` per clock domain (don't drive clocks manually).
7. (Optional, cosim only) Instantiate the `uvvm_cosim` entity from `uvvm_cosim_lib`.

Skeleton:

```vhdl
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library uvvm_util;
context uvvm_util.uvvm_util_context;

library uvvm_vvc_framework;
use uvvm_vvc_framework.ti_vvc_framework_support_pkg.all;

library bitvis_vip_clock_generator;
context bitvis_vip_clock_generator.vvc_context;

library bitvis_vip_axistream;
context bitvis_vip_axistream.vvc_context;

library bitvis_vip_uart;
context bitvis_vip_uart.vvc_context;

entity <dut>_uvvm_th is
  generic (
    GC_AXIS_VVC_TRANSMIT_IDX : natural;
    GC_AXIS_VVC_RECEIVE_IDX  : natural;
    GC_UART_VVC_IDX          : natural);
  port (
    arst : in std_logic);
end entity;

architecture th of <dut>_uvvm_th is
  constant C_CLK_PERIOD : time := 20 ns;

  signal clk     : std_logic;
  signal dut_rxd : std_logic;
  signal dut_txd : std_logic;

  -- Constrained subtype for the AXI-Stream record (the BFM record is unconstrained).
  subtype t_axistream_8b is t_axistream_if(
      tdata(7 downto 0), tkeep(0 downto 0), tuser(0 downto 0),
      tstrb(0 downto 0), tid(0 downto 0),   tdest(0 downto 0));

  signal axistream_if_transmit : t_axistream_8b;
  signal axistream_if_receive  : t_axistream_8b;
begin
  -- Drive sideband signals you don't actually use
  axistream_if_transmit.tkeep <= (others => '1');
  axistream_if_transmit.tuser <= (others => '0');
  -- ... (same for tstrb, tid, tdest, and the receive side)

  i_ti_uvvm_engine : entity uvvm_vvc_framework.ti_uvvm_engine;

  i_dut : entity work.<dut>
    generic map (...)
    port map (
      clk => clk, rst => arst,
      rxd => dut_rxd, txd => dut_txd,
      s_axis_transmit_tdata  => axistream_if_transmit.tdata,
      s_axis_transmit_tvalid => axistream_if_transmit.tvalid,
      s_axis_transmit_tready => axistream_if_transmit.tready,
      m_axis_receive_tdata   => axistream_if_receive.tdata,
      m_axis_receive_tvalid  => axistream_if_receive.tvalid,
      m_axis_receive_tready  => axistream_if_receive.tready);

  i_axistream_vvc_transmit : entity bitvis_vip_axistream.axistream_vvc
    generic map (
      GC_VVC_IS_MASTER => true,    -- master drives the bus
      GC_DATA_WIDTH => 8, GC_USER_WIDTH => 1, GC_ID_WIDTH => 1, GC_DEST_WIDTH => 1,
      GC_INSTANCE_IDX => GC_AXIS_VVC_TRANSMIT_IDX)
    port map (clk => clk, axistream_vvc_if => axistream_if_transmit);

  i_axistream_vvc_receive : entity bitvis_vip_axistream.axistream_vvc
    generic map (
      GC_VVC_IS_MASTER => false,   -- slave samples the bus
      GC_DATA_WIDTH => 8, GC_USER_WIDTH => 1, GC_ID_WIDTH => 1, GC_DEST_WIDTH => 1,
      GC_INSTANCE_IDX => GC_AXIS_VVC_RECEIVE_IDX)
    port map (clk => clk, axistream_vvc_if => axistream_if_receive);

  i_uart_vvc : entity bitvis_vip_uart.uart_vvc
    generic map (GC_DATA_WIDTH => 8, GC_INSTANCE_IDX => GC_UART_VVC_IDX)
    port map (uart_vvc_rx => dut_txd, uart_vvc_tx => dut_rxd);

  i_clock_generator_vvc : entity bitvis_vip_clock_generator.clock_generator_vvc
    generic map (
      GC_INSTANCE_IDX    => 0,
      GC_CLOCK_NAME      => "Clock 1",
      GC_CLOCK_PERIOD    => C_CLK_PERIOD,
      GC_CLOCK_HIGH_TIME => C_CLK_PERIOD / 2)
    port map (clk => clk);
end architecture;
```

Notes on master/slave selection for symmetric VVCs:

| VVC | `GC_VVC_IS_MASTER` semantics |
|---|---|
| `axistream_vvc` (master) | Drives `tdata`/`tvalid`; reads `tready`. Use for `axistream_transmit`/`axistream_expect`. |
| `axistream_vvc` (slave) | Drives `tready`; samples `tdata`/`tvalid`. Use for `axistream_receive`/`axistream_expect`. |

The UART VVC is multi-channel — instance one entity per UART; it handles TX and RX internally through `t_channel`.

The clock-generator VVC is mandatory if you want any other VVC to function — those VVCs all use the clock supplied by it.

## File 2: the testbench (`<dut>_uvvm_tb.vhd`)

Responsibilities:

1. Same context imports as the harness, plus whatever you use *inside* `p_sequencer`.
2. The `GC_TESTCASE : string` generic that HDLRegression injects.
3. Constants for each VVC index (so the sequencer and the harness agree on which instance is which).
4. Instantiate the harness, passing the indices through.
5. `p_sequencer` process containing the whole test.

Skeleton (matches `examples/axistream_uart/src/tb/axistream_uart_uvvm_tb.vhd`):

```vhdl
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

library bitvis_vip_axistream;
context bitvis_vip_axistream.vvc_context;

library bitvis_vip_uart;
context bitvis_vip_uart.vvc_context;

library bitvis_vip_clock_generator;
context bitvis_vip_clock_generator.vvc_context;

entity <dut>_uvvm_tb is
  generic (
    GC_TESTCASE : string);
end entity;

architecture tb of <dut>_uvvm_tb is
  constant C_SCOPE      : string := "<DUT>_UVVM_TB";
  constant C_CLK_PERIOD : time   := 20 ns;

  constant C_AXIS_VVC_TRANSMIT_IDX : natural := 0;
  constant C_AXIS_VVC_RECEIVE_IDX  : natural := 1;
  constant C_UART_VVC_IDX          : natural := 0;

  signal arst : std_logic := '0';
begin
  i_test_harness : entity work.<dut>_uvvm_th
    generic map (
      GC_AXIS_VVC_TRANSMIT_IDX => C_AXIS_VVC_TRANSMIT_IDX,
      GC_AXIS_VVC_RECEIVE_IDX  => C_AXIS_VVC_RECEIVE_IDX,
      GC_UART_VVC_IDX          => C_UART_VVC_IDX)
    port map (arst => arst);

  p_sequencer : process
    variable v_axistream_bfm_config : t_axistream_bfm_config := C_AXISTREAM_BFM_CONFIG_DEFAULT;
    variable v_uart_bfm_config      : t_uart_bfm_config      := C_UART_BFM_CONFIG_DEFAULT;

    procedure test_case_uart_transmit (void : t_void) is
      constant data : t_slv_array(0 to 3)(7 downto 0) := (x"1A", x"FC", x"37", x"E2");
    begin
      axistream_transmit(AXISTREAM_VVCT, C_AXIS_VVC_TRANSMIT_IDX, data, "send via AXI-Stream");
      for i in 0 to 3 loop
        uart_expect(UART_VVCT, C_UART_VVC_IDX, RX, data(i), "verify UART output");
      end loop;
      await_completion(AXISTREAM_VVCT, C_AXIS_VVC_TRANSMIT_IDX, 1 ms, C_SCOPE);
      await_completion(UART_VVCT, C_UART_VVC_IDX, RX, 4 ms, C_SCOPE);
    end procedure;

    -- … more test_case_* procedures …

  begin
    -- Per-testcase log/alert files
    set_log_file_name(GC_TESTCASE & "_Log.txt");
    set_alert_file_name(GC_TESTCASE & "_Alert.txt");

    report_global_ctrl(VOID);
    report_msg_id_panel(VOID);
    enable_log_msg(ALL_MESSAGES);

    -- 1. Wait for UVVM engine to finish initialising
    await_uvvm_initialization(VOID);

    -- 2. Start clock(s) and pulse reset
    start_clock(CLOCK_GENERATOR_VVCT, 0, "Start system clock");
    gen_pulse(arst, '1', 10 * C_CLK_PERIOD, "10-cycle active-high reset");
    wait for 10 * C_CLK_PERIOD;

    -- 3. Tune verbosity for the test
    disable_log_msg(ALL_MESSAGES);
    enable_log_msg(ID_SEQUENCER);
    enable_log_msg(ID_LOG_HDR);

    -- 4. Configure BFMs of each VVC (clock_period, bit_time, check_packet_length, etc.)
    v_axistream_bfm_config.check_packet_length := false;
    v_axistream_bfm_config.clock_period        := C_CLK_PERIOD;
    v_axistream_bfm_config.ready_default_value := '1';
    shared_axistream_vvc_config(C_AXIS_VVC_TRANSMIT_IDX).bfm_config := v_axistream_bfm_config;
    shared_axistream_vvc_config(C_AXIS_VVC_RECEIVE_IDX).bfm_config  := v_axistream_bfm_config;

    v_uart_bfm_config.parity   := PARITY_NONE;
    v_uart_bfm_config.bit_time := (1 sec) / 115200;
    shared_uart_vvc_config(RX, C_UART_VVC_IDX).bfm_config := v_uart_bfm_config;
    shared_uart_vvc_config(TX, C_UART_VVC_IDX).bfm_config := v_uart_bfm_config;

    -- 5. Dispatch on GC_TESTCASE
    log(ID_SEQUENCER, "Running testcase: " & GC_TESTCASE, C_SCOPE);
    if    GC_TESTCASE = "TC_UART_TRANSMIT" then test_case_uart_transmit(VOID);
    elsif GC_TESTCASE = "TC_UART_RECEIVE"  then test_case_uart_receive(VOID);
    -- elsif ...
    end if;

    -- 6. Finish
    wait for 1000 ns;
    report_alert_counters(FINAL);
    log(ID_LOG_HDR, "SIMULATION COMPLETED", C_SCOPE);
    std.env.stop;
    wait;
  end process p_sequencer;
end architecture;
```

### Standard sequencer recipe (always in this order)

1. `set_log_file_name(...)` / `set_alert_file_name(...)` — per-testcase log files; usually parameterised on `GC_TESTCASE`.
2. `enable_log_msg(ALL_MESSAGES)` + `report_global_ctrl(VOID)` + `report_msg_id_panel(VOID)` — log the initial config so the log is self-describing.
3. `await_uvvm_initialization(VOID)` — must be called before *any* VVC procedure.
4. `start_clock(CLOCK_GENERATOR_VVCT, idx, "msg")` — bring clocks live.
5. `gen_pulse(arst, '1', 10 * C_CLK_PERIOD, "msg")` — reset the DUT.
6. Tune verbosity: `disable_log_msg(ALL_MESSAGES)` + `enable_log_msg(ID_SEQUENCER)` + `enable_log_msg(ID_LOG_HDR)` is the standard "quiet but readable" preset.
7. Apply BFM config to each `shared_<name>_vvc_config(...)`. **This must come before any VVC procedure call**, otherwise the BFM uses defaults that probably don't match the DUT.
8. `log(ID_SEQUENCER, "Running testcase: " & GC_TESTCASE, C_SCOPE)` then dispatch on `GC_TESTCASE`.
9. `wait for 1000 ns; report_alert_counters(FINAL); log(ID_LOG_HDR, "SIMULATION COMPLETED", C_SCOPE); std.env.stop; wait;`

### Test-case dispatch pattern

Wrap each test in a procedure declared inside the sequencer:

```vhdl
procedure test_case_<name>(void : t_void) is
   -- locals
begin
   -- test body
end procedure test_case_<name>;
```

Then dispatch with an `if/elsif` chain on `GC_TESTCASE`. Use upper-case `TC_*` names — `TC_UART_TRANSMIT`, `TC_COSIM`, etc. — matched verbatim against `GC_TESTCASE`. HDLRegression will run one simulation per test case and feed the name in via the generic.

The example also uses `GC_TESTCASE` outside the sequencer to feature-gate the harness:

```vhdl
GC_COSIM_ENABLE => (GC_TESTCASE = "TC_COSIM")
```

That's how the example turns on the cosim entity only for the cosim testcase.

## File 3: the HDLRegression `run.py`

Pattern (mirrors `examples/axistream_uart/scripts/run.py`):

```python
import os, sys, pathlib

project_path = pathlib.Path(__file__).resolve().parent.parent  # adjust as needed
repo_path    = project_path.parent.parent                       # path to repo root, holding thirdparty/

sys.path.append(str(repo_path / "thirdparty" / "hdlregression"))
from hdlregression import HDLRegression

def add_project_files(hr):
    # UVVM core
    hr.add_files(repo_path / "thirdparty/uvvm/uvvm_util/src/*.vhd",          "uvvm_util")
    hr.add_files(repo_path / "thirdparty/uvvm/uvvm_vvc_framework/src/*.vhd", "uvvm_vvc_framework")
    hr.add_files(repo_path / "thirdparty/uvvm/bitvis_vip_scoreboard/src/*.vhd", "bitvis_vip_scoreboard")

    # One stanza per VVC used in this testbench. Crucially: each VVC library
    # needs BOTH its own src/ files AND the framework's src_target_dependent/ files
    # compiled into it.
    hr.add_files(repo_path / "thirdparty/uvvm/bitvis_vip_axistream/src/*.vhd",            "bitvis_vip_axistream")
    hr.add_files(repo_path / "thirdparty/uvvm/uvvm_vvc_framework/src_target_dependent/*.vhd", "bitvis_vip_axistream")

    hr.add_files(repo_path / "thirdparty/uvvm/bitvis_vip_uart/src/*.vhd",                 "bitvis_vip_uart")
    hr.add_files(repo_path / "thirdparty/uvvm/uvvm_vvc_framework/src_target_dependent/*.vhd", "bitvis_vip_uart")

    hr.add_files(repo_path / "thirdparty/uvvm/bitvis_vip_clock_generator/src/*.vhd",      "bitvis_vip_clock_generator")
    hr.add_files(repo_path / "thirdparty/uvvm/uvvm_vvc_framework/src_target_dependent/*.vhd", "bitvis_vip_clock_generator")

    # DUT
    hr.add_files(project_path / "src/rtl/*.vhd", "work")

    # Testbench + harness (work library; HDLRegression auto-detects which is the tb via the `-- hdlregression:tb` marker)
    hr.add_files(project_path / "src/tb/<dut>_uvvm_tb.vhd", "work")
    hr.add_files(project_path / "src/tb/<dut>_uvvm_th.vhd", "work")

def main():
    hr = HDLRegression()
    if hr.settings.get_simulator_name() not in ("NVC", "MODELSIM"):
        print(f"Unsupported simulator {hr.settings.get_simulator_name()}")
        return -1
    add_project_files(hr)

    if hr.settings.get_simulator_name() == "NVC":
        # NVC defaults of -H64m/-M64m are too small for UVVM.
        global_opts = [o for o in hr.settings.get_global_options() if "-H" not in o and "-M" not in o]
        global_opts += ["-H1g", "-M1g"]
        hr.settings.set_global_options(global_opts)
        return hr.start()
    else:
        return hr.start()

if __name__ == "__main__":
    sys.exit(main())
```

Key facts about HDLRegression:

- It picks the simulator from the environment — there's no `--simulator` flag in `run.py`. Set `NVC=/opt/nvc/...` / `MODELSIM=...` env vars or rely on `which`. The exact pattern is in the project root `run.py`.
- **Library mapping is per-file**, second arg of `hr.add_files`. The single hardest gotcha: you must compile `uvvm_vvc_framework/src_target_dependent/*.vhd` *separately into every VVC library* (`bitvis_vip_axistream`, `bitvis_vip_uart`, …). That's how the framework's td_ files get bound to each VVC's `vvc_cmd_pkg` / `vvc_methods_pkg`.
- Mark the testbench entity file with `-- hdlregression:tb` near the top. Without it, HDLRegression won't pick it as a runnable testbench.
- Test cases are discovered from the `if/elsif GC_TESTCASE = "..."` chain inside the testbench. HDLRegression parses the source, finds the strings, and runs one simulation per case.
- For ad-hoc shortcut compilation of all of UVVM: `hr.compile_uvvm("thirdparty/uvvm/")` does the job in one call, at the cost of compiling unused VIPs.
- For NVC's foreign loading or Modelsim's FLI loading (cosim use case only), pass `sim_options=...` to `hr.start()`. See the project root `run.py` for the exact incantation.

## Running it

From the directory containing `run.py`:

```
python3 run.py             # run all testcases
python3 run.py -tc TC_X    # one testcase
python3 run.py -g          # open the simulator GUI (Modelsim)
```

Outputs land in `<scripts-dir>/hdlregression/` — per-testcase log files, transcripts, and an XML report (if you called `hr.gen_report(...)`).

## Quick checklist when adding a new testbench

- [ ] DUT compiled into `work`.
- [ ] Harness file declares the right BFM-signal-record subtypes (constraining widths of `tdata` etc.).
- [ ] `ti_uvvm_engine` instantiated exactly once in the harness.
- [ ] Every VVC has a unique `GC_INSTANCE_IDX`. Clock-generator VVC included.
- [ ] Testbench has `-- hdlregression:tb` marker and a `GC_TESTCASE : string` generic.
- [ ] `p_sequencer` calls `await_uvvm_initialization(VOID)` before anything else, then `start_clock(...)`, then `gen_pulse(arst, ...)`, then BFM config, then test dispatch.
- [ ] Each `test_case_*` calls `await_completion(...)` on every VVC it queued commands on before returning.
- [ ] `run.py` compiles `src_target_dependent/*.vhd` into *every* VVC library used.
- [ ] `run.py` adds both `_tb.vhd` and `_th.vhd` to `work`.
