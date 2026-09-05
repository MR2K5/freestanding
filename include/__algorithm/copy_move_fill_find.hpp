#pragma once
// code: language=c++
// IWYU pragma: private: include <algorithm>

#include <compare>

namespace std {

template<class InputIt1, class InputIt2>

constexpr auto lexicographical_compare_three_way(
    InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2
) {
    return lexicographical_compare_three_way(first1, last1, first2, last2, compare_three_way{});
}

}  // namespace std
