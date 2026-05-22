#include <catch2/catch_test_macros.hpp>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include "vvc/vvc_factory.hpp"
#include "vvc/vvc_byte_stream.hpp"
#include "vvc/vvc_packet_stream.hpp"
#include "vvc/vvc_mem_mapped.hpp"

using namespace uvvm_cosim;

TEST_CASE("VvcFactory_stream_types_default_to_byte_stream")
{
  // No "packet_based" key in bfm_cfg → VvcByteStream
  std::map<std::string, int> cfg{};

  for (auto t : {VvcTypes::UART, VvcTypes::AXI_STREAM, VvcTypes::AVALON_ST}) {
    auto vvc = VvcFactory::create(t, VvcChannel::NA, 0, cfg);
    REQUIRE(vvc != nullptr);
    REQUIRE(dynamic_cast<VvcByteStream*>(vvc.get())          != nullptr);
    REQUIRE(dynamic_cast<VvcPacketStreamBased*>(vvc.get())   == nullptr);
    REQUIRE(vvc->vvc_type == t);
  }
}

TEST_CASE("VvcFactory_packet_based_flag_selects_packet_stream")
{
  // packet_based: 1 → VvcPacketStreamBased (only for AXI_STREAM / AVALON_ST)
  std::map<std::string, int> cfg{{"packet_based", 1}};

  for (auto t : {VvcTypes::AXI_STREAM, VvcTypes::AVALON_ST}) {
    auto vvc = VvcFactory::create(t, VvcChannel::NA, 0, cfg);
    REQUIRE(dynamic_cast<VvcPacketStreamBased*>(vvc.get()) != nullptr);
    REQUIRE(dynamic_cast<VvcByteStream*>(vvc.get())        == nullptr);
  }
}

TEST_CASE("VvcFactory_packet_based_zero_treated_as_false")
{
  // packet_based: 0 must select byte stream
  std::map<std::string, int> cfg{{"packet_based", 0}};

  auto vvc = VvcFactory::create(VvcTypes::AXI_STREAM, VvcChannel::NA, 0, cfg);
  REQUIRE(dynamic_cast<VvcByteStream*>(vvc.get()) != nullptr);
}

TEST_CASE("VvcFactory_packet_based_UART_still_byte_stream")
{
  // VvcPacketStreamBased rejects UART; the factory currently still respects the
  // packet_based flag for UART which would propagate the throw. Pin down the
  // behavior: with packet_based=1 on UART, factory call propagates the throw.
  std::map<std::string, int> cfg{{"packet_based", 1}};
  REQUIRE_THROWS_AS(VvcFactory::create(VvcTypes::UART, VvcChannel::NA, 0, cfg),
                    std::invalid_argument);
}

TEST_CASE("VvcFactory_mem_mapped_types")
{
  std::map<std::string, int> cfg{};

  for (auto t : {VvcTypes::AXI, VvcTypes::AXILITE, VvcTypes::AVALON_MM}) {
    auto vvc = VvcFactory::create(t, VvcChannel::NA, 0, cfg);
    REQUIRE(dynamic_cast<VvcMemMapped*>(vvc.get())  != nullptr);
    REQUIRE(dynamic_cast<VvcStreamBased*>(vvc.get()) == nullptr);
    REQUIRE(vvc->vvc_type == t);
  }
}

TEST_CASE("VvcFactory_cosim_support_flag")
{
  std::map<std::string, int> cfg_on { {"cosim_support", 1} };
  std::map<std::string, int> cfg_off{ {"cosim_support", 0} };
  std::map<std::string, int> cfg_absent{};

  auto v_on     = VvcFactory::create(VvcTypes::UART, VvcChannel::NA, 0, cfg_on);
  auto v_off    = VvcFactory::create(VvcTypes::UART, VvcChannel::NA, 0, cfg_off);
  auto v_absent = VvcFactory::create(VvcTypes::UART, VvcChannel::NA, 0, cfg_absent);

  REQUIRE(v_on->cosim_support     == true);
  REQUIRE(v_off->cosim_support    == false);
  REQUIRE(v_absent->cosim_support == false);
}

TEST_CASE("VvcFactory_strips_well_known_keys_from_bfm_cfg")
{
  std::map<std::string, int> cfg{
    {"cosim_support", 1},
    {"packet_based",  1},
    {"baudrate",      115200},
    {"timeout",       0},
  };

  auto vvc = VvcFactory::create(VvcTypes::AXI_STREAM, VvcChannel::NA, 0, cfg);

  // packet_based and cosim_support are consumed by the factory, not stored.
  REQUIRE_FALSE(vvc->bfm_cfg.contains("cosim_support"));
  REQUIRE_FALSE(vvc->bfm_cfg.contains("packet_based"));

  // Everything else passes through.
  REQUIRE(vvc->bfm_cfg.at("baudrate") == 115200);
  REQUIRE(vvc->bfm_cfg.at("timeout")  == 0);
  REQUIRE(vvc->bfm_cfg.size() == 2);
}

TEST_CASE("VvcFactory_instance_id_and_channel_propagated")
{
  std::map<std::string, int> cfg{};

  auto vvc = VvcFactory::create(VvcTypes::UART, VvcChannel::TX, 42, cfg);
  REQUIRE(vvc->vvc_instance_id == 42);
  REQUIRE(vvc->vvc_channel     == VvcChannel::TX);
}

TEST_CASE("VvcFactory_string_overload_dispatches_correctly")
{
  std::map<std::string, int> cfg{{"packet_based", 1}};

  auto vvc = VvcFactory::create("AXISTREAM_VVC", "RX", 3, cfg);

  REQUIRE(dynamic_cast<VvcPacketStreamBased*>(vvc.get()) != nullptr);
  REQUIRE(vvc->vvc_channel     == VvcChannel::RX);
  REQUIRE(vvc->vvc_instance_id == 3);
}

TEST_CASE("VvcFactory_string_overload_throws_on_unknown_type")
{
  std::map<std::string, int> cfg{};
  REQUIRE_THROWS_AS(VvcFactory::create("BANANAS_VVC", "NA", 0, cfg),
                    std::invalid_argument);
}

TEST_CASE("VvcFactory_enum_overload_throws_on_unknown_VvcTypes")
{
  // The switch in VvcFactory::create has a default arm that throws.  All
  // declared VvcTypes enum values are covered by the switch, so the default
  // can only be reached with an out-of-range value cast into the enum.
  std::map<std::string, int> cfg{};
  auto bogus = static_cast<VvcTypes>(999);
  REQUIRE_THROWS_AS(VvcFactory::create(bogus, VvcChannel::NA, 0, cfg),
                    std::invalid_argument);
}
