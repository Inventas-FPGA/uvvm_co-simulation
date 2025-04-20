#pragma once
#include <deque>
#include <map>
#include <string>
#include "nlohmann/json.hpp"
#include "byte_queue.hpp"
#include "packet_queue.hpp"

// Todo: Use namespace
namespace uvvm_cosim {

using json = nlohmann::json;

enum QueueId { QID_TRANSMIT, QID_RECEIVE, QID_MAX};

// Used as key in std::map of all VVCs in server
struct VvcInstanceKey {
  std::string vvc_type;
  std::string vvc_channel;
  int vvc_instance_id;
};

inline std::string to_string(const VvcInstanceKey& vvc)
{
  return "type=" + vvc.vvc_type +
         ", channel=" + vvc.vvc_channel +
         ", instance_id=" + std::to_string(vvc.vvc_instance_id);
}

struct VvcConfig {
  // Common config options used with most/all VVC types
  bool listen_enable = false;
  bool cosim_support = false;
  bool packet_based = false;

  // Map for VVC/BFM-specific config options
  std::map<std::string, int> bfm_cfg;
};

// Used as value in std::map of all VVCs in server
struct VvcInstanceData {
  VvcConfig cfg;

  // Used only for VVCs that are not packet-based
  std::array<ByteQueue, QID_MAX> byte_queues;

  // Used only for VVCs that are packet-based
  std::array<PacketQueue, QID_MAX> packet_queues;
};

// This struct contains all fields that identify a VVC as well as
// configuration values, but not the transmit/receive queues.
// It's used to send VVC info over JSON-RPC
struct VvcInstance : public VvcInstanceKey, VvcConfig {
  VvcInstance() {}
  VvcInstance(VvcInstanceKey k, VvcConfig c)
      : VvcInstanceKey(k), VvcConfig(c) {}
};

// This class should implement the necessary comparator function (with
// strict ordering) so we can use VvcInstanceKey with std::map.
// https://stackoverflow.com/questions/6573225/what-requirements-must-stdmap-key-classes-meet-to-be-valid-keys
class VvcCompare {
public:
  bool operator() (const VvcInstanceKey &lhs, const VvcInstanceKey &rhs) const {
    if (lhs.vvc_type < rhs.vvc_type) return true;
    if (lhs.vvc_type > rhs.vvc_type) return false;
    if (lhs.vvc_channel < rhs.vvc_channel) return true;
    if (lhs.vvc_channel > rhs.vvc_channel) return false;

    return lhs.vvc_instance_id < rhs.vvc_instance_id;
  }
};

inline void to_json(json &j, const VvcInstance &v) {
  j = json{{"vvc_type", v.vvc_type},
           {"vvc_channel", v.vvc_channel},
           {"vvc_instance_id", v.vvc_instance_id},
           {"bfm_cfg", v.bfm_cfg},
           {"listen_enable", v.listen_enable}};
}

inline void from_json(const json &j, VvcInstance &v) {
  j.at("vvc_type").get_to(v.vvc_type);
  j.at("vvc_channel").get_to(v.vvc_channel);
  j.at("vvc_instance_id").get_to(v.vvc_instance_id);
  j.at("bfm_cfg").get_to(v.bfm_cfg);
  j.at("listen_enable").get_to(v.listen_enable);
}

struct JsonResponse {
  bool success;
  json result;
};

inline void to_json(json &j, const JsonResponse &response) {
  j = json{{"success", response.success},
           {"result", response.result}};
}

inline void from_json(const json &j, JsonResponse &response) {
  j.at("success").get_to(response.success);
  j.at("result").get_to(response.result);
}

}; // namespace uvvm_cosim
