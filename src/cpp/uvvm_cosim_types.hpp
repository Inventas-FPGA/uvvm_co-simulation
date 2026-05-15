#pragma once
#include <map>
#include <string>
#include "nlohmann/json.hpp"
#include "vvc/vvc_base.hpp"

namespace uvvm_cosim {

using json = nlohmann::json;

// Comparator for VvcIdentifier, required for use as a std::map key.
class VvcCompare {
public:
  bool operator()(const VvcIdentifier& lhs, const VvcIdentifier& rhs) const {
    if (lhs.vvc_type    != rhs.vvc_type)       return lhs.vvc_type    < rhs.vvc_type;
    if (lhs.vvc_channel != rhs.vvc_channel)    return lhs.vvc_channel < rhs.vvc_channel;
    return lhs.vvc_instance_id < rhs.vvc_instance_id;
  }
};

// Plain struct used for GetVvcList JSON responses — avoids polymorphic serialization.
struct VvcInfo {
  std::string              vvc_type;
  std::string              vvc_channel;
  int                      vvc_instance_id;
  bool                     listen_enable;
  bool                     cosim_support;
  bool                     packet_based;
  std::map<std::string, int> bfm_cfg;
};

inline void to_json(json& j, const VvcInfo& v) {
  j = json{
    {"vvc_type",        v.vvc_type},
    {"vvc_channel",     v.vvc_channel},
    {"vvc_instance_id", v.vvc_instance_id},
    {"listen_enable",   v.listen_enable},
    {"cosim_support",   v.cosim_support},
    {"packet_based",    v.packet_based},
    {"bfm_cfg",         v.bfm_cfg}
  };
}

struct JsonResponse {
  bool success;
  json result;
};

inline void to_json(json& j, const JsonResponse& response) {
  j = json{{"success", response.success},
           {"result",  response.result}};
}

inline void from_json(const json& j, JsonResponse& response) {
  j.at("success").get_to(response.success);
  j.at("result").get_to(response.result);
}

} // namespace uvvm_cosim
