#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>
#include <jsonrpccxx/client.hpp>
#include <jsonrpccxx/iclientconnector.hpp>
#include "uvvm_cosim_types.hpp"


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

  // JsonResponse TransmitPacket(std::string vvc_type, int vvc_id, std::vector<uint8_t> data)
  // {
  // }

  JsonResponse ReceiveBytes(std::string vvc_type, int vvc_id, int length, bool all_or_nothing)
  {
    return CallMethod<JsonResponse>(requestId++, "ReceiveBytes", {vvc_type, vvc_id, length, all_or_nothing});
  }

  // JsonResponse ReceivePacket(std::string vvc_type, int vvc_id);
  // {
  // }

};
