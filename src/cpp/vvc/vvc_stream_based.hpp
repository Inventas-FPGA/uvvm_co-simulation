#pragma once
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>
#include "vvc_base.hpp"

namespace uvvm_cosim {

// Abstract intermediate class for all stream-based VVCs (UART, AXI-Stream, Avalon-ST).
// Declares the pure-virtual queue interface; concrete storage lives in derived classes.
class VvcStreamBased : public VvcBase {
public:
  bool packet_based  = false;
  bool rx_channel_en = false;
  bool tx_channel_en = false;

  VvcStreamBased(VvcTypes vvc_type, VvcChannel channel, int instance_id, bool is_packet_based)
    : VvcBase(vvc_type, channel, instance_id)
    , packet_based(is_packet_based)
    , rx_channel_en(channel == VvcChannel::RX || channel == VvcChannel::NA)
    , tx_channel_en(channel == VvcChannel::TX || channel == VvcChannel::NA)
  {
    static const std::set<VvcTypes> supported{
      VvcTypes::AXI_STREAM, VvcTypes::AVALON_ST, VvcTypes::UART};
    if (!supported.count(vvc_type)) {
      throw std::invalid_argument("VvcStreamBased does not support VvcTypes::" +
                                  to_string(vvc_type));
    }
  }

  // ------------------------------------------------------------------
  // Pure-virtual queue interface.
  // Implementations use QID_TRANSMIT / QID_RECEIVE only.
  // eop is always false for byte-stream VVCs.
  // ------------------------------------------------------------------

  // Put a single byte (with optional end-of-packet marker for packet VVCs).
  virtual void QueuePutByte(QueueId id, uint8_t byte, bool eop) = 0;

  // Put a complete packet (for byte-stream VVCs the bytes are appended without a boundary).
  virtual void QueuePutPacket(QueueId id, const std::vector<uint8_t>& pkt) = 0;

  // Get next byte. Returns {byte, eop}. eop is always false for byte-stream VVCs.
  virtual auto QueueGetByte(QueueId id) -> std::optional<std::pair<uint8_t, bool>> = 0;

  // For byte-stream VVCs: returns up to num_bytes bytes.
  // For packet-stream VVCs: returns the next complete packet (num_bytes is ignored).
  virtual auto QueueGetPacket(QueueId id, int num_bytes) -> std::vector<uint8_t> = 0;

  virtual bool   QueueEmpty(QueueId id) const = 0;
  virtual size_t QueueSize(QueueId id)  const = 0;
};

} // namespace uvvm_cosim
