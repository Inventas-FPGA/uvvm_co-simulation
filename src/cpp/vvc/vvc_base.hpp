#pragma once
#include <map>
#include <stdexcept>
#include <string>

namespace uvvm_cosim {

enum class VvcTypes {
  AXI_STREAM, AXI, AXILITE, AVALON_ST, AVALON_MM, UART
};

enum class VvcChannel {
  NA, RX, TX
};

// operator< needed for use as std::map keys via VvcCompare
inline bool operator<(VvcTypes a, VvcTypes b) {
  return static_cast<int>(a) < static_cast<int>(b);
}

inline bool operator<(VvcChannel a, VvcChannel b) {
  return static_cast<int>(a) < static_cast<int>(b);
}

inline std::string to_string(VvcTypes vvc_type) {
  switch (vvc_type) {
    case VvcTypes::AXI_STREAM: return "AXISTREAM_VVC";
    case VvcTypes::AXI:        return "AXI_VVC";
    case VvcTypes::AXILITE:    return "AXILITE_VVC";
    case VvcTypes::AVALON_ST:  return "AVALON_ST_VVC";
    case VvcTypes::AVALON_MM:  return "AVALON_MM_VVC";
    case VvcTypes::UART:       return "UART_VVC";
    default:                   return "UNKNOWN_VVC";
  }
}

inline VvcTypes to_VvcTypes(const std::string& str) {
  if (str == "AXISTREAM_VVC") return VvcTypes::AXI_STREAM;
  if (str == "AXI_VVC")       return VvcTypes::AXI;
  if (str == "AXILITE_VVC")   return VvcTypes::AXILITE;
  if (str == "AVALON_ST_VVC") return VvcTypes::AVALON_ST;
  if (str == "AVALON_MM_VVC") return VvcTypes::AVALON_MM;
  if (str == "UART_VVC")      return VvcTypes::UART;
  throw std::invalid_argument("Unknown VVC type: " + str);
}

inline std::string to_string(VvcChannel channel) {
  switch (channel) {
    case VvcChannel::NA: return "NA";
    case VvcChannel::RX: return "RX";
    case VvcChannel::TX: return "TX";
    default:             return "NA";
  }
}

inline VvcChannel to_VvcChannel(const std::string& str) {
  if (str == "RX") return VvcChannel::RX;
  if (str == "TX") return VvcChannel::TX;
  return VvcChannel::NA;
}

// QueueId is shared by stream and mem-mapped VVCs.
// Stream VVCs use QID_TRANSMIT / QID_RECEIVE.
// Mem-mapped VVCs use QID_REQUEST / QID_RESPONSE.
enum QueueId { QID_TRANSMIT, QID_RECEIVE, QID_REQUEST, QID_RESPONSE, QID_MAX };

struct VvcIdentifier {
  VvcTypes   vvc_type;
  VvcChannel vvc_channel;
  int        vvc_instance_id;

  VvcIdentifier(VvcTypes t, VvcChannel c, int id)
    : vvc_type(t), vvc_channel(c), vvc_instance_id(id) {}
};

inline std::string to_string(const VvcIdentifier& vvc) {
  return "type=" + to_string(vvc.vvc_type) +
         ", channel=" + to_string(vvc.vvc_channel) +
         ", instance_id=" + std::to_string(vvc.vvc_instance_id);
}

class VvcBase : public VvcIdentifier {
public:
  bool listen_enable = false;
  bool cosim_support = false;
  std::map<std::string, int> bfm_cfg;

  VvcBase(VvcTypes vvc_type, VvcChannel vvc_channel, int vvc_instance_id)
    : VvcIdentifier(vvc_type, vvc_channel, vvc_instance_id)
  {}

  virtual ~VvcBase() = default;
};

} // namespace uvvm_cosim
