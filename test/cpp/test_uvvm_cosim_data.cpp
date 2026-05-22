#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <map>
#include <stdexcept>
#include <vector>
#include "uvvm_cosim_data.hpp"
#include "uvvm_cosim_types.hpp"

using namespace uvvm_cosim;

// VVC identifiers in the order VvcCompare should sort them.
// Sort key: (vvc_type enum value, vvc_channel enum value, instance_id).
// AXI_STREAM=0, UART=5; NA=0, RX=1, TX=2.
std::vector<VvcIdentifier> vk = {
  {VvcTypes::AXI_STREAM, VvcChannel::NA,  1},
  {VvcTypes::AXI_STREAM, VvcChannel::NA, 10},
  {VvcTypes::UART,       VvcChannel::RX,  3},
  {VvcTypes::UART,       VvcChannel::TX,  0},
  {VvcTypes::UART,       VvcChannel::TX,  3},
};

// bfm_cfg maps — none contain "cosim_support" or "packet_based",
// so all keys are stored verbatim in the VVC's bfm_cfg.
std::vector<std::map<std::string, int>> bfm_cfgs = {
  {{"cfg_val1", 0}, {"cfg_val2", 1}, {"cfg_val3", 10}},
  {{"airplane", 200}, {"helicopter", 123}},
  {{"spaghetti", 1}, {"pizza", 234}, {"lasagne", 0}, {"risotto", 89}},
  {{"cookie", 14}},
  {{"firetruck", 123}, {"boat", 23}, {"submarine", 10}},
};

// listen_enable flags corresponding to each VVC (set separately via SetVvcListenEnable)
std::vector<bool> listen_enable = {true, false, false, true, false};

// Helper: add vk[i] to cosim_data using the string-based AddVvc overload
static void addVvc(UvvmCosimData& d, int i)
{
  d.AddVvc(to_string(vk[i].vvc_type), to_string(vk[i].vvc_channel),
           vk[i].vvc_instance_id, bfm_cfgs[i]);
}

// Helper: find the VvcInfo matching vk[i] in a VvcInfo list
static VvcInfo findInfo(const std::vector<VvcInfo>& list, int i)
{
  auto it = std::find_if(list.begin(), list.end(), [&](const VvcInfo& v) {
    return v.vvc_type        == to_string(vk[i].vvc_type) &&
           v.vvc_channel     == to_string(vk[i].vvc_channel) &&
           v.vvc_instance_id == vk[i].vvc_instance_id;
  });
  REQUIRE(it != list.end());
  return *it;
}

TEST_CASE("UvvmCosimData_AddVvc_and_GetVvcList")
{
  INFO("Check adding and retrieving VVCs.");

  UvvmCosimData cosim_data;

  INFO("Check empty initially");
  REQUIRE(cosim_data.GetVvcList().empty());

  INFO("Add one VVC, retrieve VVC list and check that it matches");
  addVvc(cosim_data, 0);

  auto vvc_list = cosim_data.GetVvcList();
  REQUIRE(vvc_list.size() == 1);
  REQUIRE(vvc_list[0].vvc_type        == to_string(vk[0].vvc_type));
  REQUIRE(vvc_list[0].vvc_channel     == to_string(vk[0].vvc_channel));
  REQUIRE(vvc_list[0].vvc_instance_id == vk[0].vvc_instance_id);

  // AddVvc does not set listen_enable; it should be false initially.
  REQUIRE(vvc_list[0].listen_enable == false);

  REQUIRE(vvc_list[0].bfm_cfg.size() == bfm_cfgs[0].size());
  for (auto& [k, v] : bfm_cfgs[0]) {
    REQUIRE(vvc_list[0].bfm_cfg.contains(k));
    REQUIRE(vvc_list[0].bfm_cfg.at(k) == v);
  }

  INFO("Check that same VVC can't be added twice");
  REQUIRE_THROWS(addVvc(cosim_data, 0));

  INFO("Check that size remained the same after throw");
  REQUIRE(cosim_data.GetVvcList().size() == 1);

  INFO("Add the rest of the VVCs");
  for (int i = 1; i < 5; i++) {
    addVvc(cosim_data, i);
  }

  INFO("Compare all VVCs in retrieved list, ignoring order");
  vvc_list = cosim_data.GetVvcList();
  REQUIRE(vvc_list.size() == 5);

  for (int i = 0; i < 5; i++) {
    auto info = findInfo(vvc_list, i);
    REQUIRE(info.vvc_type        == to_string(vk[i].vvc_type));
    REQUIRE(info.vvc_channel     == to_string(vk[i].vvc_channel));
    REQUIRE(info.vvc_instance_id == vk[i].vvc_instance_id);
    REQUIRE(info.listen_enable   == false);
    REQUIRE(info.bfm_cfg.size()  == bfm_cfgs[i].size());
    for (auto& [k, v] : bfm_cfgs[i]) {
      REQUIRE(info.bfm_cfg.contains(k));
      REQUIRE(info.bfm_cfg.at(k) == v);
    }
  }
}

TEST_CASE("UvvmCosimData_SetVvcListenEnable")
{
  INFO("UvvmCosimData_SetVvcListenEnable test start.");

  UvvmCosimData cosim_data;

  INFO("Add all VVCs.");
  for (int i = 0; i < 5; i++) {
    addVvc(cosim_data, i);
  }

  auto vvc_list = cosim_data.GetVvcList();
  REQUIRE(vvc_list.size() == 5);

  VvcIdentifier bad_id{VvcTypes::AXI_STREAM, VvcChannel::NA, 113};

  INFO("Set listen enable on non-existent VVC should cause exception");
  REQUIRE_THROWS(cosim_data.SetVvcListenEnable(bad_id, true));

  INFO("Check that listen enable flag is false initially for all VVCs");
  for (auto& v : vvc_list) {
    REQUIRE(v.listen_enable == false);
  }

  INFO("Set listen enable flags to match listen_enable[]");
  for (int i = 0; i < 5; i++) {
    cosim_data.SetVvcListenEnable(vk[i], listen_enable[i]);
  }

  INFO("Retrieve updated list and verify listen_enable flags");
  vvc_list = cosim_data.GetVvcList();
  for (int i = 0; i < 5; i++) {
    auto info = findInfo(vvc_list, i);
    REQUIRE(info.listen_enable == listen_enable[i]);
  }
}

TEST_CASE("UvvmCosimData_byte_queues")
{
  INFO("UvvmCosimData_byte_queues test start.");

  UvvmCosimData cosim_data;

  INFO("Add all VVCs.");
  for (int i = 0; i < 5; i++) {
    addVvc(cosim_data, i);
  }

  INFO("Check that all queues are empty initially");
  for (int i = 0; i < 5; i++) {
    REQUIRE(cosim_data.StreamQueueEmpty(QID_RECEIVE,  vk[i]));
    REQUIRE(cosim_data.StreamQueueEmpty(QID_TRANSMIT, vk[i]));
  }

  INFO("Put byte 0xAB in transmit queue for first VVC");
  cosim_data.StreamQueuePutByte(QID_TRANSMIT, vk[0], 0xAB, false);
  REQUIRE_FALSE(cosim_data.StreamQueueEmpty(QID_TRANSMIT, vk[0]));
  REQUIRE(cosim_data.StreamQueueSize(QID_TRANSMIT, vk[0]) == 1);

  INFO("Check that all other queues are empty");
  REQUIRE(cosim_data.StreamQueueEmpty(QID_RECEIVE, vk[0]));
  REQUIRE(cosim_data.StreamQueueSize(QID_RECEIVE, vk[0]) == 0);
  for (int i = 1; i < 5; i++) {
    REQUIRE(cosim_data.StreamQueueEmpty(QID_RECEIVE,  vk[i]));
    REQUIRE(cosim_data.StreamQueueEmpty(QID_TRANSMIT, vk[i]));
    REQUIRE(cosim_data.StreamQueueSize(QID_RECEIVE,  vk[i]) == 0);
    REQUIRE(cosim_data.StreamQueueSize(QID_TRANSMIT, vk[i]) == 0);
  }

  INFO("Fetch and check byte from transmit queue for first VVC");
  auto byte = cosim_data.StreamQueueGetByte(QID_TRANSMIT, vk[0]);
  REQUIRE(byte.has_value());
  REQUIRE(byte.value().first == 0xAB);

  INFO("Check queue empty and no more bytes returned");
  REQUIRE(cosim_data.StreamQueueSize(QID_TRANSMIT, vk[0]) == 0);
  REQUIRE(cosim_data.StreamQueueEmpty(QID_TRANSMIT, vk[0]));
  byte = cosim_data.StreamQueueGetByte(QID_TRANSMIT, vk[0]);
  REQUIRE_FALSE(byte.has_value());

  INFO("Put some data in different queues");
  std::vector<uint8_t> vk1_transmit_data {0xAA, 0xBB, 0xCC, 0xDD};
  std::vector<uint8_t> vk1_receive_data  {0x12, 0x23, 0x45, 0x67, 0x89};
  std::vector<uint8_t> vk2_transmit_data {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<uint8_t> vk3_receive_data  {0x12, 0x23, 0x45, 0x67, 0x89};

  // Mix of single-byte puts and vector puts
  cosim_data.StreamQueuePutByte(QID_TRANSMIT, vk[1], vk1_transmit_data[0], false);
  cosim_data.StreamQueuePutByte(QID_TRANSMIT, vk[1], vk1_transmit_data[1], false);
  cosim_data.StreamQueuePutPacket(QID_TRANSMIT, vk[1],
    std::vector(vk1_transmit_data.begin()+2, vk1_transmit_data.end()));

  cosim_data.StreamQueuePutByte(QID_RECEIVE, vk[1], vk1_receive_data[0], false);
  cosim_data.StreamQueuePutPacket(QID_RECEIVE, vk[1],
    std::vector(vk1_receive_data.begin()+1, vk1_receive_data.end()-1));
  cosim_data.StreamQueuePutByte(QID_RECEIVE, vk[1], vk1_receive_data[4], false);

  cosim_data.StreamQueuePutPacket(QID_TRANSMIT, vk[2], vk2_transmit_data);
  cosim_data.StreamQueuePutPacket(QID_RECEIVE,  vk[3], vk3_receive_data);

  INFO("Check queue sizes");
  for (int i = 0; i < 5; i++) {
    switch (i) {
      case 1:
        REQUIRE_FALSE(cosim_data.StreamQueueEmpty(QID_TRANSMIT, vk[i]));
        REQUIRE(cosim_data.StreamQueueSize(QID_TRANSMIT, vk[i]) == vk1_transmit_data.size());
        break;
      case 2:
        REQUIRE_FALSE(cosim_data.StreamQueueEmpty(QID_TRANSMIT, vk[i]));
        REQUIRE(cosim_data.StreamQueueSize(QID_TRANSMIT, vk[i]) == vk2_transmit_data.size());
        break;
      default:
        REQUIRE(cosim_data.StreamQueueEmpty(QID_TRANSMIT, vk[i]));
        REQUIRE(cosim_data.StreamQueueSize(QID_TRANSMIT, vk[i]) == 0);
        break;
    }

    switch (i) {
      case 1:
        REQUIRE_FALSE(cosim_data.StreamQueueEmpty(QID_RECEIVE, vk[i]));
        REQUIRE(cosim_data.StreamQueueSize(QID_RECEIVE, vk[i]) == vk1_receive_data.size());
        break;
      case 3:
        REQUIRE_FALSE(cosim_data.StreamQueueEmpty(QID_RECEIVE, vk[i]));
        REQUIRE(cosim_data.StreamQueueSize(QID_RECEIVE, vk[i]) == vk3_receive_data.size());
        break;
      default:
        REQUIRE(cosim_data.StreamQueueEmpty(QID_RECEIVE, vk[i]));
        REQUIRE(cosim_data.StreamQueueSize(QID_RECEIVE, vk[i]) == 0);
        break;
    }
  }

  INFO("Get data from queues");

  // Get all bytes from vk[1] transmit queue (num_bytes=0 means "all available")
  std::vector<uint8_t> data = cosim_data.StreamQueueGetPacket(QID_TRANSMIT, vk[1], 0);
  REQUIRE(data == vk1_transmit_data);
  REQUIRE(cosim_data.StreamQueueEmpty(QID_TRANSMIT, vk[1]));

  // Fetch one byte from vk[2] transmit queue...
  byte = cosim_data.StreamQueueGetByte(QID_TRANSMIT, vk[2]);
  REQUIRE(byte.has_value());
  REQUIRE(byte.value().first == vk2_transmit_data[0]);
  REQUIRE(cosim_data.StreamQueueSize(QID_TRANSMIT, vk[2]) == vk2_transmit_data.size()-1);
  REQUIRE_FALSE(cosim_data.StreamQueueEmpty(QID_TRANSMIT, vk[2]));

  // ... then one byte from vk[3] receive queue ...
  byte = cosim_data.StreamQueueGetByte(QID_RECEIVE, vk[3]);
  REQUIRE(byte.has_value());
  REQUIRE(byte.value().first == vk3_receive_data[0]);
  REQUIRE(cosim_data.StreamQueueSize(QID_RECEIVE, vk[3]) == vk3_receive_data.size()-1);
  REQUIRE_FALSE(cosim_data.StreamQueueEmpty(QID_RECEIVE, vk[3]));

  // ... then the rest from vk[2] transmit queue
  data = cosim_data.StreamQueueGetPacket(QID_TRANSMIT, vk[2], 0);
  REQUIRE(data.size() == vk2_transmit_data.size()-1);
  REQUIRE(data == std::vector(vk2_transmit_data.begin()+1, vk2_transmit_data.end()));
  REQUIRE(cosim_data.StreamQueueEmpty(QID_TRANSMIT, vk[2]));

  // ... then the rest from vk[3] receive queue
  data = cosim_data.StreamQueueGetPacket(QID_RECEIVE, vk[3], 0);
  REQUIRE(data.size() == vk3_receive_data.size()-1);
  REQUIRE(data == std::vector(vk3_receive_data.begin()+1, vk3_receive_data.end()));
  REQUIRE(cosim_data.StreamQueueEmpty(QID_RECEIVE, vk[3]));

  // Get all but one byte from vk[1] receive queue
  data = cosim_data.StreamQueueGetPacket(QID_RECEIVE, vk[1], (int)vk1_receive_data.size()-1);
  REQUIRE(data.size() == vk1_receive_data.size()-1);
  REQUIRE(data == std::vector(vk1_receive_data.begin(), vk1_receive_data.end()-1));

  // Fetch the last byte from vk[1] receive queue
  byte = cosim_data.StreamQueueGetByte(QID_RECEIVE, vk[1]);
  REQUIRE(byte.has_value());
  REQUIRE(byte.value().first == vk1_receive_data.back());

  INFO("Check that all queues are now empty");
  for (int i = 0; i < 5; i++) {
    REQUIRE(cosim_data.StreamQueueEmpty(QID_RECEIVE,  vk[i]));
    REQUIRE(cosim_data.StreamQueueEmpty(QID_TRANSMIT, vk[i]));
  }
}

TEST_CASE("UvvmCosimData_packet_queues")
{
  INFO("Exercise StreamQueue* against packet-based VVCs.");

  UvvmCosimData cosim_data;

  // Two packet-based AXI_STREAM VVCs.  bfm_cfg with packet_based=1 makes the
  // factory build VvcPacketStreamBased; UvvmCosimData::StreamQueue* dispatches
  // via virtual calls.
  std::map<std::string, int> pkt_cfg{{"packet_based", 1}};

  VvcIdentifier pkt_vvc_a{VvcTypes::AXI_STREAM, VvcChannel::NA, 1};
  VvcIdentifier pkt_vvc_b{VvcTypes::AXI_STREAM, VvcChannel::NA, 2};

  cosim_data.AddVvc(to_string(pkt_vvc_a.vvc_type), to_string(pkt_vvc_a.vvc_channel),
                    pkt_vvc_a.vvc_instance_id, pkt_cfg);
  cosim_data.AddVvc(to_string(pkt_vvc_b.vvc_type), to_string(pkt_vvc_b.vvc_channel),
                    pkt_vvc_b.vvc_instance_id, pkt_cfg);

  // Sanity: packet_based flag visible in GetVvcList().
  auto list = cosim_data.GetVvcList();
  REQUIRE(list.size() == 2);
  for (auto& info : list) REQUIRE(info.packet_based == true);

  // Whole-packet round-trip
  std::vector<uint8_t> pkt_a{0x11, 0x22, 0x33, 0x44};
  std::vector<uint8_t> pkt_b{0xAA, 0xBB};

  cosim_data.StreamQueuePutPacket(QID_TRANSMIT, pkt_vvc_a, pkt_a);
  cosim_data.StreamQueuePutPacket(QID_TRANSMIT, pkt_vvc_a, pkt_b);

  REQUIRE(cosim_data.StreamQueueSize(QID_TRANSMIT, pkt_vvc_a) == 2);  // two packets
  REQUIRE_FALSE(cosim_data.StreamQueueEmpty(QID_TRANSMIT, pkt_vvc_a));

  // num_bytes is ignored for packet VVCs - returns next complete packet
  REQUIRE(cosim_data.StreamQueueGetPacket(QID_TRANSMIT, pkt_vvc_a, 99) == pkt_a);
  REQUIRE(cosim_data.StreamQueueGetPacket(QID_TRANSMIT, pkt_vvc_a, 0)  == pkt_b);
  REQUIRE(cosim_data.StreamQueueEmpty(QID_TRANSMIT, pkt_vvc_a));

  // Byte-wise put with eop delimits packets
  cosim_data.StreamQueuePutByte(QID_RECEIVE, pkt_vvc_b, 0xDE, false);
  cosim_data.StreamQueuePutByte(QID_RECEIVE, pkt_vvc_b, 0xAD, true);   // closes packet 1
  cosim_data.StreamQueuePutByte(QID_RECEIVE, pkt_vvc_b, 0xBE, false);
  cosim_data.StreamQueuePutByte(QID_RECEIVE, pkt_vvc_b, 0xEF, true);   // closes packet 2
  REQUIRE(cosim_data.StreamQueueSize(QID_RECEIVE, pkt_vvc_b) == 2);

  // Byte-wise get returns eop flag
  auto b = cosim_data.StreamQueueGetByte(QID_RECEIVE, pkt_vvc_b);
  REQUIRE(b.has_value());
  REQUIRE(b->first == 0xDE);
  REQUIRE(b->second == false);

  b = cosim_data.StreamQueueGetByte(QID_RECEIVE, pkt_vvc_b);
  REQUIRE(b->first == 0xAD);
  REQUIRE(b->second == true);    // last byte of packet 1

  // Read second packet whole
  REQUIRE(cosim_data.StreamQueueGetPacket(QID_RECEIVE, pkt_vvc_b, 0)
          == std::vector<uint8_t>{0xBE, 0xEF});
  REQUIRE(cosim_data.StreamQueueEmpty(QID_RECEIVE, pkt_vvc_b));

  // Empty queue: GetByte returns nullopt, GetPacket returns empty vector
  REQUIRE_FALSE(cosim_data.StreamQueueGetByte(QID_TRANSMIT, pkt_vvc_b).has_value());
  REQUIRE(cosim_data.StreamQueueGetPacket(QID_TRANSMIT, pkt_vvc_b, 0).empty());
}

TEST_CASE("UvvmCosimData_packet_byte_dual_access")
{
  UvvmCosimData cosim_data;

  std::map<std::string, int> byte_cfg{};
  std::map<std::string, int> pkt_cfg {{"packet_based", 1}};

  VvcIdentifier byte_vvc{VvcTypes::AXI_STREAM, VvcChannel::NA, 10};
  VvcIdentifier pkt_vvc {VvcTypes::AXI_STREAM, VvcChannel::NA, 20};

  cosim_data.AddVvc("AXISTREAM_VVC", "NA", byte_vvc.vvc_instance_id, byte_cfg);
  cosim_data.AddVvc("AXISTREAM_VVC", "NA", pkt_vvc.vvc_instance_id,  pkt_cfg);

  INFO("Byte VVC: eop arg to PutByte is ignored, GetByte always returns eop=false");
  cosim_data.StreamQueuePutByte(QID_TRANSMIT, byte_vvc, 0x01, /*eop=*/true);
  cosim_data.StreamQueuePutByte(QID_TRANSMIT, byte_vvc, 0x02, /*eop=*/false);

  auto b = cosim_data.StreamQueueGetByte(QID_TRANSMIT, byte_vvc);
  REQUIRE(b.has_value());
  REQUIRE(b->first  == 0x01);
  REQUIRE(b->second == false);   // byte stream never reports eop

  b = cosim_data.StreamQueueGetByte(QID_TRANSMIT, byte_vvc);
  REQUIRE(b->first  == 0x02);
  REQUIRE(b->second == false);

  INFO("Byte VVC: PutPacket appends bytes without a boundary; GetPacket(N) returns first N");
  cosim_data.StreamQueuePutPacket(QID_TRANSMIT, byte_vvc,
                                  std::vector<uint8_t>{0xA, 0xB, 0xC, 0xD});
  REQUIRE(cosim_data.StreamQueueSize(QID_TRANSMIT, byte_vvc) == 4);

  auto first_two = cosim_data.StreamQueueGetPacket(QID_TRANSMIT, byte_vvc, 2);
  REQUIRE(first_two == std::vector<uint8_t>{0xA, 0xB});

  INFO("Packet VVC: PutByte(eop=true) closes a packet; num_bytes arg to GetPacket is ignored");
  cosim_data.StreamQueuePutByte(QID_RECEIVE, pkt_vvc, 0x77, /*eop=*/true);
  REQUIRE(cosim_data.StreamQueueSize(QID_RECEIVE, pkt_vvc) == 1);
  // num_bytes=99 should be ignored — returns the complete single-byte packet
  REQUIRE(cosim_data.StreamQueueGetPacket(QID_RECEIVE, pkt_vvc, 99)
          == std::vector<uint8_t>{0x77});
}

TEST_CASE("UvvmCosimData_mem_mapped_queues")
{
  INFO("Add one mem-mapped VVC of each type and round-trip Transfers.");

  UvvmCosimData cosim_data;

  std::map<std::string, int> cfg{};
  VvcIdentifier axilite{VvcTypes::AXILITE,    VvcChannel::NA, 0};
  VvcIdentifier axi    {VvcTypes::AXI,        VvcChannel::NA, 1};
  VvcIdentifier avmm   {VvcTypes::AVALON_MM,  VvcChannel::NA, 2};

  cosim_data.AddVvc("AXILITE_VVC",   "NA", axilite.vvc_instance_id, cfg);
  cosim_data.AddVvc("AXI_VVC",       "NA", axi.vvc_instance_id,     cfg);
  cosim_data.AddVvc("AVALON_MM_VVC", "NA", avmm.vvc_instance_id,    cfg);

  REQUIRE(cosim_data.GetVvcList().size() == 3);

  // Empty initially
  REQUIRE(cosim_data.MemQueueEmpty(QID_REQUEST,  axilite));
  REQUIRE(cosim_data.MemQueueEmpty(QID_RESPONSE, axilite));
  REQUIRE_FALSE(cosim_data.MemQueueGet(QID_REQUEST, axilite).has_value());

  // Push two write requests, one read request
  Transfer w1; w1.addr = 0x0100; w1.data = {0xDE, 0xAD};                  w1.num_bytes = 2;
  Transfer w2; w2.addr = 0x0200; w2.data = {0xBE, 0xEF, 0xCA, 0xFE};      w2.num_bytes = 4;
  Transfer r1; r1.addr = 0x0300; r1.addr_increment = false;               r1.num_bytes = 8;

  cosim_data.MemQueuePut(QID_REQUEST, axilite, w1);
  cosim_data.MemQueuePut(QID_REQUEST, axilite, w2);
  cosim_data.MemQueuePut(QID_REQUEST, axilite, r1);

  REQUIRE_FALSE(cosim_data.MemQueueEmpty(QID_REQUEST, axilite));

  // FIFO order
  auto t1 = cosim_data.MemQueueGet(QID_REQUEST, axilite);
  REQUIRE(t1.has_value());
  REQUIRE(t1->addr == 0x0100);
  REQUIRE(t1->data == std::vector<uint8_t>{0xDE, 0xAD});

  auto t2 = cosim_data.MemQueueGet(QID_REQUEST, axilite);
  REQUIRE(t2->addr == 0x0200);
  REQUIRE(t2->num_bytes == 4);

  auto t3 = cosim_data.MemQueueGet(QID_REQUEST, axilite);
  REQUIRE(t3->addr == 0x0300);
  REQUIRE(t3->data.empty());          // read request
  REQUIRE(t3->addr_increment == false);

  REQUIRE(cosim_data.MemQueueEmpty(QID_REQUEST, axilite));
  REQUIRE_FALSE(cosim_data.MemQueueGet(QID_REQUEST, axilite).has_value());

  // Response queue is independent
  Transfer resp; resp.addr = 0x0300; resp.data = {0x01, 0x02, 0x03, 0x04};
  cosim_data.MemQueuePut(QID_RESPONSE, axi, resp);
  REQUIRE_FALSE(cosim_data.MemQueueEmpty(QID_RESPONSE, axi));
  REQUIRE(cosim_data.MemQueueEmpty(QID_REQUEST,  axi));   // request queue untouched
  REQUIRE(cosim_data.MemQueueEmpty(QID_RESPONSE, axilite)); // different VVC untouched

  auto r = cosim_data.MemQueueGet(QID_RESPONSE, axi);
  REQUIRE(r.has_value());
  REQUIRE(r->data == std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04});
}

TEST_CASE("UvvmCosimData_wrong_vvc_kind_throws")
{
  // The dynamic_cast guards in getStreamVvc / getMemVvc must reject the
  // wrong family of VVC.
  UvvmCosimData cosim_data;

  std::map<std::string, int> cfg{};
  VvcIdentifier uart   {VvcTypes::UART,    VvcChannel::TX, 0};
  VvcIdentifier axilite{VvcTypes::AXILITE, VvcChannel::NA, 0};

  cosim_data.AddVvc("UART_VVC",    "TX", 0, cfg);
  cosim_data.AddVvc("AXILITE_VVC", "NA", 0, cfg);

  // Stream API on a mem-mapped VVC must throw
  REQUIRE_THROWS(cosim_data.StreamQueueEmpty(QID_TRANSMIT, axilite));
  REQUIRE_THROWS(cosim_data.StreamQueuePutByte(QID_TRANSMIT, axilite, 0, false));

  // Mem API on a stream VVC must throw
  Transfer t;
  REQUIRE_THROWS(cosim_data.MemQueueEmpty(QID_REQUEST, uart));
  REQUIRE_THROWS(cosim_data.MemQueuePut(QID_REQUEST,   uart, t));
  REQUIRE_THROWS(cosim_data.MemQueueGet(QID_RESPONSE,  uart));
}

TEST_CASE("UvvmCosimData_simulation_flags")
{
  UvvmCosimData cosim_data;

  REQUIRE(cosim_data.getStartSim()     == false);
  REQUIRE(cosim_data.getTerminateSim() == false);

  cosim_data.setStartSim(true);
  REQUIRE(cosim_data.getStartSim()     == true);
  REQUIRE(cosim_data.getTerminateSim() == false);

  cosim_data.setTerminateSim(true);
  REQUIRE(cosim_data.getTerminateSim() == true);

  cosim_data.setStartSim(false);
  cosim_data.setTerminateSim(false);
  REQUIRE(cosim_data.getStartSim()     == false);
  REQUIRE(cosim_data.getTerminateSim() == false);
}

TEST_CASE("UvvmCosimData_GetVvcListenEnable")
{
  UvvmCosimData cosim_data;
  std::map<std::string, int> cfg{};
  VvcIdentifier id{VvcTypes::UART, VvcChannel::TX, 0};

  cosim_data.AddVvc("UART_VVC", "TX", 0, cfg);

  REQUIRE(cosim_data.GetVvcListenEnable(id) == false);
  cosim_data.SetVvcListenEnable(id, true);
  REQUIRE(cosim_data.GetVvcListenEnable(id) == true);

  VvcIdentifier missing{VvcTypes::UART, VvcChannel::TX, 99};
  REQUIRE_THROWS(cosim_data.GetVvcListenEnable(missing));
}

TEST_CASE("UvvmCosimData_AddVvc_unsupported_type_silently_skipped")
{
  // The factory throws on unknown types; UvvmCosimData::AddVvc catches that
  // and prints a warning instead of propagating.
  UvvmCosimData cosim_data;
  std::map<std::string, int> cfg{};

  // Should NOT throw; should NOT add anything
  cosim_data.AddVvc("CLOCK_GENERATOR_VVC", "NA", 0, cfg);
  REQUIRE(cosim_data.GetVvcList().empty());
}
