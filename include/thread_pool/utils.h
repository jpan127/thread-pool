#pragma once

#include <functional>
#include <tuple>
#include <utility>

namespace tp::details {

template <typename T>
struct function_type;

template <typename T>
struct function_type<std::function<T>> {
    using type = T;
};

template <typename Tuple, size_t ... Indices>
auto transform_tuple_strip_first(const Tuple &tuple, std::index_sequence<Indices...>) {
    return std::make_tuple(std::get<1 + Indices>(tuple)...);
}

template <typename Tuple>
auto transform_tuple_strip_first(const Tuple &tuple) {
    constexpr size_t kSize = std::tuple_size<Tuple>::value;
    return transform_tuple_strip_first(tuple, std::make_index_sequence<kSize - 1>());
}

/// Set up thread with the callable and arguments
template <typename Callable, typename ... Args>
std::function<void()> construct(Callable &&callable, Args && ... args) noexcept {
    constexpr bool kIsFreeFunction = !std::is_member_function_pointer_v<Callable>;
    constexpr auto kSize = sizeof...(args);
    static_assert(kIsFreeFunction || kSize > 0, "If member function, object must be an arg");

    if constexpr (kIsFreeFunction) {
        return [callable = std::forward<Callable>(callable),
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
        return [object,
                callable = std::forward<Callable>(callable),
                args = std::move(arg_tuple)] {
            std::apply([&](auto && ... args) {
                std::mem_fn(callable)(object, std::forward<decltype(args)>(args)...);
            }, std::move(args));
        };
    }
}

} // namespace tp::details
