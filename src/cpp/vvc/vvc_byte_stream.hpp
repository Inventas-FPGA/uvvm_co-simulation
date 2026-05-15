#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>
#include "vvc_stream_based.hpp"
#include "../byte_queue.hpp"

namespace uvvm_cosim {

// Concrete stream-based VVC for byte-oriented protocols (UART, non-packet AXI-Stream, Avalon-ST).
// Owns a pair of ByteQueues indexed by QID_TRANSMIT (0) and QID_RECEIVE (1).
class VvcByteStream : public VvcStreamBased {
  std::array<ByteQueue, 2> byteQueues;

  static int queueIndex(QueueId id) {
    if (id == QID_TRANSMIT) return 0;
    if (id == QID_RECEIVE)  return 1;
    throw std::out_of_range("VvcByteStream: invalid QueueId " + std::to_string(id));
  }

public:
  VvcByteStream(VvcTypes vvc_type, VvcChannel channel, int instance_id)
    : VvcStreamBased(vvc_type, channel, instance_id, /*packet_based=*/false)
  {}

  void QueuePutByte(QueueId id, uint8_t byte, bool /*eop*/) override {
    byteQueues[queueIndex(id)].put(byte);
  }

  void QueuePutPacket(QueueId id, const std::vector<uint8_t>& pkt) override {
    byteQueues[queueIndex(id)].put(pkt);
  }

  auto QueueGetByte(QueueId id) -> std::optional<std::pair<uint8_t, bool>> override {
    auto byte = byteQueues[queueIndex(id)].get();
    if (byte) return std::make_pair(byte.value(), false);
    return std::nullopt;
  }

  auto QueueGetPacket(QueueId id, int num_bytes) -> std::vector<uint8_t> override {
    return byteQueues[queueIndex(id)].get(num_bytes);
  }

  bool QueueEmpty(QueueId id) const override {
    return byteQueues[queueIndex(id)].empty();
  }

  size_t QueueSize(QueueId id) const override {
    return byteQueues[queueIndex(id)].size();
  }
};

} // namespace uvvm_cosim
