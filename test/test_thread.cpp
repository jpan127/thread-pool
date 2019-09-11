#include "thread_pool/thread.h"
#include "test_utils.h"

#include "catch.hpp"

using namespace tp;

TEST_CASE("thread::DefaultConstructible", "[thread]") {
    thread t;
}

TEST_CASE("thread::Movable", "[thread]") {
    thread t1;
    REQUIRE(!t1.joinable());

    thread t2([] {});
    REQUIRE(t2.joinable());

    t1 = std::move(t2);
    REQUIRE(t1.joinable());
    REQUIRE(!t2.joinable());

    t1.join();
}

TEST_CASE("thread::WithParameters", "[thread]") {
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

TEST_CASE("thread::HardwareConcurrency", "[thread]") {
    REQUIRE(thread::hardware_concurrency() == std::thread::hardware_concurrency());
}

TEST_CASE("thread::Work", "[thread]") {
    SECTION("FreeFunction") {
        std::atomic<size_t> count = 0;
        thread t(std::bind(free_function<decltype(count)>, std::ref(count)));

        REQUIRE(t.joinable());

        t.join();
        REQUIRE(count == 1);
        REQUIRE(!t.joinable());
    }

    SECTION("Class") {
        Class obj;
        thread t(&Class::function, &obj);

        REQUIRE(t.joinable());

        t.join();
        REQUIRE(obj.count == 1);
        REQUIRE(!t.joinable());
    }

    SECTION("Functor") {
        Functor::count = 0;
        thread t(Functor{});

        REQUIRE(t.joinable());

        t.join();
        REQUIRE(Functor::count == 1);
        REQUIRE(!t.joinable());
    }

    SECTION("Lambda") {
        size_t count = 0;
        thread t([&count] { lambda(count); });

        REQUIRE(t.joinable());

        t.join();
        REQUIRE(count == 1);
        REQUIRE(!t.joinable());
    }
}

TEST_CASE("thread::100Threads", "[thread]") {
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

TEST_CASE("thread::IdAndHandle", "[thread]") {
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
