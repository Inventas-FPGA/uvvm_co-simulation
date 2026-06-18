#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <string>
#include "vvc/vvc_base.hpp"

using namespace uvvm_cosim;

TEST_CASE("VvcTypes_to_string_and_back")
{
  INFO("Round-trip every VvcTypes enum value through to_string / to_VvcTypes.");

  const VvcTypes all[] = {
    VvcTypes::AXI_STREAM,
    VvcTypes::AXI,
    VvcTypes::AXILITE,
    VvcTypes::AVALON_ST,
    VvcTypes::AVALON_MM,
    VvcTypes::UART,
  };

  for (auto t : all) {
    std::string s = to_string(t);
    REQUIRE(s != "UNKNOWN_VVC");
    REQUIRE(to_VvcTypes(s) == t);
  }
}

TEST_CASE("VvcTypes_to_VvcTypes_throws_on_garbage")
{
  REQUIRE_THROWS_AS(to_VvcTypes("DEFINITELY_NOT_A_VVC"), std::invalid_argument);
  REQUIRE_THROWS_AS(to_VvcTypes(""),                     std::invalid_argument);
}

TEST_CASE("VvcChannel_to_string_and_back")
{
  INFO("Round-trip every VvcChannel enum value.");

  REQUIRE(to_string(VvcChannel::NA) == "NA");
  REQUIRE(to_string(VvcChannel::RX) == "RX");
  REQUIRE(to_string(VvcChannel::TX) == "TX");

  REQUIRE(to_VvcChannel("NA") == VvcChannel::NA);
  REQUIRE(to_VvcChannel("RX") == VvcChannel::RX);
  REQUIRE(to_VvcChannel("TX") == VvcChannel::TX);
}

TEST_CASE("VvcChannel_unknown_string_falls_back_to_NA")
{
  // Current behavior of to_VvcChannel: unknown strings return NA (no throw).
  // Pin this down so a refactor doesn't silently change it.
  REQUIRE(to_VvcChannel("ABC") == VvcChannel::NA);
  REQUIRE(to_VvcChannel("")    == VvcChannel::NA);
}

TEST_CASE("VvcIdentifier_to_string_contains_all_fields")
{
  VvcIdentifier id{VvcTypes::UART, VvcChannel::TX, 7};
  std::string s = to_string(id);

  REQUIRE(s.find("UART_VVC")     != std::string::npos);
  REQUIRE(s.find("TX")           != std::string::npos);
  REQUIRE(s.find("instance_id=") != std::string::npos);
  REQUIRE(s.find("7")            != std::string::npos);
}

TEST_CASE("VvcTypes_to_string_unknown_value_returns_UNKNOWN_VVC")
{
  // The switch in to_string(VvcTypes) has a default arm that returns
  // "UNKNOWN_VVC".  It only fires for enum values outside the declared set,
  // which can only be produced by casting an arbitrary int into the enum.
  auto bogus = static_cast<VvcTypes>(999);
  REQUIRE(to_string(bogus) == "UNKNOWN_VVC");
}

TEST_CASE("VvcChannel_to_string_unknown_value_returns_NA")
{
  // Default arm of to_string(VvcChannel) returns "NA" for unknown values.
  auto bogus = static_cast<VvcChannel>(42);
  REQUIRE(to_string(bogus) == "NA");
}

TEST_CASE("VvcTypes_and_VvcChannel_strict_ordering")
{
  // operator< on enums is used by VvcCompare; smoke-check transitivity-friendly
  // ordering matches the underlying integer order.
  REQUIRE(VvcTypes::AXI_STREAM < VvcTypes::UART);
  REQUIRE_FALSE(VvcTypes::UART < VvcTypes::AXI_STREAM);

  REQUIRE(VvcChannel::NA < VvcChannel::RX);
  REQUIRE(VvcChannel::RX < VvcChannel::TX);
  REQUIRE_FALSE(VvcChannel::TX < VvcChannel::NA);
}
