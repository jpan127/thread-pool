#include "thread.h"

#include "catch.hpp"

#include <chrono>
#include <thread>

using namespace tp;

TEST_CASE("DefaultConstructible", "thread") {
    thread t;
}

TEST_CASE("Movable", "thread") {
    thread t1;
    REQUIRE(!t1.joinable());

    thread t2([] {});
    REQUIRE(t2.joinable());

    t1 = std::move(t2);
    REQUIRE(t1.joinable());
    REQUIRE(!t2.joinable());

    t1.join();
}

TEST_CASE("WithParameters", "thread") {
    constexpr auto k2MB = 1 << 21;
    thread t(
        {.stack_size = k2MB},
        [] {
            bool flag = false;
            for (auto i = 0; i < 100; i++) {
                flag = !flag;
            }
        });

    t.join();
}

TEST_CASE("HardwareConcurrency", "thread") {
    REQUIRE(thread::hardware_concurrency() == std::thread::hardware_concurrency());
}

TEST_CASE("Lambda", "thread") {
    size_t count = 0;
    thread t(
        [&count] {
            for (auto i = 0; i < 100; i++) {
                count++;
            }
        });

    REQUIRE(t.joinable());

    t.join();
    REQUIRE(count == 100);
    REQUIRE(!t.joinable());
}

void free_function(size_t &count) {
    for (auto i = 0; i < 100; i++) {
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

TEST_CASE("FreeFunction", "thread") {
    size_t count = 0;
    thread t(free_function, std::ref(count));

    REQUIRE(t.joinable());

    t.join();
    REQUIRE(count == 100);
    REQUIRE(!t.joinable());
}

class Foo {
  public:
    size_t count = 0;
    void bar() {
        free_function(count);
    }
};

TEST_CASE("MemberFunction", "thread") {
    Foo foo;
    thread t(&Foo::bar, &foo);

    REQUIRE(t.joinable());

    t.join();
    REQUIRE(foo.count == 100);
    REQUIRE(!t.joinable());
}

struct Functor {
    static inline size_t count = 0;
    void operator()() {
        free_function(count);
    }
};

TEST_CASE("Functor", "thread") {
    thread t(Functor{});

    REQUIRE(t.joinable());

    t.join();
    REQUIRE(Functor::count == 100);
    REQUIRE(!t.joinable());
}

TEST_CASE("100Threads", "thread") {
    auto work = [](std::atomic<size_t> &count) {
        for (auto i = 0; i < 100; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        count++;
    };

    std::atomic<size_t> count;

    SECTION("Joined") {
        std::vector<thread> threads;

        for (size_t ii = 0; ii < 100; ii++) {
            thread t(work, std::ref(count));
            threads.push_back(std::move(t));
        }

        for (size_t ii = 0; ii < 100; ii++) {
            threads[ii].join();
        }

        REQUIRE(count == 100);
    }

    SECTION("Detached") {
        for (size_t ii = 0; ii < 100; ii++) {
            thread t(work, std::ref(count));
            t.detach();
        }

        while (count != 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

TEST_CASE("IdAndHandle", "thread") {
    std::atomic<bool> alive = true;
    auto spin_until_id_and_handle_are_ready = [&alive] {
        while (alive) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };

    thread t(spin_until_id_and_handle_are_ready);

    while (true) {
        try {
            t.get_id();
            t.native_handle();
            break;
        } catch (...) {
            continue;
        }
    }

    alive = false;
    t.join();
}
