#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "nlohmann/json.hpp"
#include "uvvm_cosim_server.hpp"
#include "uvvm_cosim_types.hpp"

// Split a string by delimiter into substrings, ignoring empty tokens.
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

// Parse "key=value,key=value,..." into a map. Values must be integers.
static std::map<std::string, int> parse_bfm_cfg_str(const std::string& cfg_str)
{
  std::map<std::string, int> bfm_cfg;
  try {
    for (auto& item : split_str(cfg_str, ",")) {
      auto kv = split_str(item, "=");
      if (kv.size() != 2) continue;
      bfm_cfg.emplace(kv[0], std::stoi(kv[1]));
    }
  } catch (std::exception& e) {
    std::cerr << "Exception parsing bfm_cfg string \"" << cfg_str
              << "\": " << e.what() << std::endl;
  }
  return bfm_cfg;
}

namespace uvvm_cosim {

// ---------------------------------------------------------------------------
// Helper: build a VvcIdentifier from string params
// ---------------------------------------------------------------------------

VvcIdentifier UvvmCosimServer::makeId(const std::string& vvc_type, VvcChannel channel,
                                       int vvc_instance_id) const
{
  return VvcIdentifier(to_VvcTypes(vvc_type), channel, vvc_instance_id);
}

// ---------------------------------------------------------------------------
// VHPI/FLI interface methods
// ---------------------------------------------------------------------------

void UvvmCosimServer::WaitForStartSim()
{
  using namespace std::chrono_literals;
  while (!getStartSim() && !getTerminateSim()) {
    std::this_thread::sleep_for(10ms);
  }
}

bool UvvmCosimServer::ShouldTerminateSim()
{
  return getTerminateSim();
}

bool UvvmCosimServer::VvcListenEnabled(std::string vvc_type, int vvc_instance_id)
{
  VvcChannel channel = (vvc_type == "UART_VVC") ? VvcChannel::RX : VvcChannel::NA;
  try {
    return GetVvcListenEnable(makeId(vvc_type, channel, vvc_instance_id));
  } catch (...) {
    return false;
  }
}

void UvvmCosimServer::AddVvc(std::string vvc_type, std::string vvc_channel,
                              int vvc_instance_id, std::string bfm_cfg_str)
{
  auto bfm_cfg = parse_bfm_cfg_str(bfm_cfg_str);
  UvvmCosimData::AddVvc(vvc_type, vvc_channel, vvc_instance_id, bfm_cfg);
}

bool UvvmCosimServer::TransmitQueueEmpty(std::string vvc_type, int vvc_instance_id)
{
  VvcChannel channel = (vvc_type == "UART_VVC") ? VvcChannel::TX : VvcChannel::NA;
  try {
    return StreamQueueEmpty(QID_TRANSMIT, makeId(vvc_type, channel, vvc_instance_id));
  } catch (...) {
    return true;
  }
}

std::optional<uint8_t> UvvmCosimServer::TransmitQueueGet(std::string vvc_type, int vvc_instance_id)
{
  VvcChannel channel = (vvc_type == "UART_VVC") ? VvcChannel::TX : VvcChannel::NA;
  try {
    auto result = StreamQueueGetByte(QID_TRANSMIT, makeId(vvc_type, channel, vvc_instance_id));
    if (result) return result.value().first;
  } catch (...) {}
  return std::nullopt;
}

void UvvmCosimServer::ReceiveQueuePut(std::string vvc_type, int vvc_instance_id, uint8_t byte)
{
  VvcChannel channel = (vvc_type == "UART_VVC") ? VvcChannel::RX : VvcChannel::NA;
  try {
    StreamQueuePutByte(QID_RECEIVE, makeId(vvc_type, channel, vvc_instance_id), byte, false);
  } catch (...) {}
}

bool UvvmCosimServer::TransmitPacketQueueEmpty(std::string vvc_type, int vvc_instance_id)
{
  try {
    return StreamQueueEmpty(QID_TRANSMIT, makeId(vvc_type, VvcChannel::NA, vvc_instance_id));
  } catch (...) {
    return true;
  }
}

auto UvvmCosimServer::TransmitPacketQueueGet(std::string vvc_type, int vvc_instance_id)
    -> std::optional<std::pair<uint8_t, bool>>
{
  try {
    return StreamQueueGetByte(QID_TRANSMIT, makeId(vvc_type, VvcChannel::NA, vvc_instance_id));
  } catch (...) {
    return std::nullopt;
  }
}

void UvvmCosimServer::ReceivePacketQueuePut(std::string vvc_type, int vvc_instance_id,
                                             uint8_t byte, bool eop)
{
  try {
    StreamQueuePutByte(QID_RECEIVE, makeId(vvc_type, VvcChannel::NA, vvc_instance_id), byte, eop);
  } catch (...) {}
}

// ---------------------------------------------------------------------------
// JSON-RPC procedures — simulation control
// ---------------------------------------------------------------------------

JsonResponse UvvmCosimServer::StartSim()
{
  setStartSim(true);
  return {.success = true};
}

JsonResponse UvvmCosimServer::PauseSim()
{
  setStartSim(false);
  return {.success = true};
}

JsonResponse UvvmCosimServer::TerminateSim()
{
  setTerminateSim(true);
  std::cout << "\n\nTERMINATING SIM!!!\n\n";
  return {.success = true};
}

// ---------------------------------------------------------------------------
// JSON-RPC procedures — VVC management
// ---------------------------------------------------------------------------

JsonResponse UvvmCosimServer::GetVvcList()
{
  std::vector<VvcInfo> vec = UvvmCosimData::GetVvcList();
  JsonResponse response;
  response.success = true;
  response.result  = json(vec);
  return response;
}

JsonResponse UvvmCosimServer::SetVvcListenEnable(std::string vvc_type, int vvc_id, bool enable)
{
  VvcChannel channel = (vvc_type == "UART_VVC") ? VvcChannel::RX : VvcChannel::NA;
  JsonResponse response;
  try {
    UvvmCosimData::SetVvcListenEnable(makeId(vvc_type, channel, vvc_id), enable);
    response.success = true;
  } catch (const std::exception& e) {
    response.success = false;
    response.result  = json{{"error", e.what()}};
  }
  return response;
}

// ---------------------------------------------------------------------------
// JSON-RPC procedures — streaming VVCs
// ---------------------------------------------------------------------------

JsonResponse UvvmCosimServer::TransmitBytes(std::string vvc_type, int vvc_id,
                                             std::vector<uint8_t> data)
{
  VvcChannel channel = (vvc_type == "UART_VVC") ? VvcChannel::TX : VvcChannel::NA;
  JsonResponse response;
  try {
    StreamQueuePutPacket(QID_TRANSMIT, makeId(vvc_type, channel, vvc_id), data);
    response.success = true;
  } catch (const std::exception& e) {
    response.success = false;
    response.result  = json{{"error", e.what()}};
  }
  return response;
}

JsonResponse UvvmCosimServer::TransmitPacket(std::string vvc_type, int vvc_id,
                                              std::vector<uint8_t> data)
{
  JsonResponse response;
  try {
    StreamQueuePutPacket(QID_TRANSMIT, makeId(vvc_type, VvcChannel::NA, vvc_id), data);
    response.success = true;
  } catch (const std::exception& e) {
    response.success = false;
    response.result  = json{{"error", e.what()}};
  }
  return response;
}

JsonResponse UvvmCosimServer::ReceiveBytes(std::string vvc_type, int vvc_id,
                                            int num_bytes, bool exact_length)
{
  VvcChannel channel = (vvc_type == "UART_VVC") ? VvcChannel::RX : VvcChannel::NA;
  JsonResponse response;
  try {
    auto id = makeId(vvc_type, channel, vvc_id);
    std::vector<uint8_t> data;
    if (!StreamQueueEmpty(QID_RECEIVE, id)) {
      if (exact_length) {
        if (StreamQueueSize(QID_RECEIVE, id) >= static_cast<size_t>(num_bytes)) {
          data = StreamQueueGetPacket(QID_RECEIVE, id, num_bytes);
        }
      } else {
        data = StreamQueueGetPacket(QID_RECEIVE, id, num_bytes);
      }
    }
    response.success = true;
    response.result  = json{{"data", data}};
  } catch (const std::exception& e) {
    response.success = false;
    response.result  = json{{"error", e.what()}};
  }
  return response;
}

JsonResponse UvvmCosimServer::ReceivePacket(std::string vvc_type, int vvc_id)
{
  VvcChannel channel = (vvc_type == "UART_VVC") ? VvcChannel::RX : VvcChannel::NA;
  JsonResponse response;
  try {
    auto id = makeId(vvc_type, channel, vvc_id);
    std::vector<uint8_t> pkt;
    if (!StreamQueueEmpty(QID_RECEIVE, id)) {
      pkt = StreamQueueGetPacket(QID_RECEIVE, id, 0);
    }
    response.success = true;
    response.result  = json{{"data", pkt}};
  } catch (const std::exception& e) {
    response.success = false;
    response.result  = json{{"error", e.what()}};
  }
  return response;
}

// ---------------------------------------------------------------------------
// JSON-RPC procedures — memory-mapped VVCs
// ---------------------------------------------------------------------------

JsonResponse UvvmCosimServer::WriteRequest(std::string vvc_type, int vvc_id, uint64_t addr,
                                            std::vector<uint8_t> data, bool addr_increment)
{
  JsonResponse response;
  try {
    Transfer trans{addr, std::move(data), addr_increment, 0};
    MemQueuePut(QID_REQUEST, makeId(vvc_type, VvcChannel::NA, vvc_id), trans);
    response.success = true;
  } catch (const std::exception& e) {
    response.success = false;
    response.result  = json{{"error", e.what()}};
  }
  return response;
}

JsonResponse UvvmCosimServer::WriteResponse(std::string vvc_type, int vvc_id)
{
  JsonResponse response;
  try {
    auto trans = MemQueueGet(QID_RESPONSE, makeId(vvc_type, VvcChannel::NA, vvc_id));
    response.success = true;
    response.result  = json{{"available", trans.has_value()}};
  } catch (const std::exception& e) {
    response.success = false;
    response.result  = json{{"error", e.what()}};
  }
  return response;
}

JsonResponse UvvmCosimServer::ReadRequest(std::string vvc_type, int vvc_id,
                                           uint64_t addr, uint32_t num_bytes)
{
  JsonResponse response;
  try {
    Transfer trans{addr, {}, false, num_bytes};
    MemQueuePut(QID_REQUEST, makeId(vvc_type, VvcChannel::NA, vvc_id), trans);
    response.success = true;
  } catch (const std::exception& e) {
    response.success = false;
    response.result  = json{{"error", e.what()}};
  }
  return response;
}

JsonResponse UvvmCosimServer::ReadResponse(std::string vvc_type, int vvc_id)
{
  JsonResponse response;
  try {
    auto trans = MemQueueGet(QID_RESPONSE, makeId(vvc_type, VvcChannel::NA, vvc_id));
    response.success = true;
    if (trans) {
      response.result = json{{"data", trans.value().data}};
    } else {
      response.result = json{{"data", json::array()}};
    }
  } catch (const std::exception& e) {
    response.success = false;
    response.result  = json{{"error", e.what()}};
  }
  return response;
}

} // namespace uvvm_cosim
