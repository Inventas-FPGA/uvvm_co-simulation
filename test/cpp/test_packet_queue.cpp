#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cstdlib>
#include <vector>
#include "packet_queue.hpp"

using namespace uvvm_cosim;

TEST_CASE("PacketQueue_empty_and_size")
{
  INFO("PacketQueue_empty_and_size test start.");

  PacketQueue q;

  INFO("Check empty/size initially");
  REQUIRE(q.empty());
  REQUIRE(q.size() == 0);

  INFO("Check empty/size after put_pkt and after get_pkt");
  q.put_pkt({0, 1, 2, 3, 4});
  REQUIRE_FALSE(q.empty());
  REQUIRE(q.size() == 1);
  (void) q.get_pkt();
  REQUIRE(q.empty());
  REQUIRE(q.size() == 0);

  INFO("Put individual bytes in pkt buffer without completing packet");
  q.put_byte(0x01, false);
  q.put_byte(0x02, false);
  REQUIRE(q.empty());
  REQUIRE(q.size() == 0);

  INFO("Complete packet and expect queue to be not empty");
  q.put_byte(0x03, true);
  REQUIRE_FALSE(q.empty());
  REQUIRE(q.size() == 1);

  INFO("Get some bytes from packet. Queue should remain non-empty until whole packet is read out");
  (void) q.get_byte();
  (void) q.get_byte();
  REQUIRE_FALSE(q.empty());
  REQUIRE(q.size() == 1);

  INFO("Get last byte in packet. Queue should go empty.");
  (void) q.get_byte();
  REQUIRE(q.empty());
  REQUIRE(q.size() == 0);

  INFO("Put a packet using put_byte() method, get with get_pkt(), queue should go empty again.");
  q.put_byte(0x01, false);
  q.put_byte(0x02, true);
  REQUIRE_FALSE(q.empty());
  (void) q.get_pkt();
  REQUIRE(q.empty());
  REQUIRE(q.size() == 0);
}

TEST_CASE("PacketQueue_put_and_get_pkt")
{
  INFO("PacketQueue_put_and_get_pkt test start.");

  PacketQueue q;

  constexpr int NUM_PACKETS = 10;
  constexpr int PKT_SIZE_MAX = 100;

  std::array<std::vector<uint8_t>, NUM_PACKETS> packets;

  srand(1234); // using constant seed

  INFO("Put random packets in queue.");
  for (int i = 0; i < NUM_PACKETS; i++) {
    int pkt_size = (rand() % PKT_SIZE_MAX) + 1;

    for (int j = 0; j < pkt_size; j++) {
      packets[i].push_back(rand() % 256);
    }

    q.put_pkt(packets[i]);
    REQUIRE_FALSE(q.empty());
    REQUIRE(q.size() == i+1);
  }

  INFO("Read out packets and check packet contents.");
  for (int i = 0; i < NUM_PACKETS; i++) {
    REQUIRE_FALSE(q.empty());
    REQUIRE(q.size() == NUM_PACKETS-i);
    
    std::vector<uint8_t> pkt = q.get_pkt();

    REQUIRE(pkt == packets[i]);
  }

  REQUIRE(q.empty());
  REQUIRE(q.size() == 0);
}

TEST_CASE("PacketQueue_put_and_get_byte")
{
  INFO("PacketQueue_put_and_get_byte test start.");

  PacketQueue q;

  constexpr int NUM_PACKETS = 10;
  constexpr int PKT_SIZE_MAX = 100;

  std::array<std::vector<uint8_t>, NUM_PACKETS> packets;

  srand(1234); // using constant seed

  INFO("Put random packets in queue one byte at a time.");
  for (int i = 0; i < NUM_PACKETS; i++) {
    int pkt_size = (rand() % PKT_SIZE_MAX) + 1;

    for (int j = 0; j < pkt_size; j++) {
      packets[i].push_back(rand() % 256);

      if (j+1 == pkt_size) {
	q.put_byte(packets[i].back(), true); // Last byte in pkt
      } else {
	q.put_byte(packets[i].back(), false);
      }
    }

    REQUIRE_FALSE(q.empty());
    REQUIRE(q.size() == i+1);
  }

  INFO("Read out packets from queue one byte at a time and check packet contents.");
  for (int i = 0; i < NUM_PACKETS; i++) {
    REQUIRE_FALSE(q.empty());
    REQUIRE(q.size() == NUM_PACKETS-i);

    std::vector<uint8_t> pkt;
    bool pkt_done = false;

    do {
      std::optional<std::pair<uint8_t, bool>> byte = q.get_byte();
      REQUIRE(byte.has_value());
      pkt.push_back(byte.value().first);
      pkt_done = byte.value().second;
    } while (!pkt_done);

    REQUIRE(pkt == packets[i]);
  }

  REQUIRE(q.empty());
  REQUIRE(q.size() == 0);

  INFO("Check that get_byte() on empty queue returns empty optional.");
  std::optional<std::pair<uint8_t, bool>> byte;
  byte = q.get_byte();

  REQUIRE_FALSE(byte.has_value());
}

TEST_CASE("PacketQueue_mixed")
{
  INFO("PacketQueue_mixed test start.");

  PacketQueue q;

  // Different "modes" to test.
  // FULL_PKT and BYTES_ONLY is used with put_byte()/put_pkt(),
  // while MIXED mode is also used with get_byte()/get_pkt() in the test below.
  enum pkt_queue_mode_t {
    FULL_PKT,   // Use put_pkt() or get_pkt() for full packet
    BYTES_ONLY, // Use put_byte() or get_byte() to put/get full packet
    MIXED       // Use get_byte() followed by get_pkt().
  };

  // NOTE:
  // Mixed mode is only used with get_byte()/get_pkt().  Since
  // it's possible to get a few bytes of a packet and then retrieve
  // the remaining packet with get_pkt.  But it's not possible to put
  // a few bytes of a packet with put_byte() and then complete it with
  // put_pkt(). In that case put_byte() would just start a packet in
  // an internal buffer in PacketQueue, will put_pkt() will put an
  // independent packet first instead of completing the packet started
  // by put_byte().

  // The rationale for how this is implemented is that only complete
  // packets that have been put in the queue should count towards
  // size() and empty().
  // The way it's used in the cosim code is that the JSON-RPC side will only put and get full packets, while the VHDL side will only put and get individual bytes.
  // We don't want the JSON-RPC side to fetch incomplete packets.
  // But we allow the VHDL side to nibble away at a packet one byte at a time without affecting size()/empty status until it has read the last byte of a packet.

  constexpr int NUM_PACKETS = 20;
  constexpr int PKT_SIZE_MIN = 10;
  constexpr int PKT_SIZE_MAX = 100;

  std::array<std::vector<uint8_t>, NUM_PACKETS> packets;

  pkt_queue_mode_t mode = FULL_PKT;

  srand(1234); // using constant seed

  INFO("Put random packets in queue in different ways (full pkt, byte at a time, mixed).");
  for (int i = 0; i < NUM_PACKETS; i++) {
    int pkt_size = (rand() % (PKT_SIZE_MAX + 1 - PKT_SIZE_MIN)) + PKT_SIZE_MIN;

    // Used only with mode == MIXED
    std::vector<uint8_t> pkt_second_half;

    for (int j = 0; j < pkt_size; j++) {
      packets[i].push_back(rand() % 256);

      if (mode == BYTES_ONLY) {
	if (j+1 == pkt_size) {
	  q.put_byte(packets[i].back(), true); // Last byte in pkt
	} else {
	  q.put_byte(packets[i].back(), false);
	}
      } else if (mode == MIXED) {
	// Read individual bytes of first half of packet,
	// read out the second half of packet in one go as vector
	if (i < pkt_size/2) {
	  q.put_byte(packets[i].back(), false);
	} else {
	  pkt_second_half.push_back(packets[i].back());
	}
      }
    }

    if (mode == FULL_PKT) {
      q.put_pkt(packets[i]);
    } else if (mode == MIXED) {
      q.put_pkt(pkt_second_half);
    }

    REQUIRE_FALSE(q.empty());
    REQUIRE(q.size() == i+1);

    // Iterate through FULL_PKT and BYTES_ONLY mode.
    mode = (mode == FULL_PKT ? BYTES_ONLY : FULL_PKT);
  }

  INFO("Read out packets from queue in different ways (full pkt, byte at a time, mixed).");

  mode = MIXED; // Start with different mode
  
  for (int i = 0; i < NUM_PACKETS; i++) {
    REQUIRE_FALSE(q.empty());
    REQUIRE(q.size() == NUM_PACKETS-i);

    std::vector<uint8_t> pkt; // Used for mode == BYTES_ONLY or mode == MIXED

    if (mode == FULL_PKT) {
      pkt = q.get_pkt();

    } else if (mode == BYTES_ONLY) {
      bool pkt_done = false;
      do {
	std::optional<std::pair<uint8_t, bool>> byte = q.get_byte();
	REQUIRE(byte.has_value());
	pkt.push_back(byte.value().first);
	pkt_done = byte.value().second;
      } while (!pkt_done);

    } else if (mode == MIXED) {
      for (int i = 0; i < packets[i].size() / 2; i++) {
	std::optional<std::pair<uint8_t, bool>> byte = q.get_byte();
	REQUIRE(byte.has_value());
	REQUIRE_FALSE(byte.value().second); // Not expecting end-of-packet flag
	pkt.push_back(byte.value().first);
      }

      std::vector<uint8_t> pkt_second_half = q.get_pkt();
      pkt.insert(pkt.end(), pkt_second_half.begin(), pkt_second_half.end());

      // Iterate through the three mode types
      mode = (mode == FULL_PKT   ? BYTES_ONLY :
	      mode == BYTES_ONLY ? MIXED      : FULL_PKT);
    }

    REQUIRE(pkt == packets[i]);
  }

  REQUIRE(q.empty());
  REQUIRE(q.size() == 0);
}
