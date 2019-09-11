#include "thread_pool/thread_pool.h"

namespace tp {

thread_pool::thread_pool(const Params &params) noexcept {
    for (size_t t = 0; t < params.size; t++) {
        threads_.push_back(thread(params.thread_params, &thread_pool::worker, this));
    }
}

thread_pool::~thread_pool() noexcept {
    join();
}

void thread_pool::join(const bool finish_queue) noexcept {
    // Block until queue is empty
    if (finish_queue) {
        std::unique_lock lock(lock_);
        if (!q_.empty()) {
            q_pop_notifier_.wait(lock, [this] { return q_.empty(); });
        }
    }

    join();
}

size_t thread_pool::size() const noexcept {
    return threads_.size();
}

size_t thread_pool::qsize() const noexcept {
    std::scoped_lock lock(lock_);
    return q_.size();
}

void thread_pool::join() noexcept {
    kill_ = true;
    q_push_notifier_.notify_all();
    for (auto &thread : threads_) {
        thread.join();
    }
}

void thread_pool::worker() noexcept {
    auto dequeue = [this] {
        Task task{};

        std::scoped_lock lock(lock_);
        if (!q_.empty()) {
            task = std::move(q_.front());
            q_.pop();
            if (q_.empty()) {
                q_pop_notifier_.notify_one();
            }
        }

        return task;
    };

    auto wait = [this] {
        std::unique_lock lock(lock_);

        // Don't wait if there is more to dequeue
        if (!q_.empty()) {
            return;
        }

        q_push_notifier_.wait(lock, [this] { return !q_.empty() || kill_; });
    };

    while (!kill_) {
        auto task = dequeue();
        if (task.valid()) {
            task();
        }

        wait();
    }
}

} // namespace tp
