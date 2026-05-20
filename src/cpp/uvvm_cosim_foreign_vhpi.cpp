#include <string>
#include <cstdint>
#include <vhpi_user.h>
#include "uvvm_cosim_vhpi_utils.hpp"
#include "uvvm_cosim_common.hpp"


long convert_time_to_ns(const vhpiTimeT *time)
{
  return (((long)time->high << 32) | (long)time->low) / 1000000;
}

extern "C" {

// ----------------------------------------------------------------------------
// VHPI callbacks
// ----------------------------------------------------------------------------

static void start_of_sim_cb(const vhpiCbDataT * cb_data)
{
  uvvm_cosim::start_of_sim();
}

static void end_of_sim_cb(const vhpiCbDataT * cb_data) {
  vhpiTimeT t;
  long cycles;

  vhpi_get_time(&t, &cycles);

  uvvm_cosim::end_of_sim(cycles, convert_time_to_ns(&t));
}


// ----------------------------------------------------------------------------
// VHPI foreign functions and procedures
// ----------------------------------------------------------------------------

static void uvvm_cosim_foreign_start_sim(const vhpiCbDataT* p_cb_data)
{
  uvvm_cosim::start_sim();
}

static void uvvm_cosim_foreign_terminate_sim(const vhpiCbDataT* p_cb_data)
{
  bool terminate = uvvm_cosim::terminate_sim();

  return_vhpi_int(p_cb_data, terminate ? 1 : 0);
}

static void uvvm_cosim_foreign_transmit_byte_queue_empty(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  bool empty = uvvm_cosim::transmit_byte_queue_empty(vvc_type, vvc_instance_id);

  return_vhpi_int(p_cb_data, empty ? 1 : 0);
}

static void uvvm_cosim_foreign_transmit_byte_queue_get(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  int data = uvvm_cosim::transmit_byte_queue_get(vvc_type, vvc_instance_id);

  return_vhpi_int(p_cb_data, data);
}

static void uvvm_cosim_foreign_receive_byte_queue_put(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);
  uint8_t byte         = get_vhpi_int_param_by_index(p_cb_data, 2);

  uvvm_cosim::receive_byte_queue_put(vvc_type, vvc_instance_id, byte);
}

static void uvvm_cosim_foreign_transmit_packet_queue_empty(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  bool empty = uvvm_cosim::transmit_packet_queue_empty(vvc_type, vvc_instance_id);

  return_vhpi_int(p_cb_data, empty ? 1 : 0);
}

static void uvvm_cosim_foreign_transmit_packet_queue_get(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  int byte_and_eop = uvvm_cosim::transmit_packet_queue_get(vvc_type, vvc_instance_id);

  return_vhpi_int(p_cb_data, byte_and_eop);
}

static void uvvm_cosim_foreign_receive_packet_queue_put(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);
  uint8_t byte         = get_vhpi_int_param_by_index(p_cb_data, 2);
  int end_of_packet    = get_vhpi_int_param_by_index(p_cb_data, 3);
  bool eop             = end_of_packet == 1 ? true : false;

  uvvm_cosim::receive_packet_queue_put(vvc_type, vvc_instance_id, byte, eop);
}

static void uvvm_cosim_foreign_mm_request_empty(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  bool empty = uvvm_cosim::mm_request_empty(vvc_type, vvc_instance_id);

  return_vhpi_int(p_cb_data, empty ? 1 : 0);
}

static void uvvm_cosim_foreign_mm_request_fetch(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  uvvm_cosim::mm_request_fetch(vvc_type, vvc_instance_id);
}

static void uvvm_cosim_foreign_mm_request_is_write(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  bool is_write = uvvm_cosim::mm_request_is_write(vvc_type, vvc_instance_id);

  return_vhpi_int(p_cb_data, is_write ? 1 : 0);
}

static void uvvm_cosim_foreign_mm_request_get_addr(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  uint32_t addr = uvvm_cosim::mm_request_get_addr(vvc_type, vvc_instance_id);

  // VHDL receives this as a signed integer; 2's-complement gives the right
  // unsigned value back via to_unsigned() on the VHDL side.
  return_vhpi_int(p_cb_data, static_cast<int>(addr));
}

static void uvvm_cosim_foreign_mm_request_get_num_bytes(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  int n = uvvm_cosim::mm_request_get_num_bytes(vvc_type, vvc_instance_id);

  return_vhpi_int(p_cb_data, n);
}

static void uvvm_cosim_foreign_mm_request_get_byte(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);
  int byte_idx         = get_vhpi_int_param_by_index(p_cb_data, 2);

  int byte = uvvm_cosim::mm_request_get_byte(vvc_type, vvc_instance_id, byte_idx);

  return_vhpi_int(p_cb_data, byte);
}

static void uvvm_cosim_foreign_mm_response_write_done(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  uvvm_cosim::mm_response_write_done(vvc_type, vvc_instance_id);
}

static void uvvm_cosim_foreign_mm_response_put_byte(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);
  uint8_t byte         = get_vhpi_int_param_by_index(p_cb_data, 2);

  uvvm_cosim::mm_response_put_byte(vvc_type, vvc_instance_id, byte);
}

static void uvvm_cosim_foreign_mm_response_read_done(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  uvvm_cosim::mm_response_read_done(vvc_type, vvc_instance_id);
}

static void uvvm_cosim_foreign_vvc_listen_enable(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  bool listen          = uvvm_cosim::vvc_listen_enable(vvc_type, vvc_instance_id);

  return_vhpi_int(p_cb_data, listen ? 1 : 0);
}

static void uvvm_cosim_foreign_report_vvc_info(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type    = get_vhpi_str_param_by_index(p_cb_data, 0);
  std::string vvc_channel = get_vhpi_str_param_by_index(p_cb_data, 1);
  int vvc_instance_id     = get_vhpi_int_param_by_index(p_cb_data, 2);
  std::string bfm_cfg_str = get_vhpi_str_param_by_index(p_cb_data, 3);

  uvvm_cosim::report_vvc_info(vvc_type, vvc_channel, vvc_instance_id, bfm_cfg_str);
}

static void register_foreign_methods(void)
{
  vhpi_printf("register_foreign_methods() called");

  const char* c_lib_name = "libuvvm_cosim_vhpi.so";

  register_vhpi_foreign_method(uvvm_cosim_foreign_report_vvc_info,
			       "uvvm_cosim_foreign_report_vvc_info",
			       c_lib_name,
			       vhpiProcF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_start_sim,
			       "uvvm_cosim_foreign_start_sim",
			       c_lib_name,
			       vhpiProcF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_terminate_sim,
			       "uvvm_cosim_foreign_terminate_sim",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_vvc_listen_enable,
			       "uvvm_cosim_foreign_vvc_listen_enable",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_transmit_byte_queue_empty,
			       "uvvm_cosim_foreign_transmit_byte_queue_empty",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_transmit_byte_queue_get,
			       "uvvm_cosim_foreign_transmit_byte_queue_get",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_receive_byte_queue_put,
			       "uvvm_cosim_foreign_receive_byte_queue_put",
			       c_lib_name,
			       vhpiProcF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_transmit_packet_queue_empty,
			       "uvvm_cosim_foreign_transmit_packet_queue_empty",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_transmit_packet_queue_get,
			       "uvvm_cosim_foreign_transmit_packet_queue_get",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_receive_packet_queue_put,
			       "uvvm_cosim_foreign_receive_packet_queue_put",
			       c_lib_name,
			       vhpiProcF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_mm_request_empty,
			       "uvvm_cosim_foreign_mm_request_empty",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_mm_request_fetch,
			       "uvvm_cosim_foreign_mm_request_fetch",
			       c_lib_name,
			       vhpiProcF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_mm_request_is_write,
			       "uvvm_cosim_foreign_mm_request_is_write",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_mm_request_get_addr,
			       "uvvm_cosim_foreign_mm_request_get_addr",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_mm_request_get_num_bytes,
			       "uvvm_cosim_foreign_mm_request_get_num_bytes",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_mm_request_get_byte,
			       "uvvm_cosim_foreign_mm_request_get_byte",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_mm_response_write_done,
			       "uvvm_cosim_foreign_mm_response_write_done",
			       c_lib_name,
			       vhpiProcF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_mm_response_put_byte,
			       "uvvm_cosim_foreign_mm_response_put_byte",
			       c_lib_name,
			       vhpiProcF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_mm_response_read_done,
			       "uvvm_cosim_foreign_mm_response_read_done",
			       c_lib_name,
			       vhpiProcF);

  vhpi_printf("Registered all foreign functions/procedures");
}

static void register_callbacks()
{
  vhpi_printf("register_callbacks() called");

  vhpiCbDataT cb_data;

  cb_data.reason    = vhpiCbStartOfSimulation;
  cb_data.cb_rtn    = start_of_sim_cb;
  cb_data.obj       = NULL;
  cb_data.time      = NULL;
  cb_data.value     = NULL;
  cb_data.user_data = NULL;

  vhpi_register_cb(&cb_data, 0);

  cb_data.reason = vhpiCbEndOfSimulation;
  cb_data.cb_rtn = end_of_sim_cb;

  static vhpiTimeT t = {.high = 0, .low = 250};
  cb_data.time = &t;

  vhpi_register_cb(&cb_data, 0);
}


void (*vhpi_startup_routines[])() = {
  register_callbacks,
  register_foreign_methods,
  NULL
};

}
