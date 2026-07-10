#pragma once
// code: language=c++
// IWYU pragma: private: include <iterator>

#include "__iterator/concepts.hpp"
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <type_traits>

namespace std {

template<class It, class Dist> constexpr void advance(It& it, Dist n) {
    using cat = iterator_traits<It>::iterator_category;
    static_assert(__detail::__LegacyInputIterator<It>);
    static_assert(is_base_of_v<input_iterator_tag, cat>);

    assert(n >= 0 || __detail::__LegacyBidirectionalIterator<It>);

    auto dist = typename iterator_traits<It>::difference_type(n);
    if constexpr (is_base_of_v<random_access_iterator_tag, cat>) {
        it += dist;
    } else {
        while (dist > 0) {
            --dist;
            ++it;
        }
        if constexpr (is_base_of_v<bidirectional_iterator_tag, cat>)
            while (dist < 0) {
                ++dist;
                --it;
            }
    }
}

template<class It>
constexpr typename iterator_traits<It>::difference_type distance(It first, It last) {
    static_assert(__detail::__LegacyInputIterator<It>);

    using cat = iterator_traits<It>::iterator_category;
    static_assert(is_base_of_v<input_iterator_tag, cat>);

    if constexpr (is_base_of_v<random_access_iterator_tag, cat>) {
        return last - first;
    } else {
        typename iterator_traits<It>::difference_type result = 0;
        while (first != last) {
            ++result;
            ++first;
        }
        return result;
    }
}

template<class It> constexpr It next(It it, typename iterator_traits<It>::difference_type n = 1) {
    std::advance(it, n);
    return it;
}
template<class It> constexpr It prev(It it, typename iterator_traits<It>::difference_type n = 1) {
    static_assert(__detail::__LegacyBidirectionalIterator<It>);
    std::advance(it, -n);
    return it;
}

template<class C> constexpr auto begin(C& c) noexcept(noexcept(c.begin())) -> decltype(c.begin()) {
    return c.begin();
}
template<class C>
constexpr auto begin(C const& c) noexcept(noexcept(c.begin())) -> decltype(c.begin()) {
    return c.begin();
}
template<class T, size_t N> constexpr T* begin(T (&arr)[N]) noexcept {
    return arr;
}
template<class C>
constexpr auto cbegin(C const& c) noexcept(noexcept(std::begin(c))) -> decltype(std::begin(c)) {
    return std::begin(c);
}

template<class C> constexpr auto end(C& c) noexcept(noexcept(c.end())) -> decltype(c.end()) {
    return c.end();
}
template<class C> constexpr auto end(C const& c) noexcept(noexcept(c.end())) -> decltype(c.end()) {
    return c.end();
}
template<class T, size_t N> constexpr T* end(T (&arr)[N]) noexcept {
    return arr + N;
}
template<class C>
constexpr auto cend(C const& c) noexcept(noexcept(std::end(c))) -> decltype(std::end(c)) {
    return std::end(c);
}

template<class C>
constexpr auto size(C const& c) noexcept(noexcept(c.size())) -> decltype(c.size()) {
    return c.size();
}
template<class C>
constexpr auto ssize(C const& c) noexcept(
    noexcept(static_cast<common_type_t<ptrdiff_t, make_signed_t<decltype(c.size())>>>(c.size()))
) -> common_type_t<ptrdiff_t, make_signed_t<decltype(c.size())>> {
    return static_cast<common_type_t<ptrdiff_t, make_signed_t<decltype(c.size())>>>(c.size());
}
template<class T, size_t N> constexpr size_t size(T (&)[N]) noexcept {
    return N;
}
template<class T, ptrdiff_t N> constexpr ptrdiff_t ssize(T (&)[N]) noexcept {
    return N;
}

template<class C>
[[nodiscard]] constexpr auto empty(C const& c)
    noexcept(noexcept(c.empty())) -> decltype(c.empty()) {
    return c.empty();
}
template<class T, size_t N> [[nodiscard]] constexpr bool empty(T (&)[N]) noexcept {
    return N != 0;
}
template<class E> [[nodiscard]] constexpr bool empty(initializer_list<E> il) noexcept {
    return il.size() != 0;
}

template<class C> constexpr auto data(C& c) noexcept(noexcept(c.data())) -> decltype(c.data()) {
    return c.data();
}
template<class C>
constexpr auto data(C const& c) noexcept(noexcept(c.data())) -> decltype(c.data()) {
    return c.data();
}
template<class T, size_t N> constexpr T* data(T (&array)[N]) noexcept {
    return array;
}
template<class E> constexpr E const* data(initializer_list<E> il) noexcept {
    return il.begin();
}

}  // namespace std
