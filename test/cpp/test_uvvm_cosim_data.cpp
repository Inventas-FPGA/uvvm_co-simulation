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
  INFO("TODO: Not implemented yet");
}

TEST_CASE("UvvmCosimData_packet_based_queue_access")
{
  INFO("TODO: Not implemented yet");

  // Check that only packet queue methods can be used for
  // VVCs with packet_based flag set, and only byte queue
  // methods for those without the flag.
}
