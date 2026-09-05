#pragma once
// code: language=c++
// IWYU pragma: private: include <iterator>

#include <utility>
#include <concepts>
#include <__iterator/concepts.hpp>

#include <type_traits>
#include <utility>

namespace std::ranges {

namespace __iter_swap {

void iter_swap(auto, auto) = delete;

struct __iter_swap_fn {
    template<class X, class Y>
    static constexpr iter_value_t<X> iter_exchange_move(X&& x, Y&& y) noexcept(
        noexcept(iter_value_t<X>(ranges::iter_move(x))) && noexcept(*x = ranges::iter_move(y))
    ) {
        iter_value_t<X> old(ranges::iter_move(x));
        *x = ranges::iter_move(y);
        return old;
    }

    template<class I1, class I2>
    requires(is_class_v<I1> || is_enum_v<I1> || is_class_v<I2> || is_enum_v<I2>) && requires {
        (void)iter_swap(declval<I1>(), declval<I2>());
    } static constexpr void __impl(I1&& i1, I2&& i2, __detail::__priority_tag<2>)
        noexcept(noexcept(iter_swap(FWD(i1), FWD(i2)))) {
        (void)iter_swap(FWD(i1), FWD(i2));
    }

    template<class I1, class I2> requires indirectly_readable<I1> && indirectly_readable<I2>
                                       && swappable_with<iter_reference_t<I1>, iter_reference_t<I2>>
    static constexpr void __impl(I1&& i1, I2&& i2, __detail::__priority_tag<1>)
        noexcept(noexcept(ranges::swap(*FWD(i1), *FWD(i2)))) {
        ranges::swap(*FWD(i1), *FWD(i2));
    }

    template<class I1, class I2>
    requires indirectly_movable_storable<I1, I2> && indirectly_movable_storable<I2, I1>
    static constexpr void __impl(I1&& i1, I2&& i2, __detail::__priority_tag<0>)
        noexcept(noexcept(iter_exchange_move(FWD(i2), FWD(i1)))) {
        iter_exchange_move(FWD(i2), FWD(i1));
    }

    template<class I1, class I2>
    requires requires { __impl(declval<I1>(), declval<I2>(), __detail::__priority_tag<2>()); }
    static constexpr void operator()(I1&& i1, I2&& i2)
        noexcept(noexcept(__impl(declval<I1>(), declval<I2>(), __detail::__priority_tag<2>()))) {
        __impl(FWD(i1), FWD(i2), __detail::__priority_tag<2>());
    }
};

}  // namespace __iter_swap

inline namespace __cpo {
inline constexpr __iter_swap::__iter_swap_fn iter_swap{};
}

}  // namespace std::ranges

namespace std {
template<class I1, class I2 = I1>

concept indirectly_swappable =
    indirectly_readable<I1> && indirectly_readable<I2> && requires(const I1 i1, const I2 i2) {
        ranges::iter_swap(i1, i1);
        ranges::iter_swap(i1, i2);
        ranges::iter_swap(i2, i1);
        ranges::iter_swap(i2, i2);
    };

template<class I>
concept permutable =
    forward_iterator<I> && indirectly_movable_storable<I, I> && indirectly_swappable<I, I>;

template<class I, class Comp = ranges::less, class Proj = std::identity>

concept sortable = permutable<I> && indirect_strict_weak_order<Comp, projected<I, Proj>>;

}  // namespace std
