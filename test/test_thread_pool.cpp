#include "thread_pool/thread_pool.h"
#include "test_utils.h"

#include "catch.hpp"

#include <array>

#define TEMPLATE_TYPES_UNDER_TEST std::thread , tp::thread

TEMPLATE_TEST_CASE("thread_pool::DefaultConstructible", "[thread_pool]", TEMPLATE_TYPES_UNDER_TEST) {
    tp::thread_pool<TestType> tp({});
}

TEMPLATE_TEST_CASE("thread_pool::100ThreadsConstruction", "[thread_pool]", TEMPLATE_TYPES_UNDER_TEST) {
    tp::thread_pool<TestType> tp({.size = 100});
}

TEMPLATE_TEST_CASE("thread_pool::1000ThreadsConstruction", "[thread_pool]", TEMPLATE_TYPES_UNDER_TEST) {
    tp::thread_pool<TestType> tp({.size = 1000});
}

TEMPLATE_TEST_CASE("thread_pool::10000ThreadsConstruction", "[thread_pool]", TEMPLATE_TYPES_UNDER_TEST) {
    tp::thread_pool<TestType> tp({.size = 10000});
}

TEMPLATE_TEST_CASE("thread_pool::Work", "[thread_pool]", TEMPLATE_TYPES_UNDER_TEST) {
    constexpr size_t kNumTasks = 1000;
    constexpr size_t kPoolSize = 100;

    tp::thread_pool<TestType> tp({.size = kPoolSize});

    SECTION("FreeFunction") {
        std::atomic<size_t> count = 0;
        for (size_t ii = 0; ii < kNumTasks; ii++) {
            tp.push(std::bind(free_function<decltype(count)>, std::ref(count)));
        }
        tp.join(true);
        REQUIRE(count == kNumTasks);
    }

    SECTION("Class") {
        std::array<Class, kNumTasks> classes{};
        for (size_t ii = 0; ii < kNumTasks; ii++) {
            tp.push(&Class::function, &classes[ii]);
        }
        tp.join(true);
        for (size_t ii = 0; ii < kNumTasks; ii++) {
            REQUIRE(classes[ii].count == 1);
        }
    }

    SECTION("Functor") {
        Functor::count = 0;
        for (size_t ii = 0; ii < kNumTasks; ii++) {
            tp.push(Functor{});
        }
        tp.join(true);
        REQUIRE(Functor::count == kNumTasks);
    }

    SECTION("Lambda") {
        std::atomic<size_t> count = 0;
        for (size_t ii = 0; ii < kNumTasks; ii++) {
            tp.push([&count] { lambda(count); });
        }
        tp.join(true);
        REQUIRE(count == kNumTasks);
    }
}

TEMPLATE_TEST_CASE("thread_pool::OneBurst", "[thread_pool]", TEMPLATE_TYPES_UNDER_TEST) {
    constexpr size_t kNumTasks = 1000;
    constexpr size_t kPoolSize = 100;

    tp::thread_pool<TestType> tp({.size = kPoolSize});

    std::atomic<size_t> count = 0;
    for (size_t ii = 0; ii < kNumTasks; ii++) {
        tp.push([&count] { free_function(count); });
    }

    SECTION("EmptyQueue") {
        tp.join(true);
        REQUIRE(count == kNumTasks);
    }

    SECTION("DontEmptyQueue") {
        tp.join(false);
        REQUIRE(count < kNumTasks);
    }
}

TEMPLATE_TEST_CASE("thread_pool::RepeatedBursts", "[thread_pool]", TEMPLATE_TYPES_UNDER_TEST) {
    constexpr size_t kNumTasks = 1000;
    constexpr size_t kPoolSize = 100;

    tp::thread_pool<TestType> tp({.size = kPoolSize});

    std::atomic<size_t> count = 0;
    for (size_t round = 0; round < (kNumTasks / kPoolSize); round++) {
        for (size_t ii = 0; ii < kPoolSize; ii++) {
            tp.push([&count] { free_function(count); });
        }
    }

    SECTION("EmptyQueue") {
        tp.join(true);
        REQUIRE(count == kNumTasks);
    }

    SECTION("DontEmptyQueue") {
        tp.join(false);
        REQUIRE(count < kNumTasks);
    }
}

TEMPLATE_TEST_CASE("thread_pool::Future", "[thread_pool]", TEMPLATE_TYPES_UNDER_TEST) {
    std::atomic<bool> signal{false};
    auto wait_for_signal = [&signal] {
        while (!signal) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };

    tp::thread_pool<TestType> tp({.size = 1});
    auto future = tp.push(wait_for_signal);

    REQUIRE(std::future_status::timeout == future.wait_for(std::chrono::milliseconds(10)));

    signal = true;

    REQUIRE(std::future_status::ready == future.wait_for(std::chrono::milliseconds(2)));
    future.get();
}
