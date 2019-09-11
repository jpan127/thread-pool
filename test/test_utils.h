#pragma once

#include <atomic>
#include <chrono>
#include <thread>

template <typename Integral>
void free_function(Integral &count) {
    for (auto i = 0; i < 100; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    count++;
}

class Class {
  public:
    size_t count = 0;
    void function() {
        free_function(count);
    }
};

struct Functor {
    static inline std::atomic<size_t> count = 0;
    void operator()() {
        free_function(count);
    }
};

namespace {

auto lambda = [](auto &count) { free_function(count); };

auto poll = [](auto &alive) {
    while (alive) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
};

} // namespace
