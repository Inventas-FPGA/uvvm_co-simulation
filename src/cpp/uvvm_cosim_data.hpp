#pragma once
#include <atomic>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "shared_map.hpp"
#include "uvvm_cosim_types.hpp"

namespace uvvm_cosim {

// VvcInstanceKey: Identifies VVC by type, channel, id
// VvcInstanceData: Transmit+receive queue for VVC, config, etc.
// VvcCompare: Comparator class for VvcInstanceKey
using VvcMap = shared_map<VvcInstanceKey, VvcInstanceData, VvcCompare>;
using VvcMapInternal = std::map<VvcInstanceKey, VvcInstanceData, VvcCompare>;

class UvvmCosimData {
private:
  VvcMap vvcInstanceMap;

  std::atomic<bool> startSim = false;
  std::atomic<bool> terminateSim = false;

private:

  /////////////////////////////////////////////////////////////////////////////
  // Byte queue private functions
  /////////////////////////////////////////////////////////////////////////////

  auto get_byte_queue(VvcMapInternal& vvc_map, VvcInstanceKey vvc, QueueId qid) -> ByteQueue&;

  bool byte_queue_empty(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc);

  size_t byte_queue_size(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc);

  void byte_queue_put(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc, uint8_t byte);

  void byte_queue_put(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc,
                      const std::vector<uint8_t>& data);

  auto byte_queue_get(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc) -> std::optional<uint8_t>;

  auto byte_queue_get(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc, int num_bytes) -> std::vector<uint8_t>;


  /////////////////////////////////////////////////////////////////////////////
  // Packet queue private functions
  /////////////////////////////////////////////////////////////////////////////

  auto get_packet_queue(VvcMapInternal& vvc_map, VvcInstanceKey vvc, QueueId qid) -> PacketQueue&;

  bool packet_queue_empty(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc);

  size_t packet_queue_size(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc);

  auto packet_queue_get_byte(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc) -> std::optional<std::pair<uint8_t, bool>>;

  auto packet_queue_get_pkt(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc) -> std::vector<uint8_t>;

  void packet_queue_put_byte(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc, uint8_t byte, bool eop);

  void packet_queue_put_pkt(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc, const std::vector<uint8_t>& pkt);

public:
  UvvmCosimData() {}

  ~UvvmCosimData() {}

  /////////////////////////////////////////////////////////////////////////////
  // Getters/setters for flags
  /////////////////////////////////////////////////////////////////////////////

  void setStartSim(bool value) {
    startSim = value;
  }

  bool getStartSim() const {
    return startSim;
  }

  void setTerminateSim(bool value) {
    terminateSim = value;
  }

  bool getTerminateSim() const {
    return terminateSim;
  }

  /////////////////////////////////////////////////////////////////////////////
  // VVC list public functions
  /////////////////////////////////////////////////////////////////////////////

  void AddVvc(VvcInstanceKey vvc, std::map<std::string, int> bfm_cfg);

  std::vector<VvcInstance> GetVvcList() const;

  void SetVvcListenEnable(VvcInstanceKey vvc, bool enable);

  bool GetVvcListenEnable(VvcInstanceKey vvc) const;

  /////////////////////////////////////////////////////////////////////////////
  // Byte queue public functions
  /////////////////////////////////////////////////////////////////////////////

  bool byte_queue_empty(QueueId qid, VvcInstanceKey vvc);

  size_t byte_queue_size(QueueId qid, VvcInstanceKey vvc);

  void byte_queue_put(QueueId qid, VvcInstanceKey vvc, uint8_t byte);

  void byte_queue_put(QueueId qid, VvcInstanceKey vvc, const std::vector<uint8_t>& data);

  auto byte_queue_get(QueueId qid, VvcInstanceKey vvc) -> std::optional<uint8_t>;

  auto byte_queue_get(QueueId qid, VvcInstanceKey vvc, int num_bytes) -> std::vector<uint8_t>;


  /////////////////////////////////////////////////////////////////////////////
  // Packet queue public functions
  /////////////////////////////////////////////////////////////////////////////
  bool packet_queue_empty(QueueId qid, VvcInstanceKey vvc);

  size_t packet_queue_size(QueueId qid, VvcInstanceKey vvc);

  auto packet_queue_get_byte(QueueId qid, VvcInstanceKey vvc) -> std::optional<std::pair<uint8_t, bool>>;

  auto packet_queue_get_pkt(QueueId qid, VvcInstanceKey vvc) -> std::vector<uint8_t>;

  void packet_queue_put_byte(QueueId qid, VvcInstanceKey vvc, uint8_t byte, bool eop);

  void packet_queue_put_pkt(QueueId qid, VvcInstanceKey vvc, const std::vector<uint8_t>& pkt);
};

} // namespace uvvm_cosim
