#pragma once
// code: language=c++
// IWYU pragma: private: include <ranges>

#include <utility>
#include <type_traits>

namespace std::ranges {

namespace __detail {

using namespace ::std::__detail;

template<class T> concept __is_lvalue = is_lvalue_reference_v<T>;
template<class T> concept __is_rvalue = is_rvalue_reference_v<T>;

struct __iter_move {
    template<class T>
    requires(is_class_v<T> || is_enum_v<T>) && requires(T&& t) { iter_move(std::forward<T>(t)); }
    static constexpr decltype(auto) __impl(T&& t, __priority_tag<2>)
        noexcept(noexcept(iter_move(std::forward<T>(t)))) {
        return iter_move(std::forward<T>(t));
    }

    template<class T> requires requires(T&& t) {
        { *std::forward<T>(t) } -> __is_lvalue;
    } static constexpr decltype(auto) __impl(T&& t, __priority_tag<1>) noexcept(noexcept(*t)) {
        return std::move(*t);
    }
    template<class T> requires requires(T&& t) {
        { *std::forward<T>(t) } -> __is_rvalue;
    } static constexpr decltype(auto) __impl(T&& t, __priority_tag<0>) noexcept(noexcept(*t)) {
        return *t;
    }

    template<class T> requires requires(T&& t) { __impl(_FORWARD(t), __priority_tag<2>()); }
    static constexpr decltype(auto) operator()(T&& t)
        noexcept(noexcept(__impl(_FORWARD(t), __priority_tag<2>()))) {
        return __impl(_FORWARD(t), __priority_tag<2>());
    }
};

}  // namespace __detail

inline namespace __cpo {
inline constexpr __detail::__iter_move iter_move;
}

}  // namespace std::ranges
