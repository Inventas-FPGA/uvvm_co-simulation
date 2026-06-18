#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <vector>
#include "vvc/vvc_byte_stream.hpp"

using namespace uvvm_cosim;

TEST_CASE("VvcByteStream_construction_supported_types")
{
  // UART/AXI_STREAM/AVALON_ST are the only types VvcStreamBased accepts.
  VvcByteStream uart{VvcTypes::UART,       VvcChannel::TX, 0};
  VvcByteStream axis{VvcTypes::AXI_STREAM, VvcChannel::NA, 1};
  VvcByteStream avst{VvcTypes::AVALON_ST,  VvcChannel::RX, 2};

  for (auto* v : {&uart, &axis, &avst}) {
    REQUIRE(v->packet_based == false);
  }

  REQUIRE(uart.tx_channel_en == true);
  REQUIRE(uart.rx_channel_en == false);

  REQUIRE(axis.tx_channel_en == true);
  REQUIRE(axis.rx_channel_en == true);

  REQUIRE(avst.rx_channel_en == true);
  REQUIRE(avst.tx_channel_en == false);
}

TEST_CASE("VvcByteStream_construction_unsupported_type_throws")
{
  REQUIRE_THROWS_AS(VvcByteStream(VvcTypes::AXILITE,   VvcChannel::NA, 0),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(VvcByteStream(VvcTypes::AXI,       VvcChannel::NA, 0),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(VvcByteStream(VvcTypes::AVALON_MM, VvcChannel::NA, 0),
                    std::invalid_argument);
}

TEST_CASE("VvcByteStream_empty_initially")
{
  VvcByteStream v{VvcTypes::UART, VvcChannel::NA, 0};

  REQUIRE(v.QueueEmpty(QID_TRANSMIT));
  REQUIRE(v.QueueEmpty(QID_RECEIVE));
  REQUIRE(v.QueueSize(QID_TRANSMIT) == 0);
  REQUIRE(v.QueueSize(QID_RECEIVE)  == 0);

  REQUIRE_FALSE(v.QueueGetByte(QID_TRANSMIT).has_value());
}

TEST_CASE("VvcByteStream_put_get_byte_eop_ignored")
{
  VvcByteStream v{VvcTypes::UART, VvcChannel::NA, 0};

  // The eop arg is documented as ignored for byte streams.
  v.QueuePutByte(QID_TRANSMIT, 0xAB, /*eop=*/true);
  v.QueuePutByte(QID_TRANSMIT, 0xCD, /*eop=*/false);

  REQUIRE_FALSE(v.QueueEmpty(QID_TRANSMIT));
  REQUIRE(v.QueueSize(QID_TRANSMIT) == 2);

  auto b1 = v.QueueGetByte(QID_TRANSMIT);
  REQUIRE(b1.has_value());
  REQUIRE(b1->first  == 0xAB);
  REQUIRE(b1->second == false);  // get always returns eop=false for byte streams

  auto b2 = v.QueueGetByte(QID_TRANSMIT);
  REQUIRE(b2.has_value());
  REQUIRE(b2->first  == 0xCD);
  REQUIRE(b2->second == false);

  REQUIRE(v.QueueEmpty(QID_TRANSMIT));
}

TEST_CASE("VvcByteStream_put_packet_and_partial_get")
{
  VvcByteStream v{VvcTypes::AXI_STREAM, VvcChannel::NA, 0};

  std::vector<uint8_t> data{0x10, 0x20, 0x30, 0x40, 0x50};
  v.QueuePutPacket(QID_RECEIVE, data);

  REQUIRE(v.QueueSize(QID_RECEIVE) == data.size());

  // Partial read: first 2 bytes
  auto first = v.QueueGetPacket(QID_RECEIVE, 2);
  REQUIRE(first.size() == 2);
  REQUIRE(first == std::vector<uint8_t>{0x10, 0x20});
  REQUIRE(v.QueueSize(QID_RECEIVE) == 3);

  // Read remaining via num_bytes=0 ("all available")
  auto rest = v.QueueGetPacket(QID_RECEIVE, 0);
  REQUIRE(rest == std::vector<uint8_t>{0x30, 0x40, 0x50});
  REQUIRE(v.QueueEmpty(QID_RECEIVE));
}

TEST_CASE("VvcByteStream_tx_and_rx_queues_independent")
{
  VvcByteStream v{VvcTypes::AXI_STREAM, VvcChannel::NA, 0};

  v.QueuePutByte(QID_TRANSMIT, 0x11, false);
  v.QueuePutByte(QID_RECEIVE,  0x22, false);

  REQUIRE(v.QueueSize(QID_TRANSMIT) == 1);
  REQUIRE(v.QueueSize(QID_RECEIVE)  == 1);

  auto tx = v.QueueGetByte(QID_TRANSMIT);
  REQUIRE(tx.has_value());
  REQUIRE(tx->first == 0x11);

  REQUIRE(v.QueueEmpty(QID_TRANSMIT));
  REQUIRE_FALSE(v.QueueEmpty(QID_RECEIVE));

  auto rx = v.QueueGetByte(QID_RECEIVE);
  REQUIRE(rx.has_value());
  REQUIRE(rx->first == 0x22);
}

TEST_CASE("VvcByteStream_invalid_QueueId_throws")
{
  VvcByteStream v{VvcTypes::UART, VvcChannel::NA, 0};

  REQUIRE_THROWS_AS(v.QueuePutByte(QID_REQUEST, 0, false), std::out_of_range);
  REQUIRE_THROWS_AS(v.QueueGetByte(QID_RESPONSE),          std::out_of_range);
  REQUIRE_THROWS_AS(v.QueueEmpty(QID_REQUEST),             std::out_of_range);
  REQUIRE_THROWS_AS(v.QueueSize(QID_RESPONSE),             std::out_of_range);
}
