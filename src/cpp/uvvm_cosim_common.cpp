#include <optional>
#include <string>
#include "uvvm_cosim_server.hpp"
#include "uvvm_cosim_types.hpp"
#include "uvvm_cosim_common.hpp"

namespace uvvm_cosim {

// ----------------------------------------------------------------------------
// SERVER FUNCTIONS
// ----------------------------------------------------------------------------

// TODO: Use shared or unique pointer?
static UvvmCosimServer* cosim_server = nullptr;

static void start_rpc_server(void)
{
  // Todo:
  // Use a foreign function and call after UVVM init instead?
  // Then we can set port in a generic in VHDL code

  cosim_server = new UvvmCosimServer(8484);

  sim_printf("Start JSON RPC server");
  cosim_server->StartListening();
}

static void stop_rpc_server(void)
{
  sim_printf("Stop JSON RPC server");
  cosim_server->StopListening();
  sim_printf("JSON RPC server stopped");

  delete cosim_server;
  cosim_server = nullptr;
}


// ----------------------------------------------------------------------------
// VHPI foreign functions, procedures, and callbacks
// ----------------------------------------------------------------------------

bool transmit_byte_queue_empty(std::string vvc_type, int vvc_instance_id)
{
  return cosim_server->TransmitQueueEmpty(vvc_type, vvc_instance_id);
}

int transmit_byte_queue_get(std::string vvc_type, int vvc_instance_id)
{
  auto byte = cosim_server->TransmitQueueGet(vvc_type, vvc_instance_id);

  if (byte) {
    int data = byte.value();
    return data;
  } else {
    // TODO:
    // Kill simulation if TransmitQueueGet didn't return anything?
    sim_printf("Error: TransmitQueueGet did not return a byte");
    return 0;
  }
}

void receive_byte_queue_put(std::string vvc_type, int vvc_instance_id, uint8_t byte)
{
  cosim_server->ReceiveQueuePut(vvc_type, vvc_instance_id, byte);
}

bool transmit_packet_queue_empty(std::string vvc_type, int vvc_instance_id)
{
  return cosim_server->TransmitPacketQueueEmpty(vvc_type, vvc_instance_id);
}

int transmit_packet_queue_get(std::string vvc_type, int vvc_instance_id)
{
  auto byte = cosim_server->TransmitPacketQueueGet(vvc_type, vvc_instance_id);

  if (byte) {
    int data = byte.value().first;
    if (byte.value().second) {
      data |= (1 << 8); // 9th bit marks eop
    }
    return data;
  } else {
    // TODO:
    // Kill simulation if TransmitPacketQueueGet didn't return anything?
    sim_printf("Error: TransmitPacketQueueGet did not return a byte");
    return 0;
  }
}

void receive_packet_queue_put(std::string vvc_type, int vvc_instance_id,
			      uint8_t byte, bool eop)
{
  cosim_server->ReceivePacketQueuePut(vvc_type, vvc_instance_id, byte, eop);
}


void start_sim(void)
{
  cosim_server->WaitForStartSim();
}

bool terminate_sim(void)
{
  bool terminate = cosim_server->ShouldTerminateSim();

  if (terminate) {
    sim_printf("uvvm_cosim_terminate_sim: Terminating sim");
  }

  return terminate;
}

bool vvc_listen_enable(std::string vvc_type, int vvc_instance_id)
{
  return cosim_server->VvcListenEnabled(vvc_type, vvc_instance_id);
}

void report_vvc_info(std::string vvc_type,
				std::string vvc_channel,
				int vvc_instance_id,
				std::string bfm_cfg_str)
{
  sim_printf("uvvm_cosim_report_vvc_info: Got:");
  sim_printf("Type=%s, Channel=%s, ID=%d, cfg=%s",
	     vvc_type.c_str(),
	     vvc_channel.c_str(),
	     vvc_instance_id,
	     bfm_cfg_str.c_str());

  cosim_server->AddVvc(vvc_type, vvc_channel, vvc_instance_id, bfm_cfg_str);
}

void start_of_sim(void)
{
  sim_printf("Start of simulation");
  start_rpc_server();
}

void end_of_sim(long cycles, long time_ns)
{
  sim_printf("End of simulation (after %ld cycles and %ld ns).", cycles, time_ns);
  stop_rpc_server();
}

} // namespace uvvm_cosim

