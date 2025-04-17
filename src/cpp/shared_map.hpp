#pragma once
#include <mutex>
#include <map>

// Based on this StackOverflow answer to the question
// "What's the proper way to associate a mutex with its data?"
// https://stackoverflow.com/a/15845911
//
// Trimmed away most of it, leaving only operator() so we can
// execute a function that works on the vector, but waiting for
// a lock so two threads won't access the vector simultaneously.

// K: Key, V: Value, C: Comparator clas
template <typename K, typename V, typename C> class shared_map {
  mutable std::mutex mtx;
  mutable std::map<K, V, C> mp;

public:
  template <typename F> auto operator()(F f) const -> decltype(f(mp)) {
    return std::lock_guard<std::mutex>(mtx), f(mp);
  }
};
