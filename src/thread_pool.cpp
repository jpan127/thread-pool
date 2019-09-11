#include "thread_pool/thread_pool.h"

namespace tp::details {

template <typename ThreadType>
thread_pool_base<ThreadType>::~thread_pool_base() noexcept {
    join();
}

template <typename ThreadType>
void thread_pool_base<ThreadType>::join(const bool finish_queue) noexcept {
    // Block until queue is empty
    if (finish_queue) {
        std::unique_lock lock(lock_);
        if (!q_.empty()) {
            q_pop_notifier_.wait(lock, [this] { return q_.empty(); });
        }
    }

    join();
}

template <typename ThreadType>
size_t thread_pool_base<ThreadType>::size() const noexcept {
    return threads_.size();
}

template <typename ThreadType>
size_t thread_pool_base<ThreadType>::qsize() const noexcept {
    std::scoped_lock lock(lock_);
    return q_.size();
}

template <typename ThreadType>
void thread_pool_base<ThreadType>::join() noexcept {
    kill_ = true;
    q_push_notifier_.notify_all();
    for (auto &thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

template <typename ThreadType>
void thread_pool_base<ThreadType>::worker() noexcept {
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

/// Explicitly instantiate only these 2 acceptable implementations of thread
template class thread_pool_base<std::thread>;
template class thread_pool_base<tp::thread>;

} // namespace tp::details
