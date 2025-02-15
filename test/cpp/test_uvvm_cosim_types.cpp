#define CONFIG_CATCH_MAIN
#include <map>
#include <vector>
#include <catch2/catch.hpp>
#include "nlohmann/json.hpp"
#include "uvvm_cosim_types.hpp"

// Some VVC examples placed in the order we expect VvcCompare to sort them
std::vector<VvcInstance> v = {
  VvcInstance{"AXISTREAM_VVC", "NA", 1},
  VvcInstance{"AXISTREAM_VVC", "NA", 10},
  VvcInstance{"UART_VVC", "RX", 3},
  VvcInstance{"UART_VVC", "TX", 0},
  VvcInstance{"UART_VVC", "TX", 3}
};

TEST_CASE("VvcCompare_order")
{
  INFO("VvcCompare_order test start. Checks expected sorting order.");

  // The order doesn't really matter for the co-sim server.
  // But, std::map requires comparator class with strict-order sorting
  // to work correctly, so makes to actually test that.

  std::map<VvcInstance, VvcQueues, VvcCompare> m;

  // Insert to map out-of-order
  m.emplace(v[4], VvcQueues());
  m.emplace(v[3], VvcQueues());
  m.emplace(v[1], VvcQueues());
  m.emplace(v[2], VvcQueues());
  m.emplace(v[0], VvcQueues());

  // If comparator class is bad then we could end up overwriting
  // existing entries in the map
  REQUIRE(m.size() == v.size());

  // Verify expected order when iterating over map
  int idx=0;
  for (auto it = m.begin(); it != m.end(); it++, idx++)
  {
    REQUIRE(it->first.vvc_type == v[idx].vvc_type);
    REQUIRE(it->first.vvc_channel == v[idx].vvc_channel);
    REQUIRE(it->first.vvc_instance_id == v[idx].vvc_instance_id);
  }
}

TEST_CASE("VvcInstance_to_json")
{
  INFO("VvcInstance_to_json test start. Checks conversion from VvcInstance to json.");

  for (auto& vvc : v)
  {
    nlohmann::json j = vvc;

    REQUIRE(j.contains("vvc_type"));
    REQUIRE(j.contains("vvc_channel"));
    REQUIRE(j.contains("vvc_instance_id"));
    REQUIRE(j.contains("vvc_cfg"));

    REQUIRE(j["vvc_type"] == vvc.vvc_type);
    REQUIRE(j["vvc_channel"] == vvc.vvc_channel);
    REQUIRE(j["vvc_instance_id"] == vvc.vvc_instance_id);

    REQUIRE(j["vvc_cfg"].size() == vvc.vvc_cfg.size());

    for (const auto& cfg_entry : vvc.vvc_cfg)
    {
      REQUIRE(j.contains(cfg_entry.first));
      REQUIRE(j[cfg_entry.first] == cfg_entry.second);
    }
  }
}

TEST_CASE("VvcInstance_from_json")
{
  INFO("VvcInstance_from_json test start. Checks conversion to VvcInstance from json.");

  for (auto& vvc : v)
  {
    nlohmann::json j = vvc;
    VvcInstance vvc_converted = j;

    REQUIRE(vvc_converted.vvc_type == vvc.vvc_type);
    REQUIRE(vvc_converted.vvc_channel == vvc.vvc_channel);
    REQUIRE(vvc_converted.vvc_instance_id == vvc.vvc_instance_id);

    REQUIRE(vvc_converted.vvc_cfg.size() == vvc.vvc_cfg.size());

    for (const auto& cfg_entry : vvc_converted.vvc_cfg)
    {
      REQUIRE(vvc.vvc_cfg.contains(cfg_entry.first));
      REQUIRE(vvc.vvc_cfg[cfg_entry.first] == cfg_entry.second);
    }
  }
}
