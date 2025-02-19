#include <catch2/catch_test_macros.hpp>
#include <map>
#include <vector>
#include "nlohmann/json.hpp"
#include "uvvm_cosim_types.hpp"

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
  VvcConfig{.bfm_cfg = {{"cfg_val1", 0}, {"cfg_val2", 1}, {"cfg_val3", 10}}, .listen_enable=true},
  VvcConfig{.bfm_cfg = {{"airplane", 200}, {"helicopter", 123}}, .listen_enable=false},
  VvcConfig{.bfm_cfg = {{"spaghetti", 1}, {"pizza", 234}, {"lasagne", 0}, {"risotto", 89}}, .listen_enable=false},
  VvcConfig{.bfm_cfg = {{"cookie", 14}}, .listen_enable=true},
  VvcConfig{.bfm_cfg = {{"firetruck", 123}, {"boat", 23}, {"submarine", 10}}, .listen_enable=false}
};

// Some VVC instances key+config data. Used to test to/from JSON.
std::vector<VvcInstance> vi = {
  {vk[0], vc[0]},
  {vk[1], vc[1]},
  {vk[2], vc[2]},
  {vk[3], vc[3]},
  {vk[4], vc[4]},
};

TEST_CASE("VvcCompare_order")
{
  INFO("VvcCompare_order test start. Checks expected sorting order.");

  // The order doesn't really matter for the co-sim server.
  // But, std::map requires comparator class with strict-order sorting
  // to work correctly, so makes to actually test that.
  // And we don't really care about data here - may as well use integers

  std::map<VvcInstanceKey, int, VvcCompare> m;

  // Insert to map out-of-order
  m.emplace(vk[4], 0);
  m.emplace(vk[3], 1);
  m.emplace(vk[1], 2);
  m.emplace(vk[2], 3);
  m.emplace(vk[0], 4);

  // If comparator class is bad then we could end up overwriting
  // existing entries in the map
  REQUIRE(m.size() == vk.size());

  // Verify expected order when iterating over map
  int idx=0;
  for (auto it = m.begin(); it != m.end(); it++, idx++)
  {
    REQUIRE(it->first.vvc_type == vk[idx].vvc_type);
    REQUIRE(it->first.vvc_channel == vk[idx].vvc_channel);
    REQUIRE(it->first.vvc_instance_id == vk[idx].vvc_instance_id);
  }
}

TEST_CASE("VvcInstance_to_json")
{
  INFO("VvcInstance_to_json test start. Checks conversion from VvcInstance to json.");

  for (auto& vvc : vi)
  {
    nlohmann::json j = vvc;

    REQUIRE(j.contains("vvc_type"));
    REQUIRE(j.contains("vvc_channel"));
    REQUIRE(j.contains("vvc_instance_id"));
    REQUIRE(j.contains("bfm_cfg"));
    REQUIRE(j.contains("listen_enable"));

    REQUIRE(j["vvc_type"] == vvc.vvc_type);
    REQUIRE(j["vvc_channel"] == vvc.vvc_channel);
    REQUIRE(j["vvc_instance_id"] == vvc.vvc_instance_id);
    REQUIRE(j["bfm_cfg"].size() == vvc.bfm_cfg.size());
    REQUIRE(j["listen_enable"] == vvc.listen_enable);

    for (const auto& cfg_entry : vvc.bfm_cfg)
    {
      REQUIRE(j["bfm_cfg"].contains(cfg_entry.first));
      REQUIRE(j["bfm_cfg"][cfg_entry.first] == cfg_entry.second);
    }
  }
}

TEST_CASE("VvcInstance_from_json")
{
  INFO("VvcInstance_from_json test start. Checks conversion to VvcInstance from json.");

  for (auto& vvc : vi)
  {
    nlohmann::json j = vvc;
    VvcInstance vvc_converted = j;

    REQUIRE(vvc_converted.vvc_type == vvc.vvc_type);
    REQUIRE(vvc_converted.vvc_channel == vvc.vvc_channel);
    REQUIRE(vvc_converted.vvc_instance_id == vvc.vvc_instance_id);
    REQUIRE(vvc_converted.bfm_cfg.size() == vvc.bfm_cfg.size());
    REQUIRE(vvc_converted.listen_enable == vvc.listen_enable);

    for (const auto& cfg_entry : vvc_converted.bfm_cfg)
    {
      REQUIRE(vvc.bfm_cfg.contains(cfg_entry.first));
      REQUIRE(vvc.bfm_cfg[cfg_entry.first] == cfg_entry.second);
    }
  }
}
