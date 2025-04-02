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
  uvvm_cosim_start_of_sim();
}

static void end_of_sim_cb(const vhpiCbDataT * cb_data) {
  vhpiTimeT t;
  long cycles;

  vhpi_get_time(&t, &cycles);

  uvvm_cosim_end_of_sim(cycles, convert_time_to_ns(&t));
}


// ----------------------------------------------------------------------------
// VHPI foreign functions and procedures
// ----------------------------------------------------------------------------

static void uvvm_cosim_foreign_start_sim(const vhpiCbDataT* p_cb_data)
{
  uvvm_cosim_start_sim();
}

static void uvvm_cosim_foreign_terminate_sim(const vhpiCbDataT* p_cb_data)
{
  bool terminate = uvvm_cosim_terminate_sim();

  return_vhpi_int(p_cb_data, terminate ? 1 : 0);
}

static void uvvm_cosim_foreign_transmit_queue_empty(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  bool empty = uvvm_cosim_transmit_queue_empty(vvc_type, vvc_instance_id);

  return_vhpi_int(p_cb_data, empty ? 1 : 0);
}

void uvvm_cosim_foreign_transmit_queue_get(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  int data = uvvm_cosim_transmit_queue_get(vvc_type, vvc_instance_id);

  return_vhpi_int(p_cb_data, data);
}

void uvvm_cosim_foreign_receive_queue_put(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);
  uint8_t byte         = get_vhpi_int_param_by_index(p_cb_data, 2);
  bool end_of_packet   = get_vhpi_int_param_by_index(p_cb_data, 3) == 1;

  uvvm_cosim_receive_queue_put(vvc_type, vvc_instance_id, byte, end_of_packet);
}

void uvvm_cosim_foreign_vvc_listen_enable(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type = get_vhpi_str_param_by_index(p_cb_data, 0);
  int vvc_instance_id  = get_vhpi_int_param_by_index(p_cb_data, 1);

  bool listen          = uvvm_cosim_vvc_listen_enable(vvc_type, vvc_instance_id);

  return_vhpi_int(p_cb_data, listen ? 1 : 0);
}

void uvvm_cosim_foreign_report_vvc_info(const vhpiCbDataT* p_cb_data)
{
  std::string vvc_type    = get_vhpi_str_param_by_index(p_cb_data, 0);
  std::string vvc_channel = get_vhpi_str_param_by_index(p_cb_data, 1);
  int vvc_instance_id     = get_vhpi_int_param_by_index(p_cb_data, 2);
  std::string bfm_cfg_str = get_vhpi_str_param_by_index(p_cb_data, 3);

  uvvm_cosim_report_vvc_info(vvc_type, vvc_channel, vvc_instance_id, bfm_cfg_str);
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

  register_vhpi_foreign_method(uvvm_cosim_foreign_transmit_queue_empty,
			       "uvvm_cosim_foreign_transmit_queue_empty",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_transmit_queue_get,
			       "uvvm_cosim_foreign_transmit_queue_get",
			       c_lib_name,
			       vhpiFuncF);

  register_vhpi_foreign_method(uvvm_cosim_foreign_receive_queue_put,
			       "uvvm_cosim_foreign_receive_queue_put",
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
