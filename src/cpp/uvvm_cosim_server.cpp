#include <map>
#include <string>
#include <utility>
#include <vector>
#include "nlohmann/json.hpp"
#include "uvvm_cosim_server.hpp"

// Split a string by delimiter into substrings.
// Unnecessary leading/trailing and extra delimiters are removed
static std::vector<std::string> split_str(std::string str, std::string delim)
{
  std::vector<std::string> str_list;

  size_t next_pos;

  while ((next_pos = str.find_first_not_of(delim)) != std::string::npos) {
    str.erase(0, next_pos);

    size_t delim_pos = str.find_first_of(delim);
    str_list.push_back(str.substr(0, delim_pos));
    str.erase(0, delim_pos);
  }

  return str_list;
}

// Search a comma-separated string for config values
// Each config value must be a key/value pair formatted as "key=value"
// Only integers are supported for value (use 0/1 for bool).
// Example string:
// "packet_based=1,enabled=0,timeout=1000"
static std::map<std::string, int> parse_bfm_cfg_str(const std::string& cfg_str)
{
  std::map<std::string, int> bfm_cfg;

  try {
    auto cfg_items = split_str(cfg_str, ",");

    for (auto &cfg_item : cfg_items) {

      std::cout << "cfg_item=\"" << cfg_item << "\"" << std::endl;

      auto cfg_key_val = split_str(cfg_item, "=");

      if (cfg_key_val.size() != 2) {
        std::cerr << "Error parsing config item" << std::endl;
        continue;
      }

      std::string cfg_key = cfg_key_val[0];
      int cfg_val = std::stoi(cfg_key_val[1]);

      bfm_cfg.emplace(cfg_key, cfg_val);
    }

  } catch (std::exception &e) {
    std::cerr << "Exception processing config string \"" << cfg_str << "\".";
    std::cerr << "Reason=" << e.what() << std::endl;
  }

  return bfm_cfg;
}

void
UvvmCosimServer::WaitForStartSim()
{
  using namespace std::chrono_literals;

  while (!startSim) {
    std::this_thread::sleep_for(10ms);
  }
}

bool
UvvmCosimServer::VvcCosimRecvEnabled(std::string vvc_type,
				     int vvc_instance_id)
{
  VvcInstanceKey vvc = {
    .vvc_type = vvc_type,
    .vvc_channel = (vvc_type == "UART_VVC" ? "RX" : "NA"),
    .vvc_instance_id = vvc_instance_id
  };

  bool listen = vvcInstanceMap([&](auto &vvc_map) {
    if (auto it = vvc_map.find(vvc); it != vvc_map.end()) {
      return it->second.cfg.cosim_recv_enable;
    } else {
      std::cerr << "VVC with";
      std::cerr << " type=" << vvc.vvc_type;
      std::cerr << " channel=" << vvc.vvc_channel;
      std::cerr << " instance_id=" << vvc.vvc_instance_id;
      std::cerr << " does not exist." << std::endl;

      return false;
    }
  });

  return listen;
}

void
UvvmCosimServer::AddVvc(std::string vvc_type, std::string vvc_channel,
			int vvc_instance_id, std::string bfm_cfg_str)
{
  auto bfm_cfg = parse_bfm_cfg_str(bfm_cfg_str);

  VvcInstanceKey vvc = {
    .vvc_type = vvc_type,
    .vvc_channel = vvc_channel,
    .vvc_instance_id = vvc_instance_id
  };

  vvcInstanceMap([&](auto &vvc_map) {
    if (vvc_map.find(vvc) == vvc_map.end()) {
      vvc_map.emplace(vvc, VvcInstanceData{.cfg{.bfm_cfg = bfm_cfg}});
    } else {
      std::cerr << "VVC with type=" << vvc.vvc_type;
      std::cerr << " channel=" << vvc.vvc_channel;
      std::cerr << " instance_id=" << vvc.vvc_instance_id;
      std::cerr << " exist already." << std::endl;
    }
  });
}

bool
UvvmCosimServer::TransmitQueueEmpty(std::string vvc_type,
				    int vvc_instance_id)
{
  VvcInstanceKey vvc = {
    .vvc_type = vvc_type,
    .vvc_channel = (vvc_type == "UART_VVC" ? "TX" : "NA"),
    .vvc_instance_id = vvc_instance_id
  };

  bool empty = vvcInstanceMap([&](auto &vvc_map) {
    if (auto it = vvc_map.find(vvc); it != vvc_map.end()) {
      return it->second.transmit_queue.empty();
    } else {
      std::cerr << "VVC with";
      std::cerr << " type=" << vvc.vvc_type;
      std::cerr << " channel=" << vvc.vvc_channel;
      std::cerr << " instance_id=" << vvc.vvc_instance_id;
      std::cerr << " does not exist." << std::endl;

      return true; // empty
    }
  });

  return empty;
}

std::optional<std::pair<uint8_t, bool>>
UvvmCosimServer::TransmitQueueGet(std::string vvc_type,
				  int vvc_instance_id)
{
  VvcInstanceKey vvc = {
    .vvc_type = vvc_type,
    .vvc_channel = (vvc_type == "UART_VVC" ? "TX" : "NA"),
    .vvc_instance_id = vvc_instance_id
  };

  std::pair<uint8_t, bool> byte = {};

  vvcInstanceMap([&](auto &vvc_map) {
    if (auto it = vvc_map.find(vvc); it != vvc_map.end()) {

      if (!it->second.transmit_queue.empty()) {
	byte = it->second.transmit_queue.front();
	it->second.transmit_queue.pop_front();
      } else {
        std::cerr << "TransmitBytesQueueGet called on empty queue for VVC with";
        std::cerr << " type=" << vvc.vvc_type;
        std::cerr << " channel=" << vvc.vvc_channel;
        std::cerr << " instance_id=" << vvc.vvc_instance_id;
        std::cerr << std::endl;
      }

    } else {
      std::cerr << "VVC with";
      std::cerr << " type=" << vvc.vvc_type;
      std::cerr << " channel=" << vvc.vvc_channel;
      std::cerr << " instance_id=" << vvc.vvc_instance_id;
      std::cerr << " does not exist." << std::endl;

      // TODO:
      // Throw exception?
    }
  });

  return byte;
}

void UvvmCosimServer::ReceiveQueuePut(std::string vvc_type,
				      int vvc_instance_id,
				      uint8_t byte, bool end_of_packet)
{
  VvcInstanceKey vvc = {
    .vvc_type = vvc_type,
    .vvc_channel = (vvc_type == "UART_VVC" ? "RX" : "NA"),
    .vvc_instance_id = vvc_instance_id
  };

  vvcInstanceMap([&](auto &vvc_map) {
    if (auto it = vvc_map.find(vvc); it != vvc_map.end()) {
      it->second.receive_queue.push_back(std::make_pair(byte, end_of_packet));
    } else {
      std::cerr << "VVC with";
      std::cerr << " type=" << vvc.vvc_type;
      std::cerr << " channel=" << vvc.vvc_channel;
      std::cerr << " instance_id=" << vvc.vvc_instance_id;
      std::cerr << " does not exist." << std::endl;
    }
  });
}

JsonResponse
UvvmCosimServer::StartSim()
{
  startSim=true;

  JsonResponse response = {
    .success = false
  };

  return response;
}

JsonResponse
UvvmCosimServer::GetVvcList()
{
  std::vector<VvcInstance> vec;

  vvcInstanceMap([&](auto &vvc_map) {
    for (auto vvc : vvc_map) {
      vec.push_back(VvcInstance(vvc.first, vvc.second.cfg));
    }
  });

  JsonResponse response;
  
  response.success = true;
  response.result = json(vec);

  return response;
}

JsonResponse
UvvmCosimServer::SetVvcCosimRecvState(std::string vvc_type, int vvc_id, bool enable)
{
  JsonResponse response;

  VvcInstanceKey vvc = {
    .vvc_type = vvc_type,
    .vvc_channel = (vvc_type == "UART_VVC" ? "RX" : "NA"),
    .vvc_instance_id = vvc_id
  };

  vvcInstanceMap([&](auto &vvc_map) {
    if (decltype(vvc_map.begin()) it = vvc_map.find(vvc); it != vvc_map.end()) {
      it->second.cfg.cosim_recv_enable = enable;
      response.success = true;
      response.result = json{};

    } else {
      std::string error_str = "VVC with";
      error_str += " type=" + vvc.vvc_type;
      error_str += " channel=" + vvc.vvc_channel;
      error_str += " instance_id=" + std::to_string(vvc.vvc_instance_id);
      error_str += " does not exist.";

      response.success = false;
      response.result = json{{"error", error_str}};
    }
  });

  return response;
}

JsonResponse
UvvmCosimServer::TransmitBytes(std::string vvc_type, int vvc_id, std::vector<uint8_t> data)
{
  JsonResponse response;

  VvcInstanceKey vvc = {
    .vvc_type = vvc_type,
    .vvc_channel = (vvc_type == "UART_VVC" ? "TX" : "NA"),
    .vvc_instance_id = vvc_id
  };

  vvcInstanceMap([&](auto &vvc_map) {
    if (auto it = vvc_map.find(vvc); it != vvc_map.end()) {
      auto& q = it->second.transmit_queue;
      
      // Transform uint8_t elements from data to the
      // std::pair<uint8_t,bool> elements that go in transmit_queue
      // and insert to end of that queue.  The bool part (end of
      // packet flag) is set to false since it's only used for
      // TransmitPacket.
      std::transform(data.begin(), data.end(), std::back_inserter(q),
                     [](const uint8_t& byte) {
		       return std::make_pair(byte, false);
		     });

      response.success = true;
      response.result = json{};

    } else {
      std::string error_str = "VVC with";
      error_str += " type=" + vvc.vvc_type;
      error_str += " channel=" + vvc.vvc_channel;
      error_str += " instance_id=" + std::to_string(vvc.vvc_instance_id);
      error_str += " does not exist.";
      
      response.success = false;
      response.result = json{{"error", error_str}};
    }
  });

  return response;
}

JsonResponse
UvvmCosimServer::TransmitPacket(std::string vvc_type, int vvc_id, std::vector<uint8_t> data)
{
  JsonResponse response = {
    .success = false,
    .result = json{{"error", "Not implemented"}}
  };

  return response;
}

JsonResponse
UvvmCosimServer::ReceiveBytes(std::string vvc_type, int vvc_id, int length, bool all_or_nothing)
{
  JsonResponse response;

  VvcInstanceKey vvc = {
    .vvc_type = vvc_type,
    .vvc_channel = (vvc_type == "UART_VVC" ? "RX" : "NA"),
    .vvc_instance_id = vvc_id
  };

  vvcInstanceMap([&](auto &vvc_map) {
    if (auto it = vvc_map.find(vvc); it != vvc_map.end()) {
      std::vector<uint8_t> data;
      auto& q = it->second.receive_queue;

      if (q.empty() || (all_or_nothing && q.size() < length)) {
        std::cout << "Server: " << "ReceiveBytes called with length=" << length;
	std::cout << " and all_or_nothing=" << (all_or_nothing ? "true" : "false");
	std::cout << ". Returning none, queue size = " << q.size() << std::endl;
      } else {
	auto q_end = q.size() > length ? q.begin()+length : q.end();

	// Transform and copy std::pair<uint8_t,bool> elements from
	// receive_queue to data vector, stripping away the bool in
	// the pair (which indicates end of packet, not used for
	// ReceiveBytes).
	std::transform(q.begin(), q_end, std::back_inserter(data),
		       [](auto q_elem){
			 return q_elem.first;
		       });

	std::cout << "Server: " << "ReceiveBytes called with length=" << length;
	std::cout << " and all_or_nothing=" << (all_or_nothing ? "true" : "false");
	std::cout << ". Returning " << data.size() << " of " << q.size();
	std::cout << " available bytes in queue" << std::endl;

        q.erase(q.begin(), q_end);
      }

      response.success = true;
      response.result = json{{"data", data}};

    } else {
      std::string error_str = "VVC with";
      error_str += " type=" + vvc.vvc_type;
      error_str += " channel=" + vvc.vvc_channel;
      error_str += " instance_id=" + std::to_string(vvc.vvc_instance_id);
      error_str += " does not exist.";
      
      response.success = false;
      response.result = json{{"error", error_str}};
    }
  });

  return response;
}

JsonResponse
UvvmCosimServer::ReceivePacket(std::string vvc_type, int vvc_id)
{
  JsonResponse response = {
    .success = false,
    .result = json{{"error", "Not implemented"}}
  };

  return response;
}
