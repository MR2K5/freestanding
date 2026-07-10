#pragma once
// code: language=c++

#include <__ranges/core.hpp>
#include <concepts>
#include <cstddef>
#include <optional>
#include <type_traits>

namespace std::ranges {

template<move_constructible T> requires is_object_v<T>
class single_view: public view_interface<single_view<T>> {
    __detail::movable_box<T> value_;

public:
    single_view() requires default_initializable<T> = default;
    constexpr explicit single_view(T const& t) requires copy_constructible<T>
        : value_(in_place, t) {}
    constexpr explicit single_view(T&& t): value_(in_place, std::move(t)) {}
    template<class... As> requires constructible_from<T, As...>
    constexpr explicit single_view(in_place_t, As&&... as)
        : value_(in_place, std::forward<As>(as)...) {}

    constexpr T* begin() noexcept { return data(); }
    constexpr T const* begin() const noexcept { return data(); }
    constexpr T* end() noexcept { return data() + 1; }
    constexpr T const* end() const noexcept { return data() + 1; }
    static constexpr bool empty() noexcept { return false; }
    static constexpr size_t size() noexcept { return 1; }
    constexpr T* data() noexcept { return value_.operator->(); }
    constexpr T const* data() const noexcept { return value_.operator->(); }
};

template<class T> single_view(T) -> single_view<T>;

namespace views {

inline constexpr auto single = []<class E>(E&& e) requires requires {
    single_view<decay_t<E>>(std::forward<E>(e));
} {
    return single_view<decay_t<E>>(std::forward<E>(e));
};

}  // namespace views

}  // namespace std::ranges
