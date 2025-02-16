#include <iostream>
#include <deque>
#include <exception>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include <vhpi_user.h>
#include "uvvm_cosim_utils.hpp"
#include "uvvm_cosim_server.hpp"
#include "uvvm_cosim_types.hpp"

extern "C" {

// ----------------------------------------------------------------------------
// SERVER FUNCTIONS
// ----------------------------------------------------------------------------

// TODO: Use shared or unique pointer?
static UvvmCosimServer* cosim_server;

void start_rpc_server(void)
{
  // Todo:
  // Use a foreign function and call after UVVM init instead?
  // Then we can set port in a generic in VHDL code

  cosim_server = new UvvmCosimServer(8484);

  std::cout << "Start JSON RPC server" << std::endl;
  cosim_server->StartListening();
}

void stop_rpc_server(void)
{
  std::cout << "Stop JSON RPC server" << std::endl;
  cosim_server->StopListening();
  std::cout << "JSON RPC server stopped" << std::endl;
}


// ----------------------------------------------------------------------------
// VHPI foreign functions, procedures, and callbacks
// ----------------------------------------------------------------------------

//int vhpi_cosim_transmit_queue_empty(const char* vvc_type, int vvc_instance_id)
void vhpi_cosim_transmit_queue_empty(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_cb_string_param_by_index(p_cb_data, 0);
  int vvc_instance_id = get_vhpi_cb_int_param_by_index(p_cb_data, 1);

  bool empty = cosim_server->TransmitQueueEmpty(vvc_type, vvc_instance_id);

  set_vhpi_int_retval(p_cb_data, empty ? 1 : 0);
}

//int vhpi_cosim_transmit_queue_get(const char* vvc_type, int vvc_instance_id)
void vhpi_cosim_transmit_queue_get(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_cb_string_param_by_index(p_cb_data, 0);
  int vvc_instance_id = get_vhpi_cb_int_param_by_index(p_cb_data, 1);

  auto byte = cosim_server->TransmitQueueGet(vvc_type, vvc_instance_id);

  if (byte) {
    int data = byte.value().first;
    data |= byte.value().second << 9;
    
    set_vhpi_int_retval(p_cb_data, data);
  } else {
    // TODO:
    // Kill simulation if TransmitQueueGet didn't return anything?
    set_vhpi_int_retval(p_cb_data, 0); // Return zero for now
  }
}

//void vhpi_cosim_receive_queue_put(const char* vvc_type, int vvc_instance_id, uint8_t byte, bool end_of_packet)
void vhpi_cosim_receive_queue_put(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_cb_string_param_by_index(p_cb_data, 0);
  int vvc_instance_id = get_vhpi_cb_int_param_by_index(p_cb_data, 1);
  uint8_t byte = get_vhpi_cb_int_param_by_index(p_cb_data, 2);
  bool end_of_packet = get_vhpi_cb_int_param_by_index(p_cb_data, 3) == 1 ? true : false;

  cosim_server->ReceiveQueuePut(vvc_type, vvc_instance_id, byte, end_of_packet);
}

void vhpi_cosim_start_sim(const vhpiCbDataT* p_cb_data)
{
  vhpi_printf("vhpi_cosim_start_sim: Waiting to start sim");
  cosim_server->WaitForStartSim();
  vhpi_printf("vhpi_cosim_start_sim: Starting sim");
}

void vhpi_cosim_listen_enable(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_cb_string_param_by_index(p_cb_data, 0);
  std::string vvc_channel = get_vhpi_cb_string_param_by_index(p_cb_data, 1);
  int vvc_instance_id = get_vhpi_cb_int_param_by_index(p_cb_data, 2);

  bool listen = cosim_server->VvcListenEnabled(vvc_type, vvc_channel, vvc_instance_id);

  set_vhpi_int_retval(p_cb_data, listen ? 1 : 0);
}

void vhpi_cosim_report_vvc_info(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_cb_string_param_by_index(p_cb_data, 0);
  std::string vvc_channel = get_vhpi_cb_string_param_by_index(p_cb_data, 1);
  int vvc_instance_id = get_vhpi_cb_int_param_by_index(p_cb_data, 2);
  std::string vvc_cfg_str = get_vhpi_cb_string_param_by_index(p_cb_data, 3);

  vhpi_printf("vhpi_cosim_report_vvc_info: Got:");
  vhpi_printf("Type=%s, Channel=%s, ID=%d, cfg=%s",
	      vvc_type.c_str(),
	      vvc_channel.c_str(),
	      vvc_instance_id,
	      vvc_cfg_str.c_str());

  cosim_server->AddVvc(vvc_type, vvc_channel, vvc_instance_id, vvc_cfg_str);
}

long convert_time_to_ns(const vhpiTimeT *time)
{
  return (((long)time->high << 32) | (long)time->low) / 1000000;
}

void start_of_sim_cb(const vhpiCbDataT * cb_data) {
  vhpi_printf("Start of simulation");
  start_rpc_server();
}

void end_of_sim_cb(const vhpiCbDataT * cb_data) {
  vhpiTimeT t;
  long cycles;

  vhpi_get_time(&t, &cycles);
  vhpi_printf("End of simulation (after %ld cycles and %ld ns).", cycles, convert_time_to_ns(&t));

  stop_rpc_server();
}

void startup_register_foreign_methods(void)
{
  vhpi_printf("startup_register_foreign_methods() called");

  const char* c_lib_name = "uvvm_cosim_lib";

  register_vhpi_foreign_method(vhpi_cosim_report_vvc_info,
			       "vhpi_cosim_report_vvc_info",
			       c_lib_name,
			       vhpiProcF);

  register_vhpi_foreign_method(vhpi_cosim_start_sim,
			       "vhpi_cosim_start_sim",
			       c_lib_name,
			       vhpiProcF);

  register_vhpi_foreign_method(vhpi_cosim_listen_enable,
			       "vhpi_cosim_listen_enable",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(vhpi_cosim_transmit_queue_empty,
			       "vhpi_cosim_transmit_queue_empty",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(vhpi_cosim_transmit_queue_get,
			       "vhpi_cosim_transmit_queue_get",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(vhpi_cosim_receive_queue_put,
			       "vhpi_cosim_receive_queue_put",
			       c_lib_name,
			       vhpiProcF);

  vhpi_printf("Registered all foreign functions/procedures");
}

void startup_register_callbacks()
{
  vhpi_printf("startup_register_callbacks() called");

  vhpiCbDataT cb_data;

  cb_data.reason = vhpiCbStartOfSimulation;
  cb_data.cb_rtn = start_of_sim_cb;
  cb_data.obj = NULL;
  cb_data.time = NULL;
  cb_data.value = NULL;
  cb_data.user_data = NULL;

  vhpi_register_cb(&cb_data, 0);

  cb_data.reason = vhpiCbEndOfSimulation;
  cb_data.cb_rtn = end_of_sim_cb;

  static vhpiTimeT t = {.high = 0, .low = 250};
  cb_data.time = &t;

  vhpi_register_cb(&cb_data, 0);
}


void (*vhpi_startup_routines[])() = {
  startup_register_callbacks,
  startup_register_foreign_methods,
  NULL
};
}
