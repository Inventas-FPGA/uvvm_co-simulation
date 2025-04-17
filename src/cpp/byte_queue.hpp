#pragma once
#include <cstdint>
#include <deque>
#include <optional>
#include <vector>

namespace uvvm_cosim {

class ByteQueue {

  std::deque<uint8_t> q;

public:

  bool empty(void) const {
    return q.empty();
  }

  size_t size(void) const {
    return q.size();
  }

  void put(uint8_t byte)
  {
    q.emplace_back(byte);
  }

  void put(const std::vector<uint8_t>& data) {
    q.insert(q.end(), data.begin(), data.end());
  }

  std::optional<uint8_t> get(void) {
    if (q.empty()) {
      return {};
    } else {
      uint8_t byte = q.front();
      q.pop_front();
      return byte;
    }
  }

  std::vector<uint8_t> get(size_t N) {
    if (q.size() < N || N == 0) {
      std::vector<uint8_t> data(q.begin(), q.end());
      q.clear();
      return data;
    } else {
      std::vector<uint8_t> data(q.begin(), q.begin()+N);
      q.erase(q.begin(), q.begin()+N);
      return data;
    }
  }

};

} // namespace uvvm_cosim
