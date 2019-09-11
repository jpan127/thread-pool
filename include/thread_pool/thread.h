#pragma once

#include "thread_pool/utils.h"

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>

namespace tp {

class thread {
  public:
    /// Generic callback
    using Callback = std::function<void()>;

    /// Thread parameters
    struct Params {
        std::optional<size_t> stack_size;
        bool print_errors = true;
    };

    /// Check how many processors there are
    static size_t hardware_concurrency();

    /// Default constructible
    thread() = default;

    /// Movable
    thread(thread &&other) noexcept;
    thread &operator=(thread &&other) noexcept;

    /// Non copyable
    thread(const thread &other) = delete;
    thread &operator=(const thread &other) = delete;

    /// Constructible
    template <typename Callable, typename ... Args>
    explicit thread(Callable &&callable, Args && ... args) {
        thread_params_->run_callback = details::construct(std::forward<Callable>(callable),
                                                          std::forward<Args>(args)...);
        create({});
    }

    /// Constructible with parameters
    template <typename Callable, typename ... Args>
    explicit thread(const Params &params, Callable &&callable, Args && ... args)
        : print_errors_(params.print_errors) {
        thread_params_->run_callback = details::construct(std::forward<Callable>(callable),
                                                          std::forward<Args>(args)...);
        create(params);
    }

    /// Terminate if thread is still alive
    ~thread() noexcept;

    /// Check if thread has been spawned
    bool joinable() const noexcept;

    /// Get thread ID
    size_t get_id() const;

    /// Get pthread handle
    size_t native_handle() const;

    /// Join thread if spawned
    void join() noexcept;

    /// Detaches the thread, this object relinquishes ownership
    void detach() noexcept;

  private:
    using StoreIdCallback = std::function<void(const size_t)>;

    /// Callbacks for the thread wrapper to execute
    struct ThreadParams {
        Callback run_callback{};
        StoreIdCallback store_id_callback{};
    };

    /// To print errors or not
    const bool print_errors_ = true;

    /// Thread handle
    std::optional<size_t> handle_;

    /// Thread ID
    class Id;
    std::shared_ptr<Id> id_;

    /// Parameters to pass to thread
    std::unique_ptr<ThreadParams> thread_params_ = std::make_unique<ThreadParams>();

    /// Wraps the callback in a function that is compliant with pthread
    static void *thread_wrapper(void *params);

    /// Creates the thread
    void create(const Params &params) noexcept;
};

} // namespace tp
