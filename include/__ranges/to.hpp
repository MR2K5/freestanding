#pragma once
// code: language=c++
// IWYU pragma: private: include <ranges>

#include <__algorithm/nonmodifying.hpp>
#include <__iterator/concepts.hpp>
#include <__ranges/core.hpp>
#include <__ranges/transform_view.hpp>
#include <concepts>
#include <type_traits>
#include <utility>

namespace std::ranges {

namespace __detail {

template<class C>
concept __reservable_container = sized_range<C> && requires(C& c, range_size_t<C> n) {
    c.reserve(n);
    { c.capacity() } -> same_as<decltype(n)>;
    { c.max_size() } -> same_as<decltype(n)>;
};

template<class Container, class Reference>

constexpr bool __container_appendable = requires(Container& c, Reference&& ref) {
    requires(
        requires { c.emplace_back(std::forward<Reference>(ref)); }
        || requires { c.push_back(std::forward<Reference>(ref)); }
        || requires { c.emplace(c.end(), std::forward<Reference>(ref)); }
        || requires { c.insert(c.end(), std::forward<Reference>(ref)); }
    );
};

template<class R, class C> constexpr auto __container_appender(C& c) {
    return [&c]<class Reference>(Reference&& ref) {
        if constexpr (requires { c.emplace_back(declval<Reference>()); })
            c.emplace_back(std::forward<Reference>(ref));
        else if constexpr (requires { c.push_back(declval<Reference>()); })
            c.push_back(std::forward<Reference>(ref));
        else if constexpr (requires { c.emplace(c.end(), declval<Reference>()); })
            c.emplace(c.end(), std::forward<Reference>(ref));
        else
            c.insert(c.end(), std::forward<Reference>(ref));
    };
}
template<class R, class T>
concept __container_compatible_range = input_range<R> && convertible_to<range_reference_t<R>, T>;

template<class R> struct __deduce_inputit {
    using iterator_category = input_iterator_tag;
    using value_type        = range_value_t<R>;
    using difference_type   = ptrdiff_t;
    using pointer           = add_pointer_t<range_reference_t<R>>;
    using reference         = range_reference_t<R>;
    reference operator*() const;
    pointer operator->() const;
    __deduce_inputit& operator++();
    __deduce_inputit operator++(int);
    bool operator==(__deduce_inputit const&) const;
};

}  // namespace __detail

template<class C, input_range R, class... As> requires(!view<C>) constexpr C to(R&& r, As&&... as) {
    if constexpr (!input_range<C> || convertible_to<range_reference_t<R>, range_value_t<C>>) {
        if constexpr (constructible_from<C, R, As...>) {
            return C(std::forward<R>(r), std::forward<As>(as)...);
        } else if constexpr (constructible_from<C, from_range_t, R, As...>) {
            return C(from_range, std::forward<R>(r), std::forward<As>(as)...);
        } else if constexpr (common_range<R> && requires {
                                 requires derived_from<
                                     typename iterator_traits<iterator_t<R>>::iterator_category,
                                     input_iterator_tag>;
                             } && constructible_from<C, iterator_t<R>, sentinel_t<R>, As...>) {
            return C(ranges::begin(r), ranges::end(r), std::forward<As>(as)...);
        } else {
            static_assert(constructible_from<C, As...> && __detail::__container_appendable<C, range_reference_t<R>>);
            C c(std::forward<As>(as)...);
            if constexpr (sized_range<R> && __detail::__reservable_container<R>)
                c.reserve(static_cast<range_size_t<C>>(ranges::size(r)));
            ranges::for_each(r, __detail::__container_appender(c));
        }
    } else {
        static_assert(input_range<range_reference_t<C>>);
        return to<C>(
            ranges::ref_view(r) | views::transform([](auto&& elem) {
                return to<range_value_t<C>>(std::forward<decltype(elem)>(elem));
            }),
            forward<As>(as)...
        );
    }
}

template<template<class...> class C, input_range R, class... As>
constexpr auto to(R&& r, As&&... as) {
    auto deduce = [] {
        if constexpr (requires { C(declval<R>(), declval<As>()...); })
            return type_identity<decltype(C(declval<R>(), declval<As>()...))>();
        else if constexpr (requires { C(from_range, declval<R>(), declval<As>()...); })
            return type_identity<decltype(C(from_range, declval<R>(), declval<As>()...))>();
        else if constexpr (requires {
                               C(declval<__detail::__deduce_inputit<R>>(),
                                 declval<__detail::__deduce_inputit<R>>(), declval<As>()...);
                           })
            return type_identity<decltype(C(
                declval<__detail::__deduce_inputit<R>>(), declval<__detail::__deduce_inputit<R>>(),
                declval<As>()...
            ))>();
        else
            static_assert(false, "Cannot deduce type from template-id");
    };

    using X = decltype(deduce())::type;
    return to<X>(std::forward<R>(r), std::forward<As>(as)...);
}

template<class C, class... As> constexpr auto to(As&&... as) {
    return __range_adaptor_closure_fn([... bound(std::forward<As>(as)
                                      )](this auto&& self, auto&& r) {
        return to<C>(std::forward<decltype(r)>(r), std::forward_like<decltype(self)>(bound)...);
    });
}
template<template<class...> class C, class... As> constexpr auto to(As&&... as) {
    return __range_adaptor_closure_fn([... bound(std::forward<As>(as)
                                      )](this auto&& self, auto&& r) {
        return to<C>(std::forward<decltype(r)>(r), std::forward_like<decltype(self)>(bound)...);
    });
}

}  // namespace std::ranges
