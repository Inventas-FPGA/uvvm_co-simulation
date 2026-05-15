#pragma once
#include <cstdint>
#include <deque>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>
#include "vvc_base.hpp"

namespace uvvm_cosim {

struct Transfer {
  uint64_t             addr          = 0;
  std::vector<uint8_t> data;         // empty = read request; filled = write request or read response
  bool                 addr_increment = true; // true = advance addr per word, false = same addr (FIFO)
  uint32_t             num_bytes     = 0;    // used for read requests to specify how many bytes to read
};

// Concrete VVC class for memory-mapped protocols (AXI-Lite, AXI, Avalon-MM).
// Maintains a request queue (VHDL polls and executes) and a response queue (VHDL writes results).
class VvcMemMapped : public VvcBase {
  std::deque<Transfer> request_queue;
  std::deque<Transfer> response_queue;

  std::deque<Transfer>& getQueue(QueueId id) {
    if (id == QID_REQUEST)  return request_queue;
    if (id == QID_RESPONSE) return response_queue;
    throw std::out_of_range("VvcMemMapped: invalid QueueId " + std::to_string(id));
  }

  const std::deque<Transfer>& getQueue(QueueId id) const {
    if (id == QID_REQUEST)  return request_queue;
    if (id == QID_RESPONSE) return response_queue;
    throw std::out_of_range("VvcMemMapped: invalid QueueId " + std::to_string(id));
  }

public:
  VvcMemMapped(VvcTypes vvc_type, VvcChannel channel, int instance_id)
    : VvcBase(vvc_type, channel, instance_id)
  {
    static const std::set<VvcTypes> supported{
      VvcTypes::AXI, VvcTypes::AXILITE, VvcTypes::AVALON_MM};
    if (!supported.count(vvc_type)) {
      throw std::invalid_argument("VvcMemMapped does not support VvcTypes::" +
                                  to_string(vvc_type));
    }
  }

  void QueuePut(QueueId id, const Transfer& trans) {
    getQueue(id).push_back(trans);
  }

  std::optional<Transfer> QueueGet(QueueId id) {
    auto& q = getQueue(id);
    if (q.empty()) return std::nullopt;
    Transfer t = q.front();
    q.pop_front();
    return t;
  }

  bool QueueEmpty(QueueId id) const {
    return getQueue(id).empty();
  }
};

} // namespace uvvm_cosim
