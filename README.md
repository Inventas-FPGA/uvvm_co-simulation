# Co-simulation library for UVVM

This library aims to make it easy to setup basic co-simulation for UVVM testbench, by enabling communication with your simulated DUT using VVCs instances in your testbench. Very few changes are required to integrate this with an existing testbench.

It uses either the standard VHDL VHPI or Modelsim/Questasim FLI, to start a JSON-RPC server in a dedicated thread. From a JSON-RPC client, accessed from your application, you can:
- Get a list of available VVCs in the simulation
- Transmit and receive data to any supported VVC

The server maintains a set of queues, one for transmit and one for receive, for each VVC in the simulation. Transmit data sent from the client is put in a transmit queue by the server. And when the client makes a request for received data, the server responds with data that is available in the receive queue.
On the VHDL side, an entity called `uvvm_cosim` uses foreign function/procedure calls to access the same queues and basically forwards this to and from the VVCs using their transmit and receive procedures.

The VHPI/FLI code and JSON-RPC server is written in C++ and there is a basic C++ client as well. But there is also example client code for Python, and the protocol is very simple and should be easy to implement in a custom client.

## Simulator support

Currently there is support for:
- Modelsim/Questasim using FLI
- NVC using VHPI

It may also work with other simulators that support VHPI, but has only been tested with NVC at the moment. And note that older commits of NVC than [3b81846](https://github.com/nickg/nvc/commit/3b81846) may not work, so you need a recent version of NVC.

Build of the FLI version of the library is also disabled by default since that requires the proprietary `mti.h` header supplied with Modelsim/Questasim. These headers may be added to the project since they are apparently distributed with a license that allows redistribution. More info on building with FLI support below.

## Integration in an existing testbench

Assuming you have a UVVM testbench with VVCs setup already, the basic steps to integrate this library is as follows.

1. Create an instance of `uvvm_cosim` in your test bench/harness:

```
inst_uvvm_cosim: entity work.uvvm_cosim
    generic map (
      GC_COSIM_EN => true)
    port map (
      clk             => clk,
      vvc_config_done => vvc_config_done);
```

You can use any clock for the cosim module, but note that the cosim module will poll the cosim server for data to transmit every cycle of this clock. Running the cosim module at the "main system clock" may slow down your simulation, so it may be beneficial to run this module at a slower clock (with the tradeoff that transmit transactions are scheduled less frequently).

The vvc\_config\_done signal should be set in your testbench/sequencer process after you have finished configuring all VVCs/BFMs. The cosim module will initially wait for this signal to go high.

2. Add a test case or mode where your DUT is in a "free-running" mode.

It should be reset, initialized, have all the necessary clocks and be ready to use. Your testbench sequencer process should either run "forever" or with a very long timeout. You should not drive stimuli to the DUT using the VVC instances you plan to access with the cosim library (but it's fine to drive other necessary stimuli, for example some input data for your DUT to process if you don't want to do this via co-sim). The `GC_COSIM_EN` generic parameter can be used to enable the co-simulation module only for the co-simulation test case.

3. Make sure VVCs/BFMs are configured appropriately for co-sim

**UART VVC:**
Set infinite receive timeout (which is also the default):
```
shared_uart_vvc_config(RX, VVC_ID).bfm_config.timeout := 0 ns;
``` 

**AXISTREAM VVC:**
This VVC does not support infinite "timeout". But the co-sim library will automatically restart receive transactions as they timeout. So, just choose some value for `max_wait_cycles` that is appropriate for your DUT:
```
shared_axistream_vvc_config(VVC_ID).bfm_config.max_wait_cycles := 1000;
```

Since receive will likely timeout frequently (depending on how high you set max wait), you will have to set `max_wait_cycles_severity` to not trigger an error, which would cause UVVM to end the simulation. Your options are `NO_ALERT` or `NOTE`.
```
shared_axistream_vvc_config(VVC_ID).bfm_config.max_wait_cycles_severity := NO_ALERT;
```

Packet based AXI-Stream (ie. using TLAST) **is** supported. This is automatically detected from the VVC's configuration:
```
shared_axistream_vvc_config(VVC_ID).bfm_config.check_packet_length := true;
```

Note that for AXI-Stream VVCs with `check_packet_length=false` you must use the `TransmitBytes` and `ReceiveBytes` JSON-RPC methods, while for packet-based VVCs (`check_packet_length=true`) you have to use the `TransmitPacket` and `ReceivePacket` methods.

4. Compile and run your simulation

Add the co-sim VHDL sources to your project. They reside under `src/vhdl`. And compile the C++ cosim library (see instructions below).

When the simulator is run you will have to load the shared library. The parameter for this is `--load=path/to/libuvvm_cosim_vhpi.so` if you are using NVC.


## Build instructions for C++ co-sim library

Clone this repo and initialize submodules:
```
git submodule update --init
```

This will initialize all three submodules used in this project:
- json-rpc-cxx
- hdlregression
- uvvm

Technically, only json-rpc-cxx is needed to build the C++ library, so you can save some time by only updating that submodule. But to run the example in this repo you will need all of them.
The CMake build also expects to find NVC under `/opt/nvc` at the moment (it does not search anywhere else).

To build the C++ library and C++ example client:

```
mkdir build
cd build
cmake ..
make
```

This will build the VHPI version of the library, `libuvvm_cosim_vhpi.so`, and `uvvm_cosim_example_client` which can be used to run an example testbench (more below).

**Building Modelsim/Questasim FLI library**

To build the FLI version of the library you must enable the `BUILD_VSIM_FLI` option when running CMake, e.g.:
```
cmake .. -DBUILD_VSIM_FLI=ON
```

That should build `libuvvm_cosim_fli.so` in addition to the VHPI version of the library.


### Example testbench

There is a very basic example of a testbench, `test/cosim_testbench.vhd`, which sets up two UART VVCs and two AXI-Stream VVCs back to back. The example client can be used to communicate with these.
Note that if you want to run the testbench in this repo, you need to have initialized the hdlregression uvvm submodules, **and you will need nvc in your PATH**.

To build the example testbench using NVC:
```
make hdl_build_nvc
```

To start the simulation and JSON-RPC co-sim server using NVC:
```
make hdl_sim_nvc
```

Or, using Modelsim or Questasim, the testbench can be built and run using either the `hdl_sim_vsim` target or `hdl_sim_vsim_gui` target if you want to run in GUI mode.

While the simulation is running and the server is listening, you can test the example client by running (also from the build directory):
```
./uvvm_cosim_example_client
```

There are also two example clients for Python under `src/python`. One using the `requests` library and another using `tinyrpc-lib`.


### Unit tests

To build unit tests for the C++ code set the `ENABLE_UNIT_TESTS` switch when running cmake and optionally `ENABLE_COVERAGE` to generate coverage info.

```
mkdir build
cd build
cmake .. -DENABLE_UNIT_TESTS=On -DENABLE_COVERAGE=On
make
```

To run all unit tests, execute `make test` or run `ctest`. Or, to run a unit test binary directly, from the build folder run:
```
test/cpp/test_byte_queue
test/cpp/test_packet_queue
test/cpp/test_uvvm_cosim_data
test/cpp/test_uvvm_cosim_types
```

To generate coverage results (requires `ENABLE_COVERAGE` option), run `make cov`. This will generate an html report of unit test coverage under `cov/index.html`.

**Note that coverage requires `gcov` and `lcov`.**


# JSON-RPC protocol

None of the JSON-RPC methods are "blocking" in the sense that they will immediately return a response and not wait for the actual request to be completed. For transmit calls, this means the data is queued up in the cosim-server, and gradually transmitted as the simulation progresses. For VVCs or VVC channels that can receive, and which have listening enabled, received data is stored in a queue in the cosim-server. Calls to the JSON-RPC receive methods will also return a response immediately, and this response will either include the requested amount of bytes if the queue has sufficient data, or, if the queue has less data available than requested, the method will return either what is available or none at all depending on what parameters it was called with.

## JSON-RPC request format

Standard JSON-RPC 2.0 where the `method` field specifies the remote procedure to call, and the `params` field contains parameters to the procedure. Some example request:

```
{"id":1,"jsonrpc":"2.0","method":"GetVvcList","params":[]}
```

```
{"id":2,"jsonrpc":"2.0","method":"TransmitBytes","params":["UART_VVC",0,[7,8,9,10,11,12]]}
```

```
{"id":3,"jsonrpc":"2.0","method":"ReceiveBytes","params":["UART_VVC",1,12,false]}
```


## JSON-RPC response format

JSON response from a RPC call which doesn't return other data:

```
{
  "success": true/false,
  "result": {},
  "id": id
}
```

ID matches the request ID. The `result` field will be empty in responses from requests that don't return any data. It will also be empty for failed requests (in this case the `success` field is false).

> In principle, the `result` field could have been "null" or omitted for responses that don't have results or for failed requests. But "null" value fields were not used since they can be a bit tricky to deal with in different languages. Example here using null for a string value in nlohmann's JSON for Modern C++ (the library used by json-rpc-cxx): https://github.com/nlohmann/json/discussions/4003

For RPC calls that should return data the result field should contain a JSON object with the results (so, the structure of the `result` field depends on which remote procedure was called). Below is a typical example for a `receive_bytes` call:

```
{
  "success": true,
  "result": {
    data: [10, 20, 30, 40]
  },
 "id": 1
}
```

Another example, for a call to get the list of VVCs in the simulation:

```
{
  "success": true,
  "result": [
    {
      "vvc_type": "UART_VVC",
      "vvc_channel": "RX",
      "vvc_instance_id": 0,
      "listen_enable": false,
      "cosim_support": true,
      "packet_based": false,
      "bfm_cfg": {"timeout": 0, "baudrate": 115200}
    },
    {
      "vvc_type": "UART_VVC",
      "vvc_channel": "TX",
      "vvc_instance_id": 0,
      "listen_enable": false,
      "cosim_support": true,
      "packet_based": false,
      "bfm_cfg": {"baudrate": 115200}
    },
    {
      "vvc_type": "AXISTREAM_VVC",
      "vvc_channel": "NA",
      "vvc_instance_id": 0,
      "listen_enable": false,
      "cosim_support": true,
      "packet_based": false,
      "bfm_cfg": {"max_wait_cycles": 1000}
    },
    {
      "vvc_type": "AXISTREAM_VVC",
      "vvc_channel": "NA",
      "vvc_instance_id": 1,
      "listen_enable": true,
      "cosim_support": true,
      "packet_based": true,
      "bfm_cfg": {"max_wait_cycles": 1000}
    }
  ],
  "id": 1
}
```

Only VVC types supported by the co-sim library appear in this list (unsupported types such as `CLOCK_GENERATOR_VVC` are skipped, and a warning is printed to stderr). Each entry includes `listen_enable` (whether the server is currently queuing received data for that VVC), `cosim_support` (set from the VHDL BFM config), `packet_based` (true for packet-framed VVCs using TLAST/EOP), and a `bfm_cfg` map of the remaining BFM configuration values reported by the VHDL side.

# JSON-RPC methods

TODO:

| Method name | Parameters | Return value | Supported VVCs | Description |
|:------------|:-----------|:-------------|:---------------|:------------|
|             |            |              |                |             |
|             |            |              |                |             |
|             |            |              |                |             |
|             |            |              |                |             |


## Transmit and receive bytes

`TransmitBytes(VVC_TYPE, VVC_ID, [bytes])`
`ReceiveBytes(VVC_TYPE, VVC_ID, num_bytes, exact_length)`

Supported VVCs:

- UART VVC
- AXISTREAM VVC with check\_packet\_length disabled in config
- AVALON-ST (planned) with use\_packet\_transfer disabled in config

## Transmit and receive packet

`TransmitPacket(VVC_TYPE, VVC_ID, [packet])`
`ReceivePacket(VVC_TYPE, VVC_ID)`

Supported VVCs:
- AXISTREAM VVC with check\_packet\_length enabled in config
- AVALON-ST (planned) with use\_packet\_transfer enabled in config

## Memory-mapped VVC methods

These methods are for VVCs that use address-based read/write transfers (AXI, AXI-Lite, Avalon-MM). Requests are non-blocking: `SendWriteRequest` and `SendReadRequest` enqueue a transfer, while `GetWriteResponse` and `GetReadResponse` poll for the corresponding response from the simulation.

`SendWriteRequest(VVC_TYPE, VVC_ID, addr, [data], addr_increment)`

`GetWriteResponse(VVC_TYPE, VVC_ID)` — returns `{"available": true/false}`

`SendReadRequest(VVC_TYPE, VVC_ID, addr, num_bytes)`

`GetReadResponse(VVC_TYPE, VVC_ID)` — returns `{"data": [...]}` (empty array if response not yet available)

> Note: VHDL VVC controllers for memory-mapped buses are not yet implemented. These JSON-RPC methods are available on the server side but require a corresponding VHDL controller to drive the actual bus transactions.

Supported VVCs (once VHDL controllers are available):
- AXI\_VVC
- AXILITE\_VVC
- AVALON\_MM\_VVC

## Note on VVC channel

It is not necessary to specify a VVC channel when calling any of the transmit or receive methods (`TransmitBytes`, `TransmitPacket`, `ReceiveBytes`, `ReceivePacket`, `SendWriteRequest`, etc.). The channel is determined by context: for UART the server internally routes to the TX channel for transmit and the RX channel for receive; for all other VVC types the channel is always NA.
