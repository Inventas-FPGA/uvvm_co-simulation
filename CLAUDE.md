# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Co-simulation library for UVVM (Universal VHDL Verification Methodology). It embeds a JSON-RPC HTTP server inside a VHDL simulation (via VHPI or Modelsim FLI), allowing external clients (C++, Python, or any HTTP client) to drive UVVM VVCs without modifying the testbench sequencer.

## Build commands

Prerequisites: submodules initialized (`git submodule update --init`), NVC at `/opt/nvc`, Questa/Modelsim at `/opt/intelFPGA_pro/24.3.1/questa_fse`.

```bash
# Build C++ libraries and example client (from repo root)
mkdir build && cd build
cmake ..
make

# Build with FLI support for Modelsim/Questasim
cmake .. -DBUILD_VSIM_FLI=ON && make

# Build and run VHDL testbench (from build/)
make hdl_build_nvc       # compile VHDL with NVC
make hdl_sim_nvc         # compile + run simulation with NVC
make hdl_sim_vsim        # compile + run with Modelsim/Questasim
make hdl_sim_vsim_gui    # compile + run in Questa GUI
```

## Unit tests

```bash
# Enable tests (default ON) and optionally coverage
cmake .. -DENABLE_UNIT_TESTS=On -DENABLE_COVERAGE=On
make

# Run all tests
make test   # or: ctest

# Run a single test binary directly (from build/)
./test/cpp/test_byte_queue
./test/cpp/test_packet_queue
./test/cpp/test_uvvm_cosim_data
./test/cpp/test_uvvm_cosim_types

# Generate HTML coverage report (requires lcov)
make cov    # output at build/cov/index.html
```

Tests use Catch2 (fetched automatically by CMake).

## Architecture

### Data flow

```
External client (Python/C++)
        |  JSON-RPC over HTTP
        v
UvvmCosimServer   (src/cpp/uvvm_cosim_server.cpp)
        |  reads/writes
        v
UvvmCosimData     (src/cpp/uvvm_cosim_data.cpp)
  VvcMap: mutex-protected map of VvcInstanceKey → VvcInstanceData
  Each VvcInstanceData holds:
    - byte_queues[QID_TRANSMIT / QID_RECEIVE]   (for UART, non-packet AXI-Stream)
    - packet_queues[QID_TRANSMIT / QID_RECEIVE]  (for packet-based AXI-Stream)
        |  foreign VHPI/FLI calls (every clock cycle)
        v
uvvm_cosim.vhd  →  uvvm_cosim_uart_vvc_ctrl.vhd
               →  uvvm_cosim_axis_vvc_ctrl.vhd
                        |  UVVM VVC API
                        v
                   DUT via UART/AXI-Stream VVCs
```

### C++ layer

| File | Role |
|------|------|
| `uvvm_cosim_server.hpp/.cpp` | JSON-RPC server; exposes RPC methods to clients and accessor methods called from VHPI/FLI code |
| `uvvm_cosim_data.hpp/.cpp` | Thread-safe VVC map + byte/packet queues |
| `uvvm_cosim_types.hpp` | `VvcInstanceKey`, `VvcInstanceData`, `VvcConfig`, `JsonResponse`, JSON serialization |
| `uvvm_cosim_common.hpp/.cpp` | Thin bridge between simulator-specific entry points and the server singleton |
| `uvvm_cosim_foreign_vhpi.cpp` | VHPI startup routines, registers foreign functions/procedures via `vhpi_startup_routines[]` |
| `uvvm_cosim_foreign_fli.cpp` | FLI equivalent; built only when `BUILD_VSIM_FLI=ON` |
| `uvvm_cosim_client.hpp` | C++ client helper (used in `uvvm_cosim_client_example.cpp`) |
| `shared_map.hpp` | Mutex-guarded `std::map` wrapper |
| `byte_queue.hpp`, `packet_queue.hpp` | Single-VVC queue types |

### VHDL layer

| File | Role |
|------|------|
| `uvvm_cosim.vhd` | Top-level entity instantiated in the testbench. Iterates registered VVCs, reports their configs to the C++ server, then runs the clock loop that drives VVC ctrl entities. Generic `GC_COSIM_EN` gates everything. |
| `uvvm_cosim_uart_vvc_ctrl.vhd` | Per-VVC-index process pair: polls transmit queue → schedules `uart_transmit`; calls `uart_receive` → writes received bytes to receive queue |
| `uvvm_cosim_axis_vvc_ctrl.vhd` | Same pattern for AXI-Stream (supports both byte-stream and packet modes) |
| `uvvm_cosim_foreign_pkg_vhpi.vhd` | VHPI `attribute foreign` declarations binding VHDL subprograms to C symbols in `libuvvm_cosim_vhpi.so` |
| `uvvm_cosim_foreign_pkg_fli.vhd` | FLI equivalent |
| `uvvm_cosim_foreign_pkg_body.vhd` | Package body shared between VHPI and FLI variants |
| `uvvm_cosim_utils_pkg.vhd` | Helpers: `strcmp`, `bfm_cfg_to_string`, VVC max instance constants |

### HDL build runner

`run.py` uses HDLRegression to compile all VHDL sources (UVVM libraries + cosim lib + testbench) and launch the simulator. Simulator is selected by HDLRegression from the environment (NVC or MODELSIM). The `--load` option passes `libuvvm_cosim_vhpi.so` to NVC; for Modelsim the `-foreign` option loads `libuvvm_cosim_fli.so`.

### Thirdparty submodules

- `thirdparty/json-rpc-cxx` — JSON-RPC 2.0 library + `cpphttplibconnector` used by the server
- `thirdparty/uvvm` — UVVM util, VVC framework, UART VIP, AXI-Stream VIP, Clock Generator VIP
- `thirdparty/hdlregression` — Python HDL test runner (HDLRegression)

## VVC support

| VVC | Byte API | Packet API | Notes |
|-----|----------|------------|-------|
| UART | `TransmitBytes` / `ReceiveBytes` | — | UART RX requires `timeout=0 ns` (infinite) |
| AXISTREAM (non-packet) | `TransmitBytes` / `ReceiveBytes` | — | `check_packet_length=false` in BFM config |
| AXISTREAM (packet) | — | `TransmitPacket` / `ReceivePacket` | `check_packet_length=true` in BFM config |

`SetVvcListenEnable` must be called before the server will queue received data for a VVC.

## Python clients

```bash
pip install -r requirements.txt   # requests, tinyrpc, lcov_cobertura

python src/python/example_client_requests-lib.py
python src/python/example_client_tinyrpc-lib.py
```
