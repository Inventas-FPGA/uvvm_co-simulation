#pragma once
#include <string>
#include <cstdint>

namespace uvvm_cosim {

#ifdef VHPI
#include <vhpi_user.h>
constexpr auto sim_printf = vhpi_printf;

#elif defined(FLI)
#include <mti.h>
constexpr auto sim_printf = mti_PrintFormatted;
#endif

bool transmit_byte_queue_empty(std::string vvc_type, int vvc_instance_id);

int transmit_byte_queue_get(std::string vvc_type, int vvc_instance_id);

void receive_byte_queue_put(std::string vvc_type, int vvc_instance_id,
                            uint8_t byte);

bool transmit_packet_queue_empty(std::string vvc_type, int vvc_instance_id);

int transmit_packet_queue_get(std::string vvc_type, int vvc_instance_id);

void receive_packet_queue_put(std::string vvc_type, int vvc_instance_id,
			      uint8_t byte, bool eop);

void start_sim(void);

bool terminate_sim(void);

bool vvc_listen_enable(std::string vvc_type, int vvc_instance_id);

void report_vvc_info(std::string vvc_type,
				std::string vvc_channel,
				int vvc_instance_id,
				std::string bfm_cfg_str);

void start_of_sim(void);

void end_of_sim(long cycles, long time_ns);

} // namespace uvvm_cosim
