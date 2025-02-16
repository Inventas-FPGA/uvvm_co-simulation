#pragma once
#include <deque>
#include <map>
#include <string>
#include "nlohmann/json.hpp"

// Todo: Use namespace
//namespace uvvm_cosim {

using json = nlohmann::json;

// Note: Many VVCs will only use one of the queues
struct VvcQueues {
  std::deque<std::pair<uint8_t, bool>> transmit_queue;
  std::deque<std::pair<uint8_t, bool>> receive_queue;
};

// Todo:
// It's pretty dumb to store data in the key
// (VvcInstance is used as key in std::map of VVCs).
// Should replace with a VvcInstanceKey class that has only
// vvc_type, vvc_channel, and vvc_id.
// And then put the queues from VvcQueues, vvc_cfg, listen_enable
// (and any future fields) in VvcInstance class.
struct VvcInstance {
  std::string vvc_type;
  std::string vvc_channel;
  int vvc_instance_id;
  std::map<std::string, int> vvc_cfg;
  bool listen_enable = false;
};

// This class should implement the necessary comparator function (with
// strict ordering) so we can use VvcInstance with std::map.
// https://stackoverflow.com/questions/6573225/what-requirements-must-stdmap-key-classes-meet-to-be-valid-keys
// Note that we don't care about vvc_cfg, just want to be able to
// distinguish between VVCs based on type, channel, and ID.
class VvcCompare {
public:
  bool operator() (const VvcInstance &lhs, const VvcInstance &rhs) const {
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
           {"vvc_cfg", v.vvc_cfg}};
}

inline void from_json(const json &j, VvcInstance &v) {
  j.at("vvc_type").get_to(v.vvc_type);
  j.at("vvc_channel").get_to(v.vvc_channel);
  j.at("vvc_instance_id").get_to(v.vvc_instance_id);
  j.at("vvc_cfg").get_to(v.vvc_cfg);
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

//}; // namespace uvvm_cosim
