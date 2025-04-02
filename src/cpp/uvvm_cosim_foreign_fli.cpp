#include <string>
#include <cstdint>
#include <mti.h>
#include "uvvm_cosim_common.hpp"

// Convert VHDL string array to std::string
// Based on function from Modelsim/Questasim FLI example four
static std::string get_string(mtiVariableIdT id)
{
  char*       buf;
  int         len;
  mtiTypeIdT  type;

  type = mti_GetVarType(id);
  len  = mti_TickLength(type);
  buf  = new char[len+1];
  mti_GetArrayVarValue(id, buf);
  buf[len] = 0;

  std::string s(buf);
  delete buf;
  return s;
}


extern "C" {

void uvvm_cosim_foreign_start_sim(void)
{
  uvvm_cosim_start_sim();
}

int uvvm_cosim_foreign_terminate_sim(void)
{
  return uvvm_cosim_terminate_sim() ? 1 : 0;
}

void uvvm_cosim_foreign_report_vvc_info(mtiVariableIdT vvc_type,
					mtiVariableIdT vvc_channel,
					int            vvc_instance_id,
					mtiVariableIdT bfm_cfg)
{
  std::string vvc_type_str    = get_string(vvc_type);
  std::string vvc_channel_str = get_string(vvc_channel);
  std::string bfm_cfg_str     = get_string(bfm_cfg);

  uvvm_cosim_report_vvc_info(vvc_type_str, vvc_channel_str, vvc_instance_id, bfm_cfg_str);
}

int uvvm_cosim_foreign_vvc_listen_enable(mtiVariableIdT vvc_type,
					 int            vvc_instance_id)
{
  std::string vvc_type_str = get_string(vvc_type);

  return uvvm_cosim_vvc_listen_enable(vvc_type_str, vvc_instance_id) ? 1 : 0;
}

int uvvm_cosim_foreign_transmit_queue_empty(mtiVariableIdT vvc_type,
					    int            vvc_instance_id)
{
  std::string vvc_type_str = get_string(vvc_type);

  return uvvm_cosim_transmit_queue_empty(vvc_type_str, vvc_instance_id) ? 1 : 0;
}

int uvvm_cosim_foreign_transmit_queue_get(mtiVariableIdT vvc_type,
					  int            vvc_instance_id)
{
  std::string vvc_type_str = get_string(vvc_type);

  return uvvm_cosim_transmit_queue_get(vvc_type_str, vvc_instance_id);
}

void uvvm_cosim_foreign_receive_queue_put(mtiVariableIdT vvc_type,
					  int            vvc_instance_id,
					  int            byte,
					  int            end_of_packet)
{
  std::string vvc_type_str = get_string(vvc_type);
  bool        eop          = end_of_packet == 1;

  uvvm_cosim_receive_queue_put(vvc_type_str, vvc_instance_id, byte, eop);
}

static void start_of_sim_cb(void* p)
{
  uvvm_cosim_start_of_sim();
}

static void end_of_sim_cb(void* p)
{
  // TODO: Retrieve and add cycles and time!
  uvvm_cosim_end_of_sim(0, 0);
}

void uvvm_cosim_fli_init(void)
{
  mti_PrintFormatted("uvvm_cosim_fli_init\n");
  mti_AddLoadDoneCB(start_of_sim_cb, NULL);
  mti_AddQuitCB(end_of_sim_cb, NULL);

  // TODO: Simulator restart/reload
  // mti_AddRestartCB()
}

} // extern "C"

