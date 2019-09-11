#pragma once

#include "thread_pool/thread.h"

#include <atomic>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace tp {

namespace details {

template <typename ThreadType>
class thread_pool_base {
  public:
    /// Constructor
    thread_pool_base() = default;

    /// Destructor
    virtual ~thread_pool_base() noexcept;

    /// Push a task to the task queue
    template <typename Callable, typename ... Args>
    std::future<void> push(Callable &&callable, Args && ... args) noexcept {
        // Create task
        auto callback = details::construct(std::forward<Callable>(callable),
                                           std::forward<Args>(args)...);
        Task task(std::move(callback));
        auto future = task.get_future();

        // Add to queue and wake up a thread
        std::scoped_lock lock(lock_);
        q_.push(std::move(task));
        q_push_notifier_.notify_one();

        // Future for caller to understand when the task is complete
        return future;
    }

    /// Joins all threads
    /// \param finish_queue To finish the queue before joining or not
    void join(const bool finish_queue) noexcept;

    /// Thread pool size
    size_t size() const noexcept;

    /// Current queue size
    size_t qsize() const noexcept;

  protected:
    /// Pool of threads
    std::vector<ThreadType> threads_;

    /// Worker thread, waits to dequeue tasks from the queue
    void worker() noexcept;

  private:
    using Callback = thread::Callback;
    using Task = std::packaged_task<details::function_type<Callback>::type>;

    /// Flag to stop all threads
    std::atomic<bool> kill_{false};

    /// Lock, protects the queue and the condition variables
    mutable std::mutex lock_;

    /// Condition Variable
    std::condition_variable q_push_notifier_;
    std::condition_variable q_pop_notifier_;

    /// Queue of tasks to execute
    std::queue<Task> q_;

    /// Cancels and joins all threads
    void join() noexcept;
};

} // namespace details

template <typename ThreadType>
class thread_pool;

/// std::thread based thread pool
template <>
class thread_pool<std::thread> : public details::thread_pool_base<std::thread> {
  public:
    /// Parameters
    struct Params {
        size_t size = 1;
    };

    /// Constructor
    thread_pool(const Params &params) noexcept {
        for (size_t t = 0; t < params.size; t++) {
            threads_.push_back(std::thread(&thread_pool::worker, this));
        }
    }

    /// Non movable
    thread_pool(thread_pool &&other) = delete;
    thread_pool &operator=(thread_pool &&other) = delete;

    /// Non copyable
    thread_pool(const thread_pool &other) = delete;
    thread_pool &operator=(const thread_pool &other) = delete;
};

/// tp::thread based thread pool
template <>
class thread_pool<tp::thread> : public details::thread_pool_base<tp::thread> {
  public:
    /// Parameters
    struct Params {
        thread::Params thread_params{};
        size_t size = 1;
    };

    /// Constructor
    thread_pool(const Params &params) noexcept {
        for (size_t t = 0; t < params.size; t++) {
            threads_.push_back(tp::thread(params.thread_params, &thread_pool::worker, this));
        }
    }

    /// Non movable
    thread_pool(thread_pool &&other) = delete;
    thread_pool &operator=(thread_pool &&other) = delete;

    /// Non copyable
    thread_pool(const thread_pool &other) = delete;
    thread_pool &operator=(const thread_pool &other) = delete;
};

} // namespace tp
