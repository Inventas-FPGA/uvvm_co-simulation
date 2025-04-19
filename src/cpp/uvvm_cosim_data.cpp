#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include "uvvm_cosim_data.hpp"

namespace uvvm_cosim {

  /////////////////////////////////////////////////////////////////////////////
  // Byte queue private functions
  /////////////////////////////////////////////////////////////////////////////

  auto UvvmCosimData::get_byte_queue(VvcMapInternal& vvc_map, VvcInstanceKey vvc, QueueId qid) -> ByteQueue&
  {
    if (auto it = vvc_map.find(vvc); it != vvc_map.end()) {
      if (it->second.cfg.packet_based) {
	throw std::runtime_error("Tried to access byte queue for packet-based VVC " + to_string(vvc) + ".");
      }
      return it->second.byte_queues[qid];
    } else {
      throw std::runtime_error("VVC " + to_string(vvc) + " does not exist.");
    }
  }

  bool UvvmCosimData::byte_queue_empty(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc)
  {
    return get_byte_queue(vvc_map, vvc, qid).empty();
  }

  size_t UvvmCosimData::byte_queue_size(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc)
  {
    return get_byte_queue(vvc_map, vvc, qid).size();
  }

  void UvvmCosimData::byte_queue_put(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc, uint8_t byte)
  {
    get_byte_queue(vvc_map, vvc, qid).put(byte);
  }

  void UvvmCosimData::byte_queue_put(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc,
                      const std::vector<uint8_t>& data)
  {
    get_byte_queue(vvc_map, vvc, qid).put(data);
  }

  auto UvvmCosimData::byte_queue_get(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc) -> std::optional<uint8_t>
  {
    return get_byte_queue(vvc_map, vvc, qid).get();
  }

  auto UvvmCosimData::byte_queue_get(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc, int num_bytes) -> std::vector<uint8_t>
  {
    return get_byte_queue(vvc_map, vvc, qid).get(num_bytes);
  }


  /////////////////////////////////////////////////////////////////////////////
  // Packet queue private functions
  /////////////////////////////////////////////////////////////////////////////

  auto UvvmCosimData::get_packet_queue(VvcMapInternal& vvc_map, VvcInstanceKey vvc, QueueId qid) -> PacketQueue&
  {
    if (auto it = vvc_map.find(vvc); it != vvc_map.end()) {
      if (!it->second.cfg.packet_based) {
	throw std::runtime_error("Tried to access packet queue for non-packet based VVC " + to_string(vvc) + ".");
      }
      return it->second.packet_queues[qid];
    } else {
      throw std::runtime_error("VVC " + to_string(vvc) + " does not exist.");
    }
  }

  bool UvvmCosimData::packet_queue_empty(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc)
  {
    return get_packet_queue(vvc_map, vvc, qid).empty();
  }

  size_t UvvmCosimData::packet_queue_size(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc)
  {
    return get_packet_queue(vvc_map, vvc, qid).size();
  }

  auto UvvmCosimData::packet_queue_get_byte(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc) -> std::optional<std::pair<uint8_t, bool>>
  {
    return get_packet_queue(vvc_map, vvc, qid).get_byte();
  }

  auto UvvmCosimData::packet_queue_get_pkt(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc) -> std::vector<uint8_t>
  {
    return get_packet_queue(vvc_map, vvc, qid).get_pkt();
  }

  void UvvmCosimData::packet_queue_put_byte(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc, uint8_t byte, bool eop)
  {
    get_packet_queue(vvc_map, vvc, qid).put_byte(byte, eop);
  }

  void UvvmCosimData::packet_queue_put_pkt(VvcMapInternal& vvc_map, QueueId qid, VvcInstanceKey vvc, const std::vector<uint8_t>& pkt)
  {
    get_packet_queue(vvc_map, vvc, qid).put_pkt(pkt);
  }

  /////////////////////////////////////////////////////////////////////////////
  // VVC list public functions
  /////////////////////////////////////////////////////////////////////////////

  void UvvmCosimData::AddVvc(VvcInstanceKey vvc, std::map<std::string, int> bfm_cfg) {
    VvcConfig cfg;

    if (auto it = bfm_cfg.find("cosim_support"); it != bfm_cfg.end()) {
      cfg.cosim_support = it->second == 0 ? false : true;
      bfm_cfg.erase(it);
    }

    if (auto it = bfm_cfg.find("packet_based"); it != bfm_cfg.end()) {
      cfg.packet_based = it->second == 0 ? false : true;
      bfm_cfg.erase(it);
    }

    cfg.bfm_cfg = bfm_cfg;

    vvcInstanceMap([&](auto &vvc_map) {
      if (vvc_map.find(vvc) == vvc_map.end()) {
        vvc_map.emplace(vvc, VvcInstanceData{.cfg = cfg});
      } else {
        throw std::runtime_error("VVC " + to_string(vvc) + " exists already.");
      }
    });
  }

  auto UvvmCosimData::GetVvcList() const -> std::vector<VvcInstance>
  {
    std::vector<VvcInstance> vec;

    vvcInstanceMap([&](auto &vvc_map) {
      for (auto vvc : vvc_map) {
        vec.push_back(VvcInstance(vvc.first, vvc.second.cfg));
      }
    });

    return vec;
  }

  void UvvmCosimData::SetVvcListenEnable(VvcInstanceKey vvc, bool enable) {
    vvcInstanceMap([&](auto &vvc_map) {
      if (auto it = vvc_map.find(vvc); it != vvc_map.end()) {
        it->second.cfg.listen_enable = enable;
      } else {
        throw std::runtime_error("VVC " + to_string(vvc) + " does not exist.");
      }
    });
  }

  bool UvvmCosimData::GetVvcListenEnable(VvcInstanceKey vvc) const {
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

  bool UvvmCosimData::byte_queue_empty(QueueId qid, VvcInstanceKey vvc)
  {
    return vvcInstanceMap([&](auto &vvc_map) {return byte_queue_empty(vvc_map, qid, vvc);});
  }

  size_t UvvmCosimData::byte_queue_size(QueueId qid, VvcInstanceKey vvc)
  {
    return vvcInstanceMap([&](auto &vvc_map) {return byte_queue_size(vvc_map, qid, vvc);});
  }

  void UvvmCosimData::byte_queue_put(QueueId qid, VvcInstanceKey vvc, uint8_t byte)
  {
    vvcInstanceMap([&](auto &vvc_map) {byte_queue_put(vvc_map, qid, vvc, byte);});
  }

  void UvvmCosimData::byte_queue_put(QueueId qid, VvcInstanceKey vvc, const std::vector<uint8_t>& data)
  {
    vvcInstanceMap([&](auto &vvc_map) {byte_queue_put(vvc_map, qid, vvc, data);});
  }

  auto UvvmCosimData::byte_queue_get(QueueId qid, VvcInstanceKey vvc) -> std::optional<uint8_t>
  {
    return vvcInstanceMap([&](auto &vvc_map) {return byte_queue_get(vvc_map, qid, vvc);});
  }

  auto UvvmCosimData::byte_queue_get(QueueId qid, VvcInstanceKey vvc, int num_bytes) -> std::vector<uint8_t>
  {
    return vvcInstanceMap([&](auto &vvc_map) {return byte_queue_get(vvc_map, qid, vvc, num_bytes);});
  }


  /////////////////////////////////////////////////////////////////////////////
  // Packet queue public functions
  /////////////////////////////////////////////////////////////////////////////
  bool UvvmCosimData::packet_queue_empty(QueueId qid, VvcInstanceKey vvc)
  {
    return vvcInstanceMap([&](auto &vvc_map) {return packet_queue_empty(vvc_map, qid, vvc);});
  }

  size_t UvvmCosimData::packet_queue_size(QueueId qid, VvcInstanceKey vvc)
  {
    return vvcInstanceMap([&](auto &vvc_map) {return packet_queue_size(vvc_map, qid, vvc);});
  }

  auto UvvmCosimData::packet_queue_get_byte(QueueId qid, VvcInstanceKey vvc) -> std::optional<std::pair<uint8_t, bool>>
  {
    return vvcInstanceMap([&](auto &vvc_map) {return packet_queue_get_byte(vvc_map, qid, vvc);});
  }

  auto UvvmCosimData::packet_queue_get_pkt(QueueId qid, VvcInstanceKey vvc) -> std::vector<uint8_t>
  {
    return vvcInstanceMap([&](auto &vvc_map) {return packet_queue_get_pkt(vvc_map, qid, vvc);});
  }

  void UvvmCosimData::packet_queue_put_byte(QueueId qid, VvcInstanceKey vvc, uint8_t byte, bool eop)
  {
    vvcInstanceMap([&](auto &vvc_map) {packet_queue_put_byte(vvc_map, qid, vvc, byte, eop);});
  }

  void UvvmCosimData::packet_queue_put_pkt(QueueId qid, VvcInstanceKey vvc, const std::vector<uint8_t>& pkt)
  {
    vvcInstanceMap([&](auto &vvc_map) {packet_queue_put_pkt(vvc_map, qid, vvc, pkt);});
  }

} // namespace uvvm_cosim
