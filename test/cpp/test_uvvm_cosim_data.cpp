#include <catch2/catch_test_macros.hpp>
#include <map>
#include <stdexcept>
#include <vector>
#include "uvvm_cosim_data.hpp"
#include "uvvm_cosim_types.hpp"

using namespace uvvm_cosim;

// Some VVC examples placed in the order we expect VvcCompare to sort them
std::vector<VvcInstanceKey> vk = {
    VvcInstanceKey{"AXISTREAM_VVC", "NA", 1},
    VvcInstanceKey{"AXISTREAM_VVC", "NA", 10},
    VvcInstanceKey{"UART_VVC", "RX", 3},
    VvcInstanceKey{"UART_VVC", "TX", 0},
    VvcInstanceKey{"UART_VVC", "TX", 3}
};

// Some random imaginary config values pairs
std::vector<VvcConfig> vc = {
  VvcConfig{.listen_enable=true, .bfm_cfg = {{"cfg_val1", 0}, {"cfg_val2", 1}, {"cfg_val3", 10}}},
  VvcConfig{.listen_enable=false, .bfm_cfg = {{"airplane", 200}, {"helicopter", 123}}},
  VvcConfig{.listen_enable=false, .bfm_cfg = {{"spaghetti", 1}, {"pizza", 234}, {"lasagne", 0}, {"risotto", 89}}},
  VvcConfig{.listen_enable=true, .bfm_cfg = {{"cookie", 14}}},
  VvcConfig{.listen_enable=false, .bfm_cfg = {{"firetruck", 123}, {"boat", 23}, {"submarine", 10}}}
};

// Some VVC instances key+config data. GetVvcList returns VvcInstance.
std::vector<VvcInstance> vi = {
  {vk[0], vc[0]},
  {vk[1], vc[1]},
  {vk[2], vc[2]},
  {vk[3], vc[3]},
  {vk[4], vc[4]},
};

static inline bool operator==(const VvcInstance& lhs, const VvcInstance& rhs)
{
  return
    lhs.vvc_type == rhs.vvc_type &&
    lhs.vvc_channel == rhs.vvc_channel &&
    lhs.vvc_instance_id == rhs.vvc_instance_id;
}

TEST_CASE("UvvmCosimData_AddVvc_and_GetVvcList")
{
  INFO("UvvmCosimData_AddVvc test start. Check adding and retrieving VVCs.");

  UvvmCosimData cosim_data;

  std::vector<VvcInstance> vvc_list = cosim_data.GetVvcList();

  INFO("Check empty initially");
  REQUIRE(vvc_list.empty() == true);

  INFO("Add one VVC, retrieve VVC list and check that it matches");
  cosim_data.AddVvc(vk[0], vc[0].bfm_cfg);

  vvc_list = cosim_data.GetVvcList();
  REQUIRE(vvc_list.empty() == false);
  REQUIRE(vvc_list.size() == 1);
  REQUIRE(vvc_list[0].vvc_type == vk[0].vvc_type);
  REQUIRE(vvc_list[0].vvc_channel == vk[0].vvc_channel);
  REQUIRE(vvc_list[0].vvc_instance_id == vk[0].vvc_instance_id);

  // NOTE:
  // AddVvc only takes the bfm_cfg map, not the whole VvcConfig type.
  // So the listen_enable flag has not been set and should be false initially.
  REQUIRE(vvc_list[0].listen_enable == false);

  REQUIRE(vc[0].bfm_cfg.size() == vvc_list[0].bfm_cfg.size());
  
  for (auto const &kv : vc[0].bfm_cfg) {
    REQUIRE(vvc_list[0].bfm_cfg.contains(kv.first));
    REQUIRE(vvc_list[0].bfm_cfg[kv.first] == kv.second);
  }
  //REQUIRE(vvc_list[0].cosim_support == false); // Not implemented
  //REQUIRE(vvc_list[0].packet_based == false); // Not implemented


  INFO("Check that same VVC can't be added twice");
  REQUIRE_THROWS(cosim_data.AddVvc(vk[0], vc[0].bfm_cfg));

  INFO("Check that size remained the same after throw");
  vvc_list = cosim_data.GetVvcList();
  REQUIRE(vvc_list.size() == 1);

  INFO("Add the rest of the VVCs");
  for (int i = 1; i < 5; i++) {
    cosim_data.AddVvc(vk[i], vc[i].bfm_cfg);
  }

  INFO("Compare all VVCs in retrieved list, but ignoring order");
  vvc_list = cosim_data.GetVvcList();
  REQUIRE(vvc_list.size() == 5);

  for (int i = 0; i < 5; i++) {
    auto it = std::find_if(vvc_list.begin(), vvc_list.end(), [&](const VvcInstance& v) {return v == vi[i];});

    
    REQUIRE(vvc_list[i].vvc_type == it->vvc_type);
    REQUIRE(vvc_list[i].vvc_channel == it->vvc_channel);
    REQUIRE(vvc_list[i].vvc_instance_id == it->vvc_instance_id);
    REQUIRE(vvc_list[i].listen_enable == it->listen_enable);
    REQUIRE(vvc_list[i].bfm_cfg.size() == it->bfm_cfg.size());
  
    for (auto const &kv : vvc_list[i].bfm_cfg) {
      REQUIRE(it->bfm_cfg.contains(kv.first));
      REQUIRE(it->bfm_cfg[kv.first] == kv.second);
    }
  }
}

TEST_CASE("UvvmCosimData_SetVvcListenEnable")
{
  INFO("UvvmCosimData_SetVvcListenEnable test start.");

  UvvmCosimData cosim_data;

  INFO("Add all VVCs.");
  for (int i = 0; i < 5; i++) {
    cosim_data.AddVvc(vk[i], vc[i].bfm_cfg);
  }

  std::vector<VvcInstance> vvc_list = cosim_data.GetVvcList();
  REQUIRE(vvc_list.size() == 5);

  VvcInstanceKey bad_vk {"AXISTREAM_VVC", "NA", 113};

  // Try setting listen enable flag on non-existant VVC. Should throw.
  INFO("Set listen enable on non-existant VVC should cause exception");
  REQUIRE_THROWS(cosim_data.SetVvcListenEnable(bad_vk, true));

  // Check that listen enable flag is false for all VVCs
  INFO("Check that listen enable flag is false initially for all VVCs");
  for (auto& v : vvc_list) {
    REQUIRE(v.listen_enable == false);
  }

  INFO("Set listen enable flags so they corresponds to those in the vi/vc lists");
  for (auto& v: vi) {
    cosim_data.SetVvcListenEnable(static_cast<VvcInstanceKey>(v), v.listen_enable);
  }

  // Retrieve VVC list again after changes
  vvc_list = cosim_data.GetVvcList();

  INFO("Check that listen enable flags in cosim_data matches those in the vi/vc lists");
  for (int i = 0; i < 5; i++) {
    auto it = std::find_if(vvc_list.begin(), vvc_list.end(), [&](const VvcInstance& v) {return v == vi[i];});

    REQUIRE(vi[i].listen_enable == it->listen_enable);
  }
}

TEST_CASE("UvvmCosimData_byte_queues")
{
  INFO("UvvmCosimData_byte_queues test start.");

  UvvmCosimData cosim_data;

  INFO("Add all VVCs.");
  for (int i = 0; i < 5; i++) {
    cosim_data.AddVvc(vk[i], vc[i].bfm_cfg);
  }

  INFO("Check that all queues are empty initially");
  for (int i = 0; i < 5; i++) {
    REQUIRE(cosim_data.byte_queue_empty(QID_RECEIVE, vk[i]));
    REQUIRE(cosim_data.byte_queue_empty(QID_TRANSMIT, vk[i]));
  }

  INFO("Put byte 0xAB in transmit queue for first VVC");
  cosim_data.byte_queue_put(QID_TRANSMIT, vk[0], 0xAB);
  REQUIRE_FALSE(cosim_data.byte_queue_empty(QID_TRANSMIT, vk[0]));
  REQUIRE(cosim_data.byte_queue_size(QID_TRANSMIT, vk[0]) == 1);

  INFO("Check that all other queues are empty");
  REQUIRE(cosim_data.byte_queue_empty(QID_RECEIVE, vk[0]));
  REQUIRE(cosim_data.byte_queue_size(QID_RECEIVE, vk[0]) == 0);
  for (int i = 1; i < 5; i++) {
    REQUIRE(cosim_data.byte_queue_empty(QID_RECEIVE, vk[i]));
    REQUIRE(cosim_data.byte_queue_empty(QID_TRANSMIT, vk[i]));
    REQUIRE(cosim_data.byte_queue_size(QID_RECEIVE, vk[i]) == 0);
    REQUIRE(cosim_data.byte_queue_size(QID_TRANSMIT, vk[i]) == 0);
  }

  INFO("Fetch and check byte from transmit queue for first VVC");
  std::optional<uint8_t> byte = cosim_data.byte_queue_get(QID_TRANSMIT, vk[0]);
  REQUIRE(byte.has_value());
  REQUIRE(byte.value() == 0xAB);

  INFO("Check queue empty and no more bytes returned");
  REQUIRE(cosim_data.byte_queue_size(QID_TRANSMIT, vk[0]) == 0);
  REQUIRE(cosim_data.byte_queue_empty(QID_TRANSMIT, vk[0]));
  byte = cosim_data.byte_queue_get(QID_TRANSMIT, vk[0]);
  REQUIRE_FALSE(byte.has_value());


  // TODO:
  // Should program a smarter test than this manual approach
  
  INFO("Put some data in different queues");
  std::vector<uint8_t> vk1_transmit_data {0xAA, 0xBB, 0xCC, 0xDD};
  std::vector<uint8_t> vk1_receive_data {0x12, 0x23, 0x45, 0x67, 0x89};
  std::vector<uint8_t> vk2_transmit_data {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<uint8_t> vk3_receive_data {0x12, 0x23, 0x45, 0x67, 0x89};

  // Use combination of put(uint8_t) and put(std::vector<uint8_t)
  cosim_data.byte_queue_put(QID_TRANSMIT, vk[1], vk1_transmit_data[0]);
  cosim_data.byte_queue_put(QID_TRANSMIT, vk[1], vk1_transmit_data[1]);
  cosim_data.byte_queue_put(QID_TRANSMIT, vk[1], std::vector(vk1_transmit_data.begin()+2, vk1_transmit_data.end()));

  cosim_data.byte_queue_put(QID_RECEIVE, vk[1], vk1_receive_data[0]);
  cosim_data.byte_queue_put(QID_RECEIVE, vk[1], std::vector(vk1_receive_data.begin()+1, vk1_receive_data.end()-1));
  cosim_data.byte_queue_put(QID_RECEIVE, vk[1], vk1_receive_data[4]);

  cosim_data.byte_queue_put(QID_TRANSMIT, vk[2], vk2_transmit_data);
  cosim_data.byte_queue_put(QID_RECEIVE, vk[3], vk3_receive_data);

  INFO("Check queue sizes");
  for (int i = 0; i < 5; i++) {
    // Check transmit queue size/empty
    switch (i) {
      case 1:
	REQUIRE_FALSE(cosim_data.byte_queue_empty(QID_TRANSMIT, vk[i]));
	REQUIRE(cosim_data.byte_queue_size(QID_TRANSMIT, vk[i]) == vk1_transmit_data.size());
	break;
      case 2:
	REQUIRE_FALSE(cosim_data.byte_queue_empty(QID_TRANSMIT, vk[i]));
	REQUIRE(cosim_data.byte_queue_size(QID_TRANSMIT, vk[i]) == vk2_transmit_data.size());
	break;
      default:
	REQUIRE(cosim_data.byte_queue_empty(QID_TRANSMIT, vk[i]));
	REQUIRE(cosim_data.byte_queue_size(QID_TRANSMIT, vk[i]) == 0);
	break;
    }

    // Check receive queue size/empty
    switch (i) {
      case 1:
	REQUIRE_FALSE(cosim_data.byte_queue_empty(QID_RECEIVE, vk[i]));
	REQUIRE(cosim_data.byte_queue_size(QID_RECEIVE, vk[i]) == vk1_receive_data.size());
	break;
      case 3:
	REQUIRE_FALSE(cosim_data.byte_queue_empty(QID_RECEIVE, vk[i]));
	REQUIRE(cosim_data.byte_queue_size(QID_RECEIVE, vk[i]) == vk3_receive_data.size());
	break;
      default:
	REQUIRE(cosim_data.byte_queue_empty(QID_RECEIVE, vk[i]));
	REQUIRE(cosim_data.byte_queue_size(QID_RECEIVE, vk[i]) == 0);
	break;
    }
  }

  INFO("Get data from queues");

  // Get all bytes from vk[1] transmit queue
  std::vector<uint8_t> data = cosim_data.byte_queue_get(QID_TRANSMIT, vk[1], 0);
  REQUIRE(data == vk1_transmit_data);
  REQUIRE(cosim_data.byte_queue_empty(QID_TRANSMIT, vk[1]));

  // Fetch one byte from vk[2] transmit queue...
  byte = cosim_data.byte_queue_get(QID_TRANSMIT, vk[2]);
  REQUIRE(byte.has_value());
  REQUIRE(byte.value() == vk2_transmit_data[0]);
  REQUIRE(cosim_data.byte_queue_size(QID_TRANSMIT, vk[2]) == vk2_transmit_data.size()-1);
  REQUIRE_FALSE(cosim_data.byte_queue_empty(QID_TRANSMIT, vk[2]));

  // ... then one byte from vk[3] receive queue ...
  byte = cosim_data.byte_queue_get(QID_RECEIVE, vk[3]);
  REQUIRE(byte.has_value());
  REQUIRE(byte.value() == vk3_receive_data[0]);
  REQUIRE(cosim_data.byte_queue_size(QID_RECEIVE, vk[3]) == vk3_receive_data.size()-1);
  REQUIRE_FALSE(cosim_data.byte_queue_empty(QID_RECEIVE, vk[3]));

  // ... then the rest from vk[2] transmit queue
  data = cosim_data.byte_queue_get(QID_TRANSMIT, vk[2], 0);
  REQUIRE(data.size() == vk2_transmit_data.size()-1);
  REQUIRE(data == std::vector(vk2_transmit_data.begin()+1, vk2_transmit_data.end()));
  REQUIRE(cosim_data.byte_queue_empty(QID_TRANSMIT, vk[2]));

  // ... then the rest from vk[3] receive queue
  data = cosim_data.byte_queue_get(QID_RECEIVE, vk[3], 0);
  REQUIRE(data.size() == vk3_receive_data.size()-1);
  REQUIRE(data == std::vector(vk3_receive_data.begin()+1, vk3_receive_data.end()));
  REQUIRE(cosim_data.byte_queue_empty(QID_RECEIVE, vk[3]));

  // Get all but one byte from vk[1] receive queue
  data = cosim_data.byte_queue_get(QID_RECEIVE, vk[1], vk1_receive_data.size()-1);
  REQUIRE(data.size() == vk1_receive_data.size()-1);
  REQUIRE(data == std::vector(vk1_receive_data.begin(), vk1_receive_data.end()-1));

  // Fetch the last byte from vk[1] receive queue
  byte = cosim_data.byte_queue_get(QID_RECEIVE, vk[1]);
  REQUIRE(byte.has_value());
  REQUIRE(byte.value() == vk1_receive_data.back());


  INFO("Check that all queues are now empty");
  for (int i = 0; i < 5; i++) {
    REQUIRE(cosim_data.byte_queue_empty(QID_RECEIVE, vk[i]));
    REQUIRE(cosim_data.byte_queue_empty(QID_TRANSMIT, vk[i]));
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
