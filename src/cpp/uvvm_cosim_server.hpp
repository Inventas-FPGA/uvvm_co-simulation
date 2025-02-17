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
#include "shared_map.hpp"

class UvvmCosimServer {
private:

  jsonrpccxx::JsonRpc2Server jsonRpcServer;
  CppHttpLibServerConnector httpServer;

  // VvcInstanceKey: Identifies VVC by type, channel, id
  // VvcInstanceData: Transmit+receive queue for VVC, config, etc.
  // VvcCompare: Comparator class for VvcInstanceKey
  shared_map<VvcInstanceKey, VvcInstanceData, VvcCompare> vvcInstanceMap;

  std::atomic<bool> startSim=false;

  // --------------------------------------------------------------------------
  // JSON-RPC remote procedures
  // --------------------------------------------------------------------------

  JsonResponse StartSim();
  JsonResponse GetVvcList();
  JsonResponse SetVvcCosimRecvState(std::string vvc_type, int vvc_id, bool enable);

  JsonResponse TransmitBytes(std::string vvc_type, int vvc_id, std::vector<uint8_t> data);
  JsonResponse TransmitPacket(std::string vvc_type, int vvc_id, std::vector<uint8_t> data);

  JsonResponse ReceiveBytes(std::string vvc_type, int vvc_id, int length, bool all_or_nothing);
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
                      {"vvc_type", "vvc_id", "length", "all_or_nothing"});

    jsonRpcServer.Add("ReceivePacket",
                      GetHandle(&UvvmCosimServer::ReceivePacket, *this),
                      {"vvc_type", "vvc_id", "length", "all_or_nothing"});

    jsonRpcServer.Add("GetVvcList",
                      GetHandle(&UvvmCosimServer::GetVvcList, *this), {});

    jsonRpcServer.Add("SetVvcCosimRecvState",
                      GetHandle(&UvvmCosimServer::SetVvcCosimRecvState, *this),
		      {"vvc_type", "vvc_id", "enable"});

    jsonRpcServer.Add("StartSim",
		      GetHandle(&UvvmCosimServer::StartSim, *this), {});
  }

  ~UvvmCosimServer()
  {
    httpServer.StopListening();
  }

  // --------------------------------------------------------------------------
  // Methods used by VHPI code
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

  bool VvcCosimRecvEnabled(std::string vvc_type,
			   int vvc_instance_id);

  void AddVvc(std::string vvc_type, std::string vvc_channel,
	      int vvc_instance_id, std::string bfm_cfg_str);

  bool TransmitQueueEmpty(std::string vvc_type, int vvc_instance_id);

  std::optional<std::pair<uint8_t, bool>> TransmitQueueGet(std::string vvc_type, int vvc_instance_id);

  void ReceiveQueuePut(std::string vvc_type, int vvc_instance_id, uint8_t byte, bool end_of_packet=false);

};
  
