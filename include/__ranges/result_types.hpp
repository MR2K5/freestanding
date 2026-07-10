#pragma once
// code: language=c++
// IWYU pragma: private: include <ranges>

#include <concepts>

namespace std::ranges {

template<class I, class O> struct in_out_result {
    [[no_unique_address]] I in;
    [[no_unique_address]] O out;

    template<class I2, class O2>

    requires std::convertible_to<I const&, I2> && std::convertible_to<O const&, O2>
    constexpr operator in_out_result<I2, O2>() const& {
        return {in, out};
    }
    template<class I2, class O2> requires std::convertible_to<I, I2> && std::convertible_to<O, O2>
    constexpr operator in_out_result<I2, O2>() && {
        return {std::move(in), std::move(out)};
    }
};

template<class I1, class I2> struct in_in_result {
    [[no_unique_address]] I1 in1;
    [[no_unique_address]] I2 in2;

    template<class X, class Y>
    requires std::convertible_to<I1 const&, X> && std::convertible_to<I2 const&, Y>
    constexpr operator in_out_result<X, Y>() const& {
        return {in1, in2};
    }
    template<class X, class Y> requires std::convertible_to<I1, X> && std::convertible_to<I2, Y>
    constexpr operator in_out_result<X, Y>() && {
        return {std::move(in1), std::move(in2)};
    }
};

template<class I, class F> struct in_fun_result {
    [[no_unique_address]] I in;
    [[no_unique_address]] F fun;

    template<class I2, class F2>

    requires convertible_to<I const&, I2> && convertible_to<F const&, F2>
    constexpr operator in_fun_result<I2, F2>() const& {
        return {in, fun};
    }

    template<class I2, class F2> requires convertible_to<I, I2> && convertible_to<F, F2>
    constexpr operator in_fun_result<I2, F2>() && {
        return {std::move(in), std::move(fun)};
    }
};

}  // namespace std::ranges
