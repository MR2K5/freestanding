#pragma once
// code: language=c++
// IWYU pragma: private: include <algorithm>

namespace std {

template<class T> constexpr T const& max(T const& a, T const& b) {
    return a < b ? b : a;
}
template<class T> constexpr T const& min(T const& a, T const& b) {
    return a < b ? a : b;
}

}  // namespace std
