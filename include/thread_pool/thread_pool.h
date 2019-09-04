#pragma once

#include "thread_pool/thread.h"

#include <atomic>
#include <future>
#include <mutex>
#include <queue>
#include <vector>

namespace tp {

class thread_pool {
  public:
    /// Parameters
    struct Params {
        thread::Params thread_params{};
        size_t size = 1;
    };

    /// Constructor
    thread_pool(const Params &params) noexcept;

    /// Destructor
    ~thread_pool() noexcept;

    /// Non movable
    thread_pool(thread_pool &&other) = delete;
    thread_pool &operator=(thread_pool &&other) = delete;

    /// Non copyable
    thread_pool(const thread_pool &other) = delete;
    thread_pool &operator=(const thread_pool &other) = delete;

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

  private:
    using Callback = thread::Callback;
    using Task = std::packaged_task<details::function_type<Callback>::type>;

    /// Flag to stop all threads
    std::atomic<bool> kill_{false};

    /// Pool of threads
    std::vector<thread> threads_;

    /// Lock, protects the queue and the condition variables
    mutable std::mutex lock_;

    /// Condition Variable
    std::condition_variable q_push_notifier_;
    std::condition_variable q_pop_notifier_;

    /// Queue of tasks to execute
    std::queue<Task> q_;

    /// Cancels and joins all threads
    void join() noexcept;

    /// Worker thread, waits to dequeu tasks from the queue
    void worker() noexcept;
};

} // namespace tp
