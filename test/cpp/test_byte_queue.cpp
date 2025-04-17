#include <catch2/catch_test_macros.hpp>
#include <vector>
#include "byte_queue.hpp"

using namespace uvvm_cosim;

TEST_CASE("ByteQueue_empty_and_size")
{
  INFO("ByteQueue_empty_and_size test start.");

  ByteQueue q;

  // Empty initially
  REQUIRE(q.empty() == true);
  REQUIRE(q.size() == 0);

  q.put(0x00);
  REQUIRE(q.empty() == false);
  REQUIRE(q.size() == 1);
  (void) q.get();
  REQUIRE(q.empty() == true);
  REQUIRE(q.size() == 0);

  std::vector<uint8_t> v {0xAA, 0xBB, 0xCC, 0xDD};
  q.put(v);
  REQUIRE(q.empty() == false);
  REQUIRE(q.size() == 4);
  (void) q.get(4);
  REQUIRE(q.empty() == true);
  REQUIRE(q.size() == 0);

  q.put(v);
  (void) q.get(2);
  REQUIRE(q.empty() == false);
  REQUIRE(q.size() == 2);
  (void) q.get();
  REQUIRE(q.empty() == false);
  REQUIRE(q.size() == 1);
  (void) q.get(1);
  REQUIRE(q.empty() == true);
  REQUIRE(q.size() == 0);
}

TEST_CASE("ByteQueue_put_and_get")
{
  INFO("ByteQueue_empty_and_size test start.");

  ByteQueue q;

  std::vector<uint8_t> v1 {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  // Put and get a whole vector by specifying size
  q.put(v1);

  std::vector<uint8_t> v2 = q.get(v1.size());

  REQUIRE(v1.size() == v2.size());
  REQUIRE(v1 == v2);

  // Put and get a whole vector getting all bytes available
  q.put(v1);
  v2 = q.get(0); // num_bytes=0 --> get all bytes

  REQUIRE(v1.size() == v2.size());
  REQUIRE(v1 == v2);

  // Put and get slices of vector
  q.put(v1);

  v2 = q.get(4);

  REQUIRE(v2.size() == 4);
  REQUIRE(std::equal(v2.begin(), v2.end(), v1.begin()));

  v2 = q.get(6);

  REQUIRE(v2.size() == 6);
  REQUIRE(std::equal(v2.begin(), v2.end(), v1.begin()+4));

  // Get individual bytes
  q.put(v1);
  
  for (int i=0; i < v1.size(); i++) {
    std::optional<uint8_t> byte = q.get();

    REQUIRE(byte.has_value() == true);
    REQUIRE(byte.value() == v1[i]);
  }

  // Get on empty queue - optional should contain no value
  REQUIRE(q.empty());
  std::optional<uint8_t> byte = q.get();
  REQUIRE(byte.has_value() == false);

  // Put bytes, get vector
  for (int i=0; i < v1.size(); i++) {
    q.put((uint8_t) i);
  }
  REQUIRE(q.size() == 10);
  v2 = q.get(10);

  REQUIRE(v2.size() == 10);
  REQUIRE(v1 == v2);
}
