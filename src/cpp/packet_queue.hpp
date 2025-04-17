#pragma once
#include <cstdint>
#include <deque>
#include <optional>
#include <stdexcept>
#include <vector>

namespace uvvm_cosim {

class PacketQueue {

  // Buffer used with put_byte until whole packet received
  std::vector<uint8_t> pkt_buff;
  std::deque<std::deque<uint8_t>> q;

public:

  bool empty(void) const {
    return q.empty();
  }

  size_t size(void) const {
    return q.size();
  }

  std::optional<uint8_t> get_byte(void)
  {
    while (!q.empty()) {

      if (q.front().empty()) {
	throw std::runtime_error("PacketQueue::get_byte(): Empty packet in queue");
      }

      // Get and pop first byte from non-empty packet
      uint8_t byte = q.front().front();
      q.front().pop_front();

      // Pop packet from queue if this was the last byte in packet
      if (q.front().empty()) {
        q.pop_front();
      }

      return byte;
    }

    return {};
  }

  std::vector<uint8_t> get_pkt(void)
  {
    if (q.empty()) {
      // Empty vector if there's no packet available
      return std::vector<uint8_t>();
    }

    std::vector<uint8_t> pkt(q.front().begin(), q.front().end());

    // Pop packet
    q.pop_front();

    return pkt;
  }

  void put_byte(uint8_t byte, bool eop)
  {
    pkt_buff.push_back(byte);

    if (eop) {
      q.emplace_back(std::deque<uint8_t>(pkt_buff.begin(), pkt_buff.end()));
      pkt_buff.clear();
    }
  }

  void put_pkt(const std::vector<uint8_t>& pkt)
  {
    if (!pkt.empty()) {
      q.emplace_back(std::deque<uint8_t>(pkt.begin(), pkt.end()));
    }
  }
  
};


} // namespace uvvm_cosim
