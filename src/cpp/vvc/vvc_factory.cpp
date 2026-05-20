#include <iostream>
#include <memory>
#include <stdexcept>
#include "vvc_factory.hpp"
#include "vvc_byte_stream.hpp"
#include "vvc_packet_stream.hpp"
#include "vvc_mem_mapped.hpp"

namespace uvvm_cosim {

std::unique_ptr<VvcBase>
VvcFactory::create(VvcTypes vvc_type, VvcChannel channel, int instance_id,
                   const std::map<std::string, int>& bfm_cfg)
{
  std::unique_ptr<VvcBase> vvc;

  switch (vvc_type) {
    case VvcTypes::UART:
    case VvcTypes::AXI_STREAM:
    case VvcTypes::AVALON_ST: {
      bool packet_based = bfm_cfg.count("packet_based") &&
                          bfm_cfg.at("packet_based") != 0;
      if (packet_based) {
        vvc = std::make_unique<VvcPacketStreamBased>(vvc_type, channel, instance_id);
      } else {
        vvc = std::make_unique<VvcByteStream>(vvc_type, channel, instance_id);
      }
      break;
    }

    case VvcTypes::AXI:
    case VvcTypes::AXILITE:
    case VvcTypes::AVALON_MM:
      vvc = std::make_unique<VvcMemMapped>(vvc_type, channel, instance_id);
      break;

    default:
      throw std::invalid_argument("VvcFactory: unknown VVC type: " +
                                  to_string(vvc_type));
  }

  // Set common config fields from bfm_cfg
  if (auto it = bfm_cfg.find("cosim_support"); it != bfm_cfg.end()) {
    vvc->cosim_support = it->second != 0;
  }

  // Store remaining bfm_cfg entries (strip the well-known ones)
  for (auto& [k, v] : bfm_cfg) {
    if (k != "cosim_support" && k != "packet_based") {
      vvc->bfm_cfg[k] = v;
    }
  }

  return vvc;
}

std::unique_ptr<VvcBase>
VvcFactory::create(const std::string& vvc_type_str,
                   const std::string& vvc_channel_str,
                   int instance_id,
                   const std::map<std::string, int>& bfm_cfg)
{
  VvcTypes   vvc_type = to_VvcTypes(vvc_type_str);
  VvcChannel channel  = to_VvcChannel(vvc_channel_str);
  return create(vvc_type, channel, instance_id, bfm_cfg);
}

} // namespace uvvm_cosim
