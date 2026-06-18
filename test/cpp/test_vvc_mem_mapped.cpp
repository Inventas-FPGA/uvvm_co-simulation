#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <vector>
#include "vvc/vvc_mem_mapped.hpp"

using namespace uvvm_cosim;

TEST_CASE("VvcMemMapped_construction_supported_types")
{
  VvcMemMapped axi    {VvcTypes::AXI,        VvcChannel::NA, 0};
  VvcMemMapped axilite{VvcTypes::AXILITE,    VvcChannel::NA, 1};
  VvcMemMapped avmm   {VvcTypes::AVALON_MM,  VvcChannel::NA, 2};

  REQUIRE(axi.vvc_type     == VvcTypes::AXI);
  REQUIRE(axilite.vvc_type == VvcTypes::AXILITE);
  REQUIRE(avmm.vvc_type    == VvcTypes::AVALON_MM);
}

TEST_CASE("VvcMemMapped_construction_unsupported_types_throw")
{
  REQUIRE_THROWS_AS(VvcMemMapped(VvcTypes::UART,       VvcChannel::NA, 0),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(VvcMemMapped(VvcTypes::AXI_STREAM, VvcChannel::NA, 0),
                    std::invalid_argument);
  REQUIRE_THROWS_AS(VvcMemMapped(VvcTypes::AVALON_ST,  VvcChannel::NA, 0),
                    std::invalid_argument);
}

TEST_CASE("VvcMemMapped_queues_empty_initially")
{
  VvcMemMapped v{VvcTypes::AXILITE, VvcChannel::NA, 0};

  REQUIRE(v.QueueEmpty(QID_REQUEST));
  REQUIRE(v.QueueEmpty(QID_RESPONSE));
  REQUIRE_FALSE(v.QueueGet(QID_REQUEST).has_value());
  REQUIRE_FALSE(v.QueueGet(QID_RESPONSE).has_value());
}

TEST_CASE("VvcMemMapped_request_queue_round_trip")
{
  VvcMemMapped v{VvcTypes::AXILITE, VvcChannel::NA, 0};

  // Write request: data filled, num_bytes set
  Transfer write_req;
  write_req.addr           = 0x1000;
  write_req.data           = {0xAA, 0xBB, 0xCC, 0xDD};
  write_req.addr_increment = true;
  write_req.num_bytes      = 4;

  // Read request: empty data signals it's a read
  Transfer read_req;
  read_req.addr           = 0x2000;
  read_req.addr_increment = false;
  read_req.num_bytes      = 8;

  v.QueuePut(QID_REQUEST, write_req);
  v.QueuePut(QID_REQUEST, read_req);

  REQUIRE_FALSE(v.QueueEmpty(QID_REQUEST));

  // FIFO order
  auto t1 = v.QueueGet(QID_REQUEST);
  REQUIRE(t1.has_value());
  REQUIRE(t1->addr           == 0x1000);
  REQUIRE(t1->data           == std::vector<uint8_t>{0xAA, 0xBB, 0xCC, 0xDD});
  REQUIRE(t1->addr_increment == true);
  REQUIRE(t1->num_bytes      == 4);

  auto t2 = v.QueueGet(QID_REQUEST);
  REQUIRE(t2.has_value());
  REQUIRE(t2->addr           == 0x2000);
  REQUIRE(t2->data.empty());
  REQUIRE(t2->addr_increment == false);
  REQUIRE(t2->num_bytes      == 8);

  REQUIRE(v.QueueEmpty(QID_REQUEST));
  REQUIRE_FALSE(v.QueueGet(QID_REQUEST).has_value());
}

TEST_CASE("VvcMemMapped_response_queue_independent_from_request")
{
  VvcMemMapped v{VvcTypes::AXILITE, VvcChannel::NA, 0};

  Transfer req;  req.addr = 0xA0; req.num_bytes = 4;
  Transfer resp; resp.addr = 0xA0; resp.data = {0xDE, 0xAD, 0xBE, 0xEF};

  v.QueuePut(QID_REQUEST,  req);
  v.QueuePut(QID_RESPONSE, resp);

  REQUIRE_FALSE(v.QueueEmpty(QID_REQUEST));
  REQUIRE_FALSE(v.QueueEmpty(QID_RESPONSE));

  // Popping one does not affect the other
  (void) v.QueueGet(QID_REQUEST);
  REQUIRE(v.QueueEmpty(QID_REQUEST));
  REQUIRE_FALSE(v.QueueEmpty(QID_RESPONSE));

  auto r = v.QueueGet(QID_RESPONSE);
  REQUIRE(r.has_value());
  REQUIRE(r->data == std::vector<uint8_t>{0xDE, 0xAD, 0xBE, 0xEF});
}

TEST_CASE("VvcMemMapped_invalid_QueueId_throws")
{
  VvcMemMapped v{VvcTypes::AXILITE, VvcChannel::NA, 0};

  Transfer t;
  REQUIRE_THROWS_AS(v.QueuePut(QID_TRANSMIT, t), std::out_of_range);
  REQUIRE_THROWS_AS(v.QueueGet(QID_RECEIVE),     std::out_of_range);
  REQUIRE_THROWS_AS(v.QueueEmpty(QID_TRANSMIT),  std::out_of_range);
}
