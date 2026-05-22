#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <thread>
#include <vector>
#include "shared_map.hpp"

// shared_map only exposes operator() which runs a callable while holding the
// internal mutex.  The counter_invariant_under_contention test case will
// stress that contract by spawning many threads and verifying invariants
// that can only hold if the mutex is doing its job.

TEST_CASE("shared_map_counter_invariant_under_contention")
{
  // N threads * M iters of read-modify-write on a single counter held in the
  // map.  If the mutex inside shared_map is removed, lost updates due to a
  // classic data race cause the final value to fall below N*M.
  constexpr int N_THREADS = 16;
  constexpr int N_ITERS   = 50000;

  shared_map<int, long long, std::less<int>> m;

  // Initialize the counter at key 0
  m([](auto& mp) { mp[0] = 0; });

  std::vector<std::thread> threads;
  threads.reserve(N_THREADS);

  for (int t = 0; t < N_THREADS; ++t) {
    threads.emplace_back([&m]() {
      for (int i = 0; i < N_ITERS; ++i) {
        // Each operator() call locks, runs the body, unlocks.
        m([](auto& mp) {
          auto& v = mp[0];
          v = v + 1;
        });
      }
    });
  }

  for (auto& th : threads) th.join();

  long long final_value = m([](auto& mp) -> long long { return mp.at(0); });
  REQUIRE(final_value == static_cast<long long>(N_THREADS) * N_ITERS);
}

TEST_CASE("shared_map_operator_returns_lambda_result")
{
  // Smoke-check that operator() forwards the lambda's return value
  // (covered by the counter test too, but explicit here).
  shared_map<int, int, std::less<int>> m;

  m([](auto& mp) { mp[1] = 7; mp[2] = 9; });

  int sum = m([](auto& mp) {
    int s = 0;
    for (auto& [k, v] : mp) s += v;
    return s;
  });
  REQUIRE(sum == 16);

  size_t n = m([](auto& mp) { return mp.size(); });
  REQUIRE(n == 2);
}
