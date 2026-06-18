#include <catch2/catch_test_macros.hpp>
#include <map>
#include <vector>
#include "nlohmann/json.hpp"
#include "uvvm_cosim_types.hpp"

using namespace uvvm_cosim;

// VVC identifiers in the order VvcCompare should sort them.
// Sort key: (vvc_type enum value, vvc_channel enum value, instance_id).
// AXI_STREAM=0, UART=5; NA=0, RX=1, TX=2.
std::vector<VvcIdentifier> vk = {
  {VvcTypes::AXI_STREAM, VvcChannel::NA, 1},
  {VvcTypes::AXI_STREAM, VvcChannel::NA, 10},
  {VvcTypes::UART,       VvcChannel::RX, 3},
  {VvcTypes::UART,       VvcChannel::TX, 0},
  {VvcTypes::UART,       VvcChannel::TX, 3},
};

TEST_CASE("VvcCompare_order")
{
  INFO("Checks that VvcCompare provides strict ordering for std::map.");

  std::map<VvcIdentifier, int, VvcCompare> m;

  // Insert out-of-order
  m.emplace(vk[4], 0);
  m.emplace(vk[3], 1);
  m.emplace(vk[1], 2);
  m.emplace(vk[2], 3);
  m.emplace(vk[0], 4);

  // A bad comparator would cause duplicate keys; verify all five are distinct.
  REQUIRE(m.size() == vk.size());

  // Verify expected iteration order.
  int idx = 0;
  for (auto it = m.begin(); it != m.end(); ++it, ++idx) {
    REQUIRE(it->first.vvc_type        == vk[idx].vvc_type);
    REQUIRE(it->first.vvc_channel     == vk[idx].vvc_channel);
    REQUIRE(it->first.vvc_instance_id == vk[idx].vvc_instance_id);
  }
}

TEST_CASE("VvcInfo_to_json")
{
  INFO("Checks JSON serialization of VvcInfo (used in GetVvcList responses).");

  VvcInfo info;
  info.vvc_type        = "UART_VVC";
  info.vvc_channel     = "TX";
  info.vvc_instance_id = 2;
  info.listen_enable   = true;
  info.cosim_support   = true;
  info.packet_based    = false;
  info.bfm_cfg         = {{"timeout", 0}, {"baudrate", 115200}};

  nlohmann::json j = info;

  REQUIRE(j.contains("vvc_type"));
  REQUIRE(j.contains("vvc_channel"));
  REQUIRE(j.contains("vvc_instance_id"));
  REQUIRE(j.contains("listen_enable"));
  REQUIRE(j.contains("cosim_support"));
  REQUIRE(j.contains("packet_based"));
  REQUIRE(j.contains("bfm_cfg"));

  REQUIRE(j["vvc_type"]        == info.vvc_type);
  REQUIRE(j["vvc_channel"]     == info.vvc_channel);
  REQUIRE(j["vvc_instance_id"] == info.vvc_instance_id);
  REQUIRE(j["listen_enable"]   == info.listen_enable);
  REQUIRE(j["cosim_support"]   == info.cosim_support);
  REQUIRE(j["packet_based"]    == info.packet_based);
  REQUIRE(j["bfm_cfg"].size()  == info.bfm_cfg.size());

  for (auto& [k, v] : info.bfm_cfg) {
    REQUIRE(j["bfm_cfg"].contains(k));
    REQUIRE(j["bfm_cfg"][k] == v);
  }
}

TEST_CASE("JsonResponse_to_from_json_round_trip")
{
  // JsonResponse is the wrapper returned by every RPC method.  to_json/from_json
  // are used by jsonrpccxx to serialize replies and (in the C++ client) to
  // deserialize them.
  JsonResponse out;
  out.success = true;
  out.result  = nlohmann::json{{"foo", 1}, {"bar", "baz"}, {"arr", {1, 2, 3}}};

  nlohmann::json j = out;
  REQUIRE(j.contains("success"));
  REQUIRE(j.contains("result"));
  REQUIRE(j["success"] == true);
  REQUIRE(j["result"]["foo"] == 1);
  REQUIRE(j["result"]["bar"] == "baz");

  JsonResponse roundtripped = j.get<JsonResponse>();
  REQUIRE(roundtripped.success == out.success);
  REQUIRE(roundtripped.result  == out.result);

  // success=false case
  JsonResponse fail{false, nlohmann::json{{"error", "nope"}}};
  nlohmann::json jf = fail;
  REQUIRE(jf["success"] == false);
  REQUIRE(jf["result"]["error"] == "nope");
  JsonResponse fail_rt = jf.get<JsonResponse>();
  REQUIRE(fail_rt.success == false);
  REQUIRE(fail_rt.result["error"] == "nope");
}

TEST_CASE("VvcCompare_equal_identifiers")
{
  // Equal keys must compare !(a < b) && !(b < a) — required for std::map
  // to treat them as the same key.
  VvcCompare cmp;
  VvcIdentifier a{VvcTypes::UART, VvcChannel::TX, 3};
  VvcIdentifier b{VvcTypes::UART, VvcChannel::TX, 3};

  REQUIRE_FALSE(cmp(a, b));
  REQUIRE_FALSE(cmp(b, a));

  // Differing only by instance_id: ordering follows instance_id
  VvcIdentifier c{VvcTypes::UART, VvcChannel::TX, 4};
  REQUIRE(cmp(a, c));
  REQUIRE_FALSE(cmp(c, a));

  // Differing only by channel: ordering follows channel enum
  VvcIdentifier d{VvcTypes::UART, VvcChannel::RX, 3};
  REQUIRE(cmp(d, a));         // RX(1) < TX(2)
  REQUIRE_FALSE(cmp(a, d));
}
