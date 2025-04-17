#pragma once
#include <atomic>
#include <initializer_list>
#include <map>
#include <stdexcept>
#include <string>
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

  auto get_byte_queue(VvcMapInternal& vvc_map, VvcInstanceKey vvc, QueueId qid) -> ByteQueue&
  {
    if (auto it = vvc_map.find(vvc); it != vvc_map.end()) {
      return it->second.byte_queues[qid];
    } else {
      throw std::runtime_error("VVC " + to_string(vvc) + " does not exist.");
    }
  }

  bool byte_queue_empty(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc)
  {
    return get_byte_queue(vvc_map, vvc, qid).empty();
  }

  size_t byte_queue_size(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc)
  {
    return get_byte_queue(vvc_map, vvc, qid).size();
  }

  void byte_queue_put(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc, uint8_t byte)
  {
    get_byte_queue(vvc_map, vvc, qid).put(byte);
  }

  void byte_queue_put(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc,
                      const std::vector<uint8_t>& data)
  {
    get_byte_queue(vvc_map, vvc, qid).put(data);
  }

  auto byte_queue_get(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc) -> std::optional<uint8_t>
  {
    return get_byte_queue(vvc_map, vvc, qid).get();
  }

  auto byte_queue_get(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc, int num_bytes) -> std::vector<uint8_t>
  {
    return get_byte_queue(vvc_map, vvc, qid).get(num_bytes);
  }


  /////////////////////////////////////////////////////////////////////////////
  // Packet queue private functions
  /////////////////////////////////////////////////////////////////////////////

  auto get_packet_queue(VvcMapInternal& vvc_map, VvcInstanceKey vvc, QueueId qid) -> PacketQueue&
  {
    if (auto it = vvc_map.find(vvc); it != vvc_map.end()) {
      return it->second.packet_queues[qid];
    } else {
      throw std::runtime_error("VVC " + to_string(vvc) + " does not exist.");
    }
  }

  bool packet_queue_empty(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc)
  {
    return get_packet_queue(vvc_map, vvc, qid).empty();
  }

  size_t packet_queue_size(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc)
  {
    return get_packet_queue(vvc_map, vvc, qid).size();
  }

  auto packet_queue_get_byte(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc) -> std::optional<uint8_t>
  {
    return get_packet_queue(vvc_map, vvc, qid).get_byte();
  }

  auto packet_queue_get_pkt(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc) -> std::vector<uint8_t>
  {
    return get_packet_queue(vvc_map, vvc, qid).get_pkt();
  }

  void packet_queue_put_byte(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc, uint8_t byte, bool eop)
  {
    get_packet_queue(vvc_map, vvc, qid).put_byte(byte, eop);
  }

  void packet_queue_put_pkt(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc, const std::vector<uint8_t>& pkt)
  {
    get_packet_queue(vvc_map, vvc, qid).put_pkt(pkt);
  }

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

  void AddVvc(VvcInstanceKey vvc, std::map<std::string, int> bfm_cfg) {
    vvcInstanceMap([&](auto &vvc_map) {
      if (vvc_map.find(vvc) == vvc_map.end()) {
        vvc_map.emplace(vvc, VvcInstanceData{.cfg{.bfm_cfg = bfm_cfg}});
      } else {
        throw std::runtime_error("VVC " + to_string(vvc) + " exists already.");
      }
    });
  }

  std::vector<VvcInstance> GetVvcList() const
  {
    std::vector<VvcInstance> vec;

    vvcInstanceMap([&](auto &vvc_map) {
      for (auto vvc : vvc_map) {
        vec.push_back(VvcInstance(vvc.first, vvc.second.cfg));
      }
    });

    return vec;
  }

  void SetVvcListenEnable(VvcInstanceKey vvc, bool enable) {
    vvcInstanceMap([&](auto &vvc_map) {
      if (auto it = vvc_map.find(vvc); it != vvc_map.end()) {
        it->second.cfg.listen_enable = enable;
      } else {
        throw std::runtime_error("VVC " + to_string(vvc) + " does not exist.");
      }
    });
  }

  bool GetVvcListenEnable(VvcInstanceKey vvc) const {
    bool listen = vvcInstanceMap([&](auto &vvc_map) {
      if (auto it = vvc_map.find(vvc); it != vvc_map.end()) {
        return it->second.cfg.listen_enable;
      } else {
        throw std::runtime_error("VVC " + to_string(vvc) + " does not exist.");
      }
    });
    return listen;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Byte queue public functions
  /////////////////////////////////////////////////////////////////////////////

  bool byte_queue_empty(QueueId qid, VvcInstanceKey vvc)
  {
    return vvcInstanceMap([&](auto &vvc_map) {return byte_queue_empty(vvc_map, qid, vvc);});
  }

  size_t byte_queue_size(QueueId qid, VvcInstanceKey vvc)
  {
    return vvcInstanceMap([&](auto &vvc_map) {return byte_queue_size(vvc_map, qid, vvc);});
  }

  void byte_queue_put(QueueId qid, VvcInstanceKey vvc, uint8_t byte)
  {
    vvcInstanceMap([&](auto &vvc_map) {byte_queue_put(vvc_map, qid, vvc, byte);});
  }

  void byte_queue_put(QueueId qid, VvcInstanceKey vvc, const std::vector<uint8_t>& data)
  {
    vvcInstanceMap([&](auto &vvc_map) {byte_queue_put(vvc_map, qid, vvc, data);});
  }

  auto byte_queue_get(QueueId qid, VvcInstanceKey vvc) -> std::optional<uint8_t>
  {
    return vvcInstanceMap([&](auto &vvc_map) {return byte_queue_get(vvc_map, qid, vvc);});
  }

  auto byte_queue_get(QueueId qid, VvcInstanceKey vvc, int num_bytes) -> std::vector<uint8_t>
  {
    return vvcInstanceMap([&](auto &vvc_map) {return byte_queue_get(vvc_map, qid, vvc, num_bytes);});
  }


  /////////////////////////////////////////////////////////////////////////////
  // Packet queue public functions
  /////////////////////////////////////////////////////////////////////////////
  bool packet_queue_empty(QueueId qid, VvcInstanceKey vvc)
  {
    return vvcInstanceMap([&](auto &vvc_map) {return packet_queue_empty(vvc_map, qid, vvc);});
  }

  size_t packet_queue_size(QueueId qid, VvcInstanceKey vvc)
  {
    return vvcInstanceMap([&](auto &vvc_map) {return packet_queue_size(vvc_map, qid, vvc);});
  }

  auto packet_queue_get_byte(QueueId qid, VvcInstanceKey vvc) -> std::optional<uint8_t>
  {
    return vvcInstanceMap([&](auto &vvc_map) {return packet_queue_get_byte(vvc_map, qid, vvc);});
  }

  auto packet_queue_get_pkt(QueueId qid, VvcInstanceKey vvc) -> std::vector<uint8_t>
  {
    return vvcInstanceMap([&](auto &vvc_map) {return packet_queue_get_pkt(vvc_map, qid, vvc);});
  }

  void packet_queue_put_byte(QueueId qid, VvcInstanceKey vvc, uint8_t byte, bool eop)
  {
    vvcInstanceMap([&](auto &vvc_map) {packet_queue_put_byte(vvc_map, qid, vvc, byte, eop);});
  }

  void packet_queue_put_pkt(QueueId qid, VvcInstanceKey vvc, const std::vector<uint8_t>& pkt)
  {
    vvcInstanceMap([&](auto &vvc_map) {packet_queue_put_pkt(vvc_map, qid, vvc, pkt);});
  }
};

} // namespace uvvm_cosim
