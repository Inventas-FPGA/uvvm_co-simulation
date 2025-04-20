#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>
#include <jsonrpccxx/client.hpp>
#include <jsonrpccxx/iclientconnector.hpp>
#include "uvvm_cosim_types.hpp"

namespace uvvm_cosim {

class UvvmCosimClient : private jsonrpccxx::JsonRpcClient {
  int requestId = 0;

public:
  explicit UvvmCosimClient(jsonrpccxx::IClientConnector &connector)
    : jsonrpccxx::JsonRpcClient(connector, jsonrpccxx::version::v2)
  {
  }

  JsonResponse StartSim() {
    return CallMethod<JsonResponse>(requestId++, "StartSim", {});
  }

  JsonResponse PauseSim() {
    return CallMethod<JsonResponse>(requestId++, "PauseSim", {});
  }

  JsonResponse TerminateSim() {
    return CallMethod<JsonResponse>(requestId++, "TerminateSim", {});
  }

  JsonResponse GetVvcList() {
    return CallMethod<JsonResponse>(requestId++, "GetVvcList", {});
  }

  JsonResponse SetVvcListenEnable(std::string vvc_type, int vvc_id, bool enable) {
    return CallMethod<JsonResponse>(requestId++, "SetVvcListenEnable", {vvc_type, vvc_id, enable});
  }

  JsonResponse TransmitBytes(std::string vvc_type, int vvc_id, std::vector<uint8_t> data)
  {
    return CallMethod<JsonResponse>(requestId++, "TransmitBytes", {vvc_type, vvc_id, data});
  }

  JsonResponse TransmitPacket(std::string vvc_type, int vvc_id, std::vector<uint8_t> pkt)
  {
    return CallMethod<JsonResponse>(requestId++, "TransmitPacket", {vvc_type, vvc_id, pkt});
  }

  JsonResponse ReceiveBytes(std::string vvc_type, int vvc_id, int num_bytes, bool exact_length)
  {
    return CallMethod<JsonResponse>(requestId++, "ReceiveBytes", {vvc_type, vvc_id, num_bytes, exact_length});
  }

  JsonResponse ReceivePacket(std::string vvc_type, int vvc_id)
  {
    return CallMethod<JsonResponse>(requestId++, "ReceivePacket", {vvc_type, vvc_id});
  }

};

} // namespace uvvm_cosim
