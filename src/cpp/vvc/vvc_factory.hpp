#pragma once
#include <map>
#include <memory>
#include <string>
#include "vvc_base.hpp"

namespace uvvm_cosim {

class VvcFactory {
public:
  // Create a VVC object of the appropriate concrete type.
  // vvc_type and channel are parsed from the strings supplied by the VHPI/FLI layer.
  // bfm_cfg is the parsed config map; factory extracts cosim_support and packet_based,
  // stores the remainder in VvcBase::bfm_cfg.
  static std::unique_ptr<VvcBase> create(VvcTypes vvc_type, VvcChannel channel,
                                         int instance_id,
                                         const std::map<std::string, int>& bfm_cfg);

  // String-based overload; converts type/channel strings and delegates to the above.
  static std::unique_ptr<VvcBase> create(const std::string& vvc_type_str,
                                         const std::string& vvc_channel_str,
                                         int instance_id,
                                         const std::map<std::string, int>& bfm_cfg);
};

} // namespace uvvm_cosim
