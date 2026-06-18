#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <vector>
#include "vvc/vvc_packet_stream.hpp"

using namespace uvvm_cosim;

TEST_CASE("VvcPacketStreamBased_construction_supported_types")
{
  VvcPacketStreamBased axis{VvcTypes::AXI_STREAM, VvcChannel::NA, 0};
  VvcPacketStreamBased avst{VvcTypes::AVALON_ST,  VvcChannel::TX, 1};

  REQUIRE(axis.packet_based == true);
  REQUIRE(avst.packet_based == true);

  REQUIRE(axis.rx_channel_en == true);
  REQUIRE(axis.tx_channel_en == true);
  REQUIRE(avst.tx_channel_en == true);
  REQUIRE(avst.rx_channel_en == false);
}

TEST_CASE("VvcPacketStreamBased_UART_unsupported_throws")
{
  // UART is allowed in VvcStreamBased but not in VvcPacketStreamBased.
  REQUIRE_THROWS_AS(VvcPacketStreamBased(VvcTypes::UART, VvcChannel::NA, 0),
                    std::invalid_argument);
  // AXILite and other MM-based not allowed in VvcStreamBased or VvcPacketStreamBased. 
  REQUIRE_THROWS_AS(VvcPacketStreamBased(VvcTypes::AXILITE, VvcChannel::NA, 0),
                    std::invalid_argument);
}

TEST_CASE("VvcPacketStreamBased_empty_initially")
{
  VvcPacketStreamBased v{VvcTypes::AXI_STREAM, VvcChannel::NA, 0};

  REQUIRE(v.QueueEmpty(QID_TRANSMIT));
  REQUIRE(v.QueueEmpty(QID_RECEIVE));
  REQUIRE(v.QueueSize(QID_TRANSMIT) == 0);

  REQUIRE_FALSE(v.QueueGetByte(QID_TRANSMIT).has_value());
  REQUIRE(v.QueueGetPacket(QID_TRANSMIT, 0).empty());
}

TEST_CASE("VvcPacketStreamBased_packet_round_trip")
{
  VvcPacketStreamBased v{VvcTypes::AXI_STREAM, VvcChannel::NA, 0};

  std::vector<uint8_t> pkt_a{0xAA, 0xBB, 0xCC};
  std::vector<uint8_t> pkt_b{0x01, 0x02, 0x03, 0x04};

  v.QueuePutPacket(QID_TRANSMIT, pkt_a);
  v.QueuePutPacket(QID_TRANSMIT, pkt_b);

  REQUIRE(v.QueueSize(QID_TRANSMIT) == 2);   // size counts packets

  REQUIRE(v.QueueGetPacket(QID_TRANSMIT, 0) == pkt_a);
  REQUIRE(v.QueueGetPacket(QID_TRANSMIT, 0) == pkt_b);
  REQUIRE(v.QueueEmpty(QID_TRANSMIT));
}

TEST_CASE("VvcPacketStreamBased_put_byte_eop_delimits_packets")
{
  VvcPacketStreamBased v{VvcTypes::AXI_STREAM, VvcChannel::NA, 0};

  // Build two packets one byte at a time, eop=true marks the last byte.
  v.QueuePutByte(QID_RECEIVE, 0x10, false);
  v.QueuePutByte(QID_RECEIVE, 0x20, false);
  v.QueuePutByte(QID_RECEIVE, 0x30, true);   // closes packet 1

  v.QueuePutByte(QID_RECEIVE, 0x40, true);   // closes packet 2 (single byte)

  // A trailing partial packet (no eop yet) should NOT be enqueued
  v.QueuePutByte(QID_RECEIVE, 0x99, false);

  REQUIRE(v.QueueSize(QID_RECEIVE) == 2);

  auto p1 = v.QueueGetPacket(QID_RECEIVE, 0);
  REQUIRE(p1 == std::vector<uint8_t>{0x10, 0x20, 0x30});

  auto p2 = v.QueueGetPacket(QID_RECEIVE, 0);
  REQUIRE(p2 == std::vector<uint8_t>{0x40});

  REQUIRE(v.QueueEmpty(QID_RECEIVE));

  // Close out the partial packet now and read it back
  v.QueuePutByte(QID_RECEIVE, 0xEE, true);
  REQUIRE(v.QueueSize(QID_RECEIVE) == 1);
  REQUIRE(v.QueueGetPacket(QID_RECEIVE, 0) == std::vector<uint8_t>{0x99, 0xEE});
}

TEST_CASE("VvcPacketStreamBased_get_byte_returns_eop_on_packet_end")
{
  VvcPacketStreamBased v{VvcTypes::AXI_STREAM, VvcChannel::NA, 0};

  v.QueuePutPacket(QID_TRANSMIT, std::vector<uint8_t>{0x11, 0x22});
  v.QueuePutPacket(QID_TRANSMIT, std::vector<uint8_t>{0x33});

  // packet 1: byte 1
  auto b = v.QueueGetByte(QID_TRANSMIT);
  REQUIRE(b.has_value());
  REQUIRE(b->first  == 0x11);
  REQUIRE(b->second == false);   // not last byte of packet

  // packet 1: byte 2 (eop)
  b = v.QueueGetByte(QID_TRANSMIT);
  REQUIRE(b.has_value());
  REQUIRE(b->first  == 0x22);
  REQUIRE(b->second == true);

  // packet 2: byte 1 (single byte, eop immediately)
  b = v.QueueGetByte(QID_TRANSMIT);
  REQUIRE(b.has_value());
  REQUIRE(b->first  == 0x33);
  REQUIRE(b->second == true);

  REQUIRE_FALSE(v.QueueGetByte(QID_TRANSMIT).has_value());
}

TEST_CASE("VvcPacketStreamBased_mixed_packet_and_byte_reads")
{
  VvcPacketStreamBased v{VvcTypes::AXI_STREAM, VvcChannel::NA, 0};

  v.QueuePutPacket(QID_RECEIVE, std::vector<uint8_t>{0x01, 0x02});
  v.QueuePutPacket(QID_RECEIVE, std::vector<uint8_t>{0x03, 0x04, 0x05});

  // First packet read as a whole
  REQUIRE(v.QueueGetPacket(QID_RECEIVE, 0) == std::vector<uint8_t>{0x01, 0x02});

  // Second packet read byte-by-byte
  auto b = v.QueueGetByte(QID_RECEIVE);
  REQUIRE(b.has_value());
  REQUIRE(b->first  == 0x03);
  REQUIRE(b->second == false);

  b = v.QueueGetByte(QID_RECEIVE);
  REQUIRE(b->first  == 0x04);
  REQUIRE(b->second == false);

  b = v.QueueGetByte(QID_RECEIVE);
  REQUIRE(b->first  == 0x05);
  REQUIRE(b->second == true);

  REQUIRE(v.QueueEmpty(QID_RECEIVE));
}

TEST_CASE("VvcPacketStreamBased_put_empty_packet_is_ignored")
{
  VvcPacketStreamBased v{VvcTypes::AXI_STREAM, VvcChannel::NA, 0};

  v.QueuePutPacket(QID_TRANSMIT, std::vector<uint8_t>{});
  REQUIRE(v.QueueEmpty(QID_TRANSMIT));
}

TEST_CASE("VvcPacketStreamBased_invalid_QueueId_throws")
{
  VvcPacketStreamBased v{VvcTypes::AXI_STREAM, VvcChannel::NA, 0};

  REQUIRE_THROWS_AS(v.QueuePutByte(QID_REQUEST, 0, false), std::out_of_range);
  REQUIRE_THROWS_AS(v.QueueGetByte(QID_RESPONSE),          std::out_of_range);
  REQUIRE_THROWS_AS(v.QueueEmpty(QID_REQUEST),             std::out_of_range);
}
