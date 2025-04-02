#pragma once
#include <string>
#include <cstdint>

#ifdef VHPI
#include <vhpi_user.h>
constexpr auto sim_printf = vhpi_printf;

#elif defined(FLI)
#include <mti.h>
constexpr auto sim_printf = mti_PrintFormatted;
#endif

bool uvvm_cosim_transmit_queue_empty(std::string vvc_type, int vvc_instance_id);

int uvvm_cosim_transmit_queue_get(std::string vvc_type, int vvc_instance_id);

void uvvm_cosim_receive_queue_put(std::string vvc_type,
				  int vvc_instance_id,
				  uint8_t byte,
				  bool end_of_packet);

void uvvm_cosim_start_sim(void);

bool uvvm_cosim_terminate_sim(void);

bool uvvm_cosim_vvc_listen_enable(std::string vvc_type, int vvc_instance_id);

void uvvm_cosim_report_vvc_info(std::string vvc_type,
				std::string vvc_channel,
				int vvc_instance_id,
				std::string bfm_cfg_str);

void uvvm_cosim_start_of_sim(void);

void uvvm_cosim_end_of_sim(long cycles, long time_ns);
