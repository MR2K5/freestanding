#pragma once

#include <__iterator/counted_iterator.hpp>
#include <__ranges/core.hpp>
#include <__ranges/subrange.hpp>
#include <span>
#include <type_traits>

namespace std::ranges::views {

inline constexpr struct __counted_fn {
    static constexpr auto __impl(auto&& E, auto&& F, __detail::__priority_tag<2>) _STD_RETURN_REQ(
        contiguous_iterator<decay_t<decltype(E)>>,
        span(
            std::to_address(FWD(E)),
            static_cast<size_t>(static_cast<iter_difference_t<decay_t<decltype(E)>>>(FWD(F)))
        )
    );

    static constexpr auto __impl(auto&& E, auto&& F, __detail::__priority_tag<1>) _STD_RETURN_REQ(
        random_access_iterator<decay_t<decltype(E)>>,
        subrange(E, E + static_cast<iter_difference_t<decay_t<decltype(E)>>>(FWD(F)))
    );

    static constexpr auto __impl(auto&& E, auto&& F, __detail::__priority_tag<0>) _STD_RETURN(
        subrange(counted_iterator(FWD(E), FWD(F)), default_sentinel)
    );

    template<class E, class F, class T = decay_t<E>, class D = iter_difference_t<T>>
    requires convertible_to<F, D> static constexpr auto
    operator()(E&& e, F&& f) _STD_RETURN(__impl(FWD(e), FWD(f), __detail::__priority_tag<2>()));
} counted;

}  // namespace std::ranges::views
