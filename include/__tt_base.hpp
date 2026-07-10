#pragma once

namespace std {

template<class T> __add_rvalue_reference(T) declval() noexcept;

template<class T, T v> struct integral_constant {
    static constexpr T value = v;
    using value_type         = T;
    using type               = integral_constant;
    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; }  // since c++14
};

template<bool B> using bool_constant = integral_constant<bool, B>;
using true_type                      = bool_constant<true>;
using false_type                     = bool_constant<false>;

template<class T, class U> inline constexpr bool is_same_v = __is_same(T, U);
template<class T, class U> struct is_same: bool_constant<__is_same(T, U)> {};

}
