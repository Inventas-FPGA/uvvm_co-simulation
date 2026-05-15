#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>
#include "vvc_stream_based.hpp"
#include "../packet_queue.hpp"

namespace uvvm_cosim {

// Concrete stream-based VVC for packet-oriented protocols (packet-based AXI-Stream, Avalon-ST).
// Owns a pair of PacketQueues indexed by QID_TRANSMIT (0) and QID_RECEIVE (1).
class VvcPacketStreamBased : public VvcStreamBased {
  std::array<PacketQueue, 2> packetQueues = {};

  static int queueIndex(QueueId id) {
    if (id == QID_TRANSMIT) return 0;
    if (id == QID_RECEIVE)  return 1;
    throw std::out_of_range("VvcPacketStreamBased: invalid QueueId " + std::to_string(id));
  }

public:
  VvcPacketStreamBased(VvcTypes vvc_type, VvcChannel channel, int instance_id)
    : VvcStreamBased(vvc_type, channel, instance_id, /*packet_based=*/true)
  {
    static const std::set<VvcTypes> supported{VvcTypes::AXI_STREAM, VvcTypes::AVALON_ST};
    if (!supported.count(vvc_type)) {
      throw std::invalid_argument("VvcPacketStreamBased does not support VvcTypes::" +
                                  to_string(vvc_type));
    }
  }

  void QueuePutByte(QueueId id, uint8_t byte, bool eop) override {
    packetQueues[queueIndex(id)].put_byte(byte, eop);
  }

  void QueuePutPacket(QueueId id, const std::vector<uint8_t>& pkt) override {
    packetQueues[queueIndex(id)].put_pkt(pkt);
  }

  auto QueueGetByte(QueueId id) -> std::optional<std::pair<uint8_t, bool>> override {
    return packetQueues[queueIndex(id)].get_byte();
  }

  // Returns the next complete packet; num_bytes is ignored for packet-based VVCs.
  auto QueueGetPacket(QueueId id, int /*num_bytes*/) -> std::vector<uint8_t> override {
    return packetQueues[queueIndex(id)].get_pkt();
  }

  bool QueueEmpty(QueueId id) const override {
    return packetQueues[queueIndex(id)].empty();
  }

  size_t QueueSize(QueueId id) const override {
    return packetQueues[queueIndex(id)].size();
  }
};

} // namespace uvvm_cosim
