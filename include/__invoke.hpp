#pragma once

#include <cstddef>

#include "__tt_base.hpp"

namespace std {

template<class F, class... As>
inline constexpr bool is_invocable_v =
    requires(F&& f, As&&... as) { __builtin_invoke(FWD(f), FWD(as)...); };
template<class F, class... As>
inline constexpr bool is_nothrow_invocable_v = requires(F&& f, As&&... as) {
    { __builtin_invoke(FWD(f), FWD(as)...) } noexcept;
};

template<class F, class T> concept __conv_to    = __is_void(T) || __is_convertible(F, T);
template<class F, class T> concept __nt_conv_to = __is_void(T) || __is_nothrow_convertible(F, T);

template<class R, class F, class... As>
inline constexpr bool is_invocable_r_v = requires(F&& f, As&&... as) {
    { __builtin_invoke(FWD(f), FWD(as)...) } -> __conv_to<R>;
};
template<class R, class F, class... As>
inline constexpr bool is_nothrow_invocable_r_v = requires(F&& f, As&&... as) {
    { __builtin_invoke(FWD(f), FWD(as)...) } noexcept -> __nt_conv_to<R>;
};

template<class F, class... As>
using invoke_result_t = decltype(__builtin_invoke(declval<F>(), declval<As>()...));

template<class F, class... As>
invoke_result_t<F, As...> invoke(F&& f, As&&... as) noexcept(is_nothrow_invocable_v<F, As...>)
    requires is_invocable_v<F, As...> {
    return __builtin_invoke(FWD(f), FWD(as)...);
}

template<class R, class F, class... As>
R invoke_r(F&& f, As&&... as) noexcept(is_nothrow_invocable_r_v<R, F, As...>)
    requires is_invocable_r_v<R, F, As...> {
    static_assert(
        !__reference_converts_from_temporary(R, decltype(__builtin_invoke(FWD(f), FWD(as)...)))
    );
    return static_cast<R>(__builtin_invoke(FWD(f), FWD(as)...));
}

template<class F, class... As> struct is_invocable: bool_constant<is_invocable_v<F, As...>> {};
template<class F, class... As>
struct is_nothrow_invocable: bool_constant<is_nothrow_invocable_v<F, As...>> {};
template<class R, class F, class... As>
struct is_invocable_r: bool_constant<is_invocable_r_v<R, F, As...>> {};
template<class R, class F, class... As>
struct is_nothrow_invocable_r: bool_constant<is_nothrow_invocable_r_v<R, F, As...>> {};

template<class F, class... As> struct invoke_result {};

template<class F, class... As> requires is_invocable_v<F, As...> struct invoke_result<F, As...> {
    using type = invoke_result_t<F, As...>;
};

}  // namespace std
