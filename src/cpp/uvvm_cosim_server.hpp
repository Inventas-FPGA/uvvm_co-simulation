#pragma once
#include <atomic>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <jsonrpccxx/server.hpp>
#include <cpphttplibconnector.hpp>
#include "uvvm_cosim_types.hpp"
#include "uvvm_cosim_data.hpp"

namespace uvvm_cosim {

class UvvmCosimServer : public UvvmCosimData {
private:
  jsonrpccxx::JsonRpc2Server jsonRpcServer;
  CppHttpLibServerConnector  httpServer;

  // --------------------------------------------------------------------------
  // JSON-RPC remote procedures — streaming VVCs
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

  // --------------------------------------------------------------------------
  // JSON-RPC remote procedures — memory-mapped VVCs
  // --------------------------------------------------------------------------
  JsonResponse WriteRequest(std::string vvc_type, int vvc_id, uint64_t addr,
                            std::vector<uint8_t> data, bool addr_increment);
  JsonResponse WriteResponse(std::string vvc_type, int vvc_id);
  JsonResponse ReadRequest(std::string vvc_type, int vvc_id, uint64_t addr, uint32_t num_bytes);
  JsonResponse ReadResponse(std::string vvc_type, int vvc_id);

  // Build a VvcIdentifier from the string-based parameters supplied by JSON-RPC / VHPI/FLI.
  // For UART_VVC the channel is inferred from the queue direction (TX for transmit, RX for others).
  VvcIdentifier makeId(const std::string& vvc_type, VvcChannel channel, int vvc_instance_id) const;

public:
  explicit UvvmCosimServer(int port)
    : jsonRpcServer()
    , httpServer(jsonRpcServer, port)
  {
    using namespace jsonrpccxx;

    jsonRpcServer.Add("TransmitBytes",
                      GetHandle(&UvvmCosimServer::TransmitBytes,  *this),
                      {"vvc_type", "vvc_id", "data"});
    jsonRpcServer.Add("TransmitPacket",
                      GetHandle(&UvvmCosimServer::TransmitPacket, *this),
                      {"vvc_type", "vvc_id", "data"});
    jsonRpcServer.Add("ReceiveBytes",
                      GetHandle(&UvvmCosimServer::ReceiveBytes,   *this),
                      {"vvc_type", "vvc_id", "num_bytes", "exact_length"});
    jsonRpcServer.Add("ReceivePacket",
                      GetHandle(&UvvmCosimServer::ReceivePacket,  *this),
                      {"vvc_type", "vvc_id"});

    jsonRpcServer.Add("SendWriteRequest",
                      GetHandle(&UvvmCosimServer::WriteRequest,   *this),
                      {"vvc_type", "vvc_id", "addr", "data", "addr_increment"});
    jsonRpcServer.Add("GetWriteResponse",
                      GetHandle(&UvvmCosimServer::WriteResponse,  *this),
                      {"vvc_type", "vvc_id"});
    jsonRpcServer.Add("SendReadRequest",
                      GetHandle(&UvvmCosimServer::ReadRequest,    *this),
                      {"vvc_type", "vvc_id", "addr", "num_bytes"});
    jsonRpcServer.Add("GetReadResponse",
                      GetHandle(&UvvmCosimServer::ReadResponse,   *this),
                      {"vvc_type", "vvc_id"});

    jsonRpcServer.Add("GetVvcList",
                      GetHandle(&UvvmCosimServer::GetVvcList,     *this), {});
    jsonRpcServer.Add("SetVvcListenEnable",
                      GetHandle(&UvvmCosimServer::SetVvcListenEnable, *this),
                      {"vvc_type", "vvc_id", "enable"});
    jsonRpcServer.Add("StartSim",
                      GetHandle(&UvvmCosimServer::StartSim,       *this), {});
    jsonRpcServer.Add("PauseSim",
                      GetHandle(&UvvmCosimServer::PauseSim,       *this), {});
    jsonRpcServer.Add("TerminateSim",
                      GetHandle(&UvvmCosimServer::TerminateSim,   *this), {});
  }

  ~UvvmCosimServer() {
    httpServer.StopListening();
  }

  // --------------------------------------------------------------------------
  // Methods called from VHPI/FLI foreign functions (uvvm_cosim_common.cpp)
  // --------------------------------------------------------------------------

  void StartListening()  { httpServer.StartListening();  }
  void StopListening()   { httpServer.StopListening();   }

  void WaitForStartSim();
  bool ShouldTerminateSim();

  bool VvcListenEnabled(std::string vvc_type, int vvc_instance_id);

  void AddVvc(std::string vvc_type, std::string vvc_channel,
              int vvc_instance_id, std::string bfm_cfg_str);

  bool TransmitQueueEmpty(std::string vvc_type, int vvc_instance_id);
  std::optional<uint8_t> TransmitQueueGet(std::string vvc_type, int vvc_instance_id);
  void ReceiveQueuePut(std::string vvc_type, int vvc_instance_id, uint8_t byte);

  bool TransmitPacketQueueEmpty(std::string vvc_type, int vvc_instance_id);
  auto TransmitPacketQueueGet(std::string vvc_type, int vvc_instance_id)
       -> std::optional<std::pair<uint8_t, bool>>;
  void ReceivePacketQueuePut(std::string vvc_type, int vvc_instance_id, uint8_t byte, bool eop);

  // Memory-mapped VVC bridge — called from VHPI/FLI.
  // MmRequestFetch atomically pops the front Transfer from QID_REQUEST into a
  // per-VVC cache; subsequent MmRequestIsWrite/GetAddr/GetNumBytes/GetByte
  // reads come from that cache. MmResponsePutByte stages bytes that are then
  // finalized into QID_RESPONSE by MmResponseReadDone. These VHPI-side caches
  // are only touched from the simulation thread so need no extra locking.
  bool     MmRequestEmpty(std::string vvc_type, int vvc_instance_id);
  void     MmRequestFetch(std::string vvc_type, int vvc_instance_id);
  bool     MmRequestIsWrite(std::string vvc_type, int vvc_instance_id);
  uint32_t MmRequestGetAddr(std::string vvc_type, int vvc_instance_id);
  int      MmRequestGetNumBytes(std::string vvc_type, int vvc_instance_id);
  uint8_t  MmRequestGetByte(std::string vvc_type, int vvc_instance_id, int byte_idx);
  void     MmResponseWriteDone(std::string vvc_type, int vvc_instance_id);
  void     MmResponsePutByte(std::string vvc_type, int vvc_instance_id, uint8_t byte);
  void     MmResponseReadDone(std::string vvc_type, int vvc_instance_id);

private:
  using VvcKey = std::pair<std::string, int>;
  std::map<VvcKey, Transfer>             mm_pending_request;
  std::map<VvcKey, std::vector<uint8_t>> mm_read_staging;
};

} // namespace uvvm_cosim
