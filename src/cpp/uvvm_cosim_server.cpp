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

namespace uvvm_cosim {

void
UvvmCosimServer::WaitForStartSim()
{
  using namespace std::chrono_literals;

  while (!cosimData.getStartSim() && !cosimData.getTerminateSim()) {
    std::this_thread::sleep_for(10ms);
  }
}

bool
UvvmCosimServer::ShouldTerminateSim()
{
  return cosimData.getTerminateSim();
}

bool
UvvmCosimServer::VvcListenEnabled(std::string vvc_type,
				  int vvc_instance_id)
{
  VvcInstanceKey vvc = {
    .vvc_type = vvc_type,
    .vvc_channel = (vvc_type == "UART_VVC" ? "RX" : "NA"),
    .vvc_instance_id = vvc_instance_id
  };

  // Note: GetVvcListenEnable will throw if vvc does not exist
  return cosimData.GetVvcListenEnable(vvc);
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

  // Note: Throws if vvc exists already
  cosimData.AddVvc(vvc, bfm_cfg);
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

  return cosimData.byte_queue_empty(QID_TRANSMIT, vvc);
}

std::optional<uint8_t>
UvvmCosimServer::TransmitQueueGet(std::string vvc_type,
				  int vvc_instance_id)
{
  VvcInstanceKey vvc = {
    .vvc_type = vvc_type,
    .vvc_channel = (vvc_type == "UART_VVC" ? "TX" : "NA"),
    .vvc_instance_id = vvc_instance_id
  };

  return cosimData.byte_queue_get(QID_TRANSMIT, vvc);
}

void UvvmCosimServer::ReceiveQueuePut(std::string vvc_type,
				      int vvc_instance_id,
				      uint8_t byte)
{
  VvcInstanceKey vvc = {
    .vvc_type = vvc_type,
    .vvc_channel = (vvc_type == "UART_VVC" ? "RX" : "NA"),
    .vvc_instance_id = vvc_instance_id
  };

  cosimData.byte_queue_put(QID_RECEIVE, vvc, byte);
}

JsonResponse
UvvmCosimServer::StartSim()
{
  cosimData.setStartSim(true);

  JsonResponse response = {
    .success = true
  };

  return response;
}

JsonResponse
UvvmCosimServer::PauseSim()
{
  cosimData.setStartSim(false);

  JsonResponse response = {
    .success = true
  };

  return response;
}

JsonResponse
UvvmCosimServer::TerminateSim()
{
  cosimData.setTerminateSim(true);

  std::cout << std::endl << std::endl << std::endl;
  std::cout << "TERMINATING SIM!!!" << std::endl << std::endl << std::endl;

  JsonResponse response = {
    .success = true
  };

  return response;
}

JsonResponse
UvvmCosimServer::GetVvcList()
{
  std::vector<VvcInstance> vec = cosimData.GetVvcList();

  JsonResponse response;
  
  response.success = true;
  response.result = json(vec);

  return response;
}

JsonResponse
UvvmCosimServer::SetVvcListenEnable(std::string vvc_type, int vvc_id, bool enable)
{
  JsonResponse response;

  VvcInstanceKey vvc = {
    .vvc_type = vvc_type,
    .vvc_channel = (vvc_type == "UART_VVC" ? "RX" : "NA"),
    .vvc_instance_id = vvc_id
  };

  try {
    cosimData.SetVvcListenEnable(vvc, enable);
    response.success = true;
  }
  catch (const std::runtime_error& e) {
    response.success = false;
    response.result = json{{"error", e.what()}};
  }

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

  try {
    cosimData.byte_queue_put(QID_TRANSMIT, vvc, data);
    response.success = true;
  }
  catch (const std::runtime_error& e) {
    response.success = false;
    response.result = json{{"error", e.what()}};
  }

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
UvvmCosimServer::ReceiveBytes(std::string vvc_type, int vvc_id, int num_bytes, bool exact_length)
{
  JsonResponse response;

  VvcInstanceKey vvc = {
    .vvc_type = vvc_type,
    .vvc_channel = (vvc_type == "UART_VVC" ? "RX" : "NA"),
    .vvc_instance_id = vvc_id
  };

  try {
    std::vector<uint8_t> data;
    bool empty = cosimData.byte_queue_empty(QID_RECEIVE, vvc);

    if (!empty && exact_length) {
      size_t size = cosimData.byte_queue_size(QID_RECEIVE, vvc);

      if (size >= num_bytes) {
        data = cosimData.byte_queue_get(QID_RECEIVE, vvc, num_bytes);
      }
    } else if (!empty && !exact_length) {
      data = cosimData.byte_queue_get(QID_RECEIVE, vvc, num_bytes);
    }

    response.success = true;
    response.result = json{{"data", data}};
  }
  catch (const std::runtime_error& e) {
    response.success = false;
    response.result = json{{"error", e.what()}};
  }

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

} // namespace uvvm_cosim

