#pragma once
#include <atomic>
#include <cstdint>
#include <iostream>
#include <optional>
#include <utility>
#include <vector>
#include <jsonrpccxx/server.hpp>
#include <cpphttplibconnector.hpp>
#include "uvvm_cosim_types.hpp"
#include "uvvm_cosim_data.hpp"

namespace uvvm_cosim {

class UvvmCosimServer {
private:

  jsonrpccxx::JsonRpc2Server jsonRpcServer;
  CppHttpLibServerConnector httpServer;
  UvvmCosimData cosimData;

  // --------------------------------------------------------------------------
  // JSON-RPC remote procedures
  // --------------------------------------------------------------------------

  JsonResponse StartSim();
  JsonResponse PauseSim();
  JsonResponse TerminateSim();
  JsonResponse GetVvcList();
  JsonResponse SetVvcListenEnable(std::string vvc_type, int vvc_id, bool enable);

  JsonResponse TransmitBytes(std::string vvc_type, int vvc_id, std::vector<uint8_t> data);
  JsonResponse TransmitPacket(std::string vvc_type, int vvc_id, std::vector<uint8_t> data);

  JsonResponse ReceiveBytes(std::string vvc_type, int vvc_id, int num_bytes, bool exact_length);
  JsonResponse ReceivePacket(std::string vvc_type, int vvc_id);

public:
  UvvmCosimServer(int port)
    : jsonRpcServer()
    , httpServer(jsonRpcServer, port)
  {
    using namespace jsonrpccxx;

    // Add JSON-RPC procedures

    jsonRpcServer.Add("TransmitBytes",
                      GetHandle(&UvvmCosimServer::TransmitBytes, *this),
                      {"vvc_type", "vvc_id", "data"});

    jsonRpcServer.Add("TransmitPacket",
                      GetHandle(&UvvmCosimServer::TransmitPacket, *this),
                      {"vvc_type", "vvc_id", "data"});

    jsonRpcServer.Add("ReceiveBytes",
                      GetHandle(&UvvmCosimServer::ReceiveBytes, *this),
                      {"vvc_type", "vvc_id", "num_bytes", "exact_length"});

    jsonRpcServer.Add("ReceivePacket",
                      GetHandle(&UvvmCosimServer::ReceivePacket, *this),
                      {"vvc_type", "vvc_id"});

    jsonRpcServer.Add("GetVvcList",
                      GetHandle(&UvvmCosimServer::GetVvcList, *this), {});

    jsonRpcServer.Add("SetVvcListenEnable",
                      GetHandle(&UvvmCosimServer::SetVvcListenEnable, *this),
		      {"vvc_type", "vvc_id", "enable"});

    jsonRpcServer.Add("StartSim",
		      GetHandle(&UvvmCosimServer::StartSim, *this), {});

    jsonRpcServer.Add("PauseSim",
		      GetHandle(&UvvmCosimServer::PauseSim, *this), {});

    jsonRpcServer.Add("TerminateSim",
		      GetHandle(&UvvmCosimServer::TerminateSim, *this), {});
  }

  ~UvvmCosimServer()
  {
    httpServer.StopListening();
  }

  // --------------------------------------------------------------------------
  // Methods used by VHPI/FLI code
  // --------------------------------------------------------------------------

  void StartListening()
  {
    httpServer.StartListening();
  }

  void StopListening()
  {
    httpServer.StopListening();
  }

  void WaitForStartSim();
  bool ShouldTerminateSim();

  bool VvcListenEnabled(std::string vvc_type,
			   int vvc_instance_id);

  void AddVvc(std::string vvc_type, std::string vvc_channel,
	      int vvc_instance_id, std::string bfm_cfg_str);

  bool TransmitQueueEmpty(std::string vvc_type, int vvc_instance_id);

  std::optional<uint8_t> TransmitQueueGet(std::string vvc_type, int vvc_instance_id);

  void ReceiveQueuePut(std::string vvc_type, int vvc_instance_id, uint8_t byte);

  bool TransmitPacketQueueEmpty(std::string vvc_type, int vvc_instance_id);

  auto TransmitPacketQueueGet(std::string vvc_type, int vvc_instance_id) -> std::optional<std::pair<uint8_t, bool>>;

  void ReceivePacketQueuePut(std::string vvc_type, int vvc_instance_id, uint8_t byte, bool eop);

};
  
} // namespace uvvm_cosim

