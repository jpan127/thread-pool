#pragma once

#include <memory>
#include <optional>
#include <type_traits>

namespace tp {

namespace details {

template <typename Tuple, size_t ... Indices>
auto transform_tuple_strip_first(const Tuple &tuple, std::index_sequence<Indices...>) {
    return std::make_tuple(std::get<1 + Indices>(tuple)...);
}

template <typename Tuple>
auto transform_tuple_strip_first(const Tuple &tuple) {
    constexpr size_t kSize = std::tuple_size<Tuple>::value;
    return transform_tuple_strip_first(tuple, std::make_index_sequence<kSize - 1>());
}

} // namespace details

class thread {
  public:
    /// Generic callback
    using Callback = std::function<void()>;

    /// Thread parameters
    struct Params {
        std::optional<size_t> stack_size;
        bool print_errors = false;
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
        construct(std::forward<Callable>(callable), std::forward<Args>(args)...);
        create({});
    }

    /// Constructible with parameters
    template <typename Callable, typename ... Args>
    explicit thread(const Params &params, Callable &&callable, Args && ... args)
        : print_errors_(params.print_errors) {
        construct(std::forward<Callable>(callable), std::forward<Args>(args)...);
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
    const bool print_errors_ = false;

    /// Thread handle
    std::optional<size_t> handle_;

    /// Thread ID
    std::optional<size_t> id_;

    /// Parameters to pass to thread
    std::unique_ptr<ThreadParams> thread_params_ = std::make_unique<ThreadParams>();

    /// Wraps the callback in a function that is compliant with pthread
    static void *thread_wrapper(void *params);

    /// Set up thread with the callable and arguments
    template <typename Callable, typename ... Args>
    void construct(Callable &&callable, Args && ... args) noexcept {
        constexpr bool kIsFreeFunction = !std::is_member_function_pointer_v<Callable>;
        constexpr auto kSize = sizeof...(args);
        static_assert(kIsFreeFunction || kSize > 0, "If member function, object must be an arg");

        if constexpr (kIsFreeFunction) {
            thread_params_->run_callback = [callable = std::forward<Callable>(callable),
                                            args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                std::apply([&](auto && ... args) {
                    callable(std::forward<Args>(args)...);
                }, std::move(args));
            };
        } else {
            using FirstArgType = std::tuple_element_t<0, std::tuple<Args...>>;
            static_assert(std::is_pointer_v<FirstArgType> || std::is_reference_v<FirstArgType>);

            auto all_tuple = std::make_tuple(std::forward<Args>(args)...);
            auto object = std::get<0>(all_tuple);

            auto arg_tuple = details::transform_tuple_strip_first(all_tuple);
            thread_params_->run_callback = [object,
                                            callable = std::forward<Callable>(callable),
                                            args = std::move(arg_tuple)] {
                std::apply([&](auto && ... args) {
                    std::mem_fn(callable)(object, std::forward<decltype(args)>(args)...);
                }, std::move(args));
            };
        }
    }

    /// Creates the thread
    void create(const Params &params) noexcept;
};

} // namespace tp
