#pragma once
// code: language=c++
// IWYU pragma: private: include <algorithm>

#include <__ranges/core.hpp>
#include <__ranges/result_types.hpp>
#include <__functional/core.hpp>
#include <initializer_list>
#include <span>

namespace std {

template<class T> constexpr T const& max(T const& a, T const& b) {
    return a < b ? b : a;
}
template<class T> constexpr T const& min(T const& a, T const& b) {
    return a < b ? a : b;
}

template<class T, class Compare> constexpr T const& min(T const& a, T const& b, Compare comp) {
    return comp(a, b) ? a : b;
}
template<class T, class Compare> constexpr T const& max(T const& a, T const& b, Compare comp) {
    return comp(a, b) ? b : a;
}

namespace ranges {

inline constexpr struct __min_element_t {
    template<
        forward_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_strict_weak_order<projected<I, Proj>> Comp = ranges::less>
    constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        if (first == last) return last;
        auto smallest = first;
        while (++first != last)
            if (std::invoke(comp, std::invoke(proj, *first), std::invoke(proj, *smallest)))
                smallest = first;
        return smallest;
    }

    template<
        ranges::forward_range R, class Proj = identity,
        indirect_strict_weak_order<projected<ranges::iterator_t<R>, Proj>> Comp = ranges::less>
    constexpr ranges::borrowed_iterator_t<R>
    operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
        return (*this)(ranges::begin(r), ranges::end(r), std::ref(comp), std::ref(proj));
    }
} min_element;

inline constexpr struct __max_element_t {
    template<
        forward_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_strict_weak_order<projected<I, Proj>> Comp = ranges::less>
    static constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) {
        if (first == last) return last;

        auto largest = first;
        while (++first != last)
            if (std::invoke(comp, std::invoke(proj, *largest), std::invoke(proj, *first)))
                largest = first;
        return largest;
    }

    template<
        ranges::forward_range R, class Proj = identity,
        indirect_strict_weak_order<projected<ranges::iterator_t<R>, Proj>> Comp = ranges::less>
    static constexpr ranges::borrowed_iterator_t<R>
    operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::ref(comp), std::ref(proj));
    }
} max_element;

inline constexpr struct __max_t {
    template<
        class T, class Proj = identity,
        indirect_strict_weak_order<projected<T const*, Proj>> Comp = ranges::less>
    static constexpr T const& operator()(T const& a, T const& b, Comp comp = {}, Proj proj = {}) {
        return std::invoke(comp, std::invoke(proj, a), std::invoke(proj, b)) ? b : a;
    }

    template<
        copyable T, class Proj = identity,
        indirect_strict_weak_order<projected<T const*, Proj>> Comp = ranges::less>
    static constexpr T operator()(initializer_list<T> r, Comp comp = {}, Proj proj = {}) {
        return *ranges::max_element(r, std::ref(comp), std::ref(proj));
    }

    template<
        ranges::input_range R, class Proj = identity,
        indirect_strict_weak_order<projected<ranges::iterator_t<R>, Proj>> Comp = ranges::less>
    requires indirectly_copyable_storable<ranges::iterator_t<R>, ranges::range_value_t<R>*>
    static constexpr ranges::range_value_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        using V = ranges::range_value_t<R>;
        if constexpr (ranges::forward_range<R>)
            return static_cast<V>(*ranges::max_element(r, std::ref(comp), std::ref(proj)));
        else {
            auto i = ranges::begin(r);
            auto s = ranges::end(r);
            V m(*i);
            while (++i != s)
                if (std::invoke(comp, std::invoke(proj, m), std::invoke(proj, *i))) m = *i;
            return m;
        }
    }
} max;

inline constexpr struct __min_t {
    template<
        class T, class Proj = identity,
        indirect_strict_weak_order<projected<T const*, Proj>> Comp = ranges::less>
    static constexpr T const& operator()(T const& a, T const& b, Comp comp = {}, Proj proj = {}) {
        return std::invoke(comp, std::invoke(proj, b), std::invoke(proj, a)) ? b : a;
    }

    template<
        copyable T, class Proj = identity,
        indirect_strict_weak_order<projected<T const*, Proj>> Comp = ranges::less>
    static constexpr T operator()(initializer_list<T> r, Comp comp = {}, Proj proj = {}) {
        return *ranges::min_element(r, std::ref(comp), std::ref(proj));
    }

    template<
        ranges::input_range R, class Proj = identity,
        indirect_strict_weak_order<projected<ranges::iterator_t<R>, Proj>> Comp = ranges::less>
    requires indirectly_copyable_storable<ranges::iterator_t<R>, ranges::range_value_t<R>*>
    static constexpr ranges::range_value_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        using V = ranges::range_value_t<R>;
        if constexpr (ranges::forward_range<R>)
            return static_cast<V>(*ranges::min_element(r, std::ref(comp), std::ref(proj)));
        else {
            auto i = ranges::begin(r);
            auto s = ranges::end(r);
            V m(*i);
            while (++i != s)
                if (std::invoke(comp, std::invoke(proj, *i), std::invoke(proj, m))) m = *i;
            return m;
        }
    }
} min;

inline constexpr struct __minmax_element_t {
    template<
        forward_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_strict_weak_order<projected<I, Proj>> Comp = ranges::less>
    static constexpr ranges::minmax_element_result<I>
    operator()(I first, S last, Comp comp = {}, Proj proj = {}) {
        auto min = first, max = first;

        if (first == last || ++first == last) return {min, max};

        if (std::invoke(comp, std::invoke(proj, *first), std::invoke(proj, *min)))
            min = first;
        else
            max = first;

        while (++first != last) {
            auto i = first;
            if (++first == last) {
                if (std::invoke(comp, std::invoke(proj, *i), std::invoke(proj, *min)))
                    min = i;
                else if (!(std::invoke(comp, std::invoke(proj, *i), std::invoke(proj, *max))))
                    max = i;
                break;
            } else {
                if (std::invoke(comp, std::invoke(proj, *first), std::invoke(proj, *i))) {
                    if (std::invoke(comp, std::invoke(proj, *first), std::invoke(proj, *min)))
                        min = first;
                    if (!(std::invoke(comp, std::invoke(proj, *i), std::invoke(proj, *max))))
                        max = i;
                } else {
                    if (std::invoke(comp, std::invoke(proj, *i), std::invoke(proj, *min))) min = i;
                    if (!(std::invoke(comp, std::invoke(proj, *first), std::invoke(proj, *max))))
                        max = first;
                }
            }
        }
        return {min, max};
    }

    template<
        ranges::forward_range R, class Proj = identity,
        indirect_strict_weak_order<projected<ranges::iterator_t<R>, Proj>> Comp = ranges::less>
    static constexpr ranges::minmax_element_result<ranges::borrowed_iterator_t<R>>
    operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::ref(comp), std::ref(proj));
    }
} minmax_element;

inline constexpr struct __minmax_t {
    template<
        class T, class Proj = identity,
        indirect_strict_weak_order<projected<T const*, Proj>> Comp = ranges::less>
    static constexpr ranges::min_max_result<T const&>
    operator()(T const& a, T const& b, Comp comp = {}, Proj proj = {}) {
        if (std::invoke(comp, std::invoke(proj, b), std::invoke(proj, a))) return {b, a};

        return {a, b};
    }

    template<
        copyable T, class Proj = identity,
        indirect_strict_weak_order<projected<T const*, Proj>> Comp = ranges::less>
    static constexpr ranges::min_max_result<T>
    operator()(initializer_list<T> r, Comp comp = {}, Proj proj = {}) {
        auto result = ranges::minmax_element(r, std::ref(comp), std::ref(proj));
        return {*result.min, *result.max};
    }

    template<
        ranges::input_range R, class Proj = identity,
        indirect_strict_weak_order<projected<ranges::iterator_t<R>, Proj>> Comp = ranges::less>
    requires indirectly_copyable_storable<ranges::iterator_t<R>, ranges::range_value_t<R>*>
    static constexpr ranges::min_max_result<ranges::range_value_t<R>>
    operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        auto first      = ranges::begin(r);
        auto const last = ranges::end(r);

        auto min = static_cast<range_value_t<R>>(*first), max = min;
        while (++first != last) {
            auto x = static_cast<range_value_t<R>>(*first);
            if (++first == last) {
                if (std::invoke(comp, std::invoke(proj, x), std::invoke(proj, min)))
                    min = x;
                else if (!std::invoke(comp, std::invoke(proj, x), std::invoke(proj, max)))
                    max = x;

                break;
            }

            if (std::invoke(comp, std::invoke(proj, *first), std::invoke(proj, x))) {
                if (std::invoke(comp, std::invoke(proj, *first), std::invoke(proj, min)))
                    min = *first;
                if (!std::invoke(comp, std::invoke(proj, x), std::invoke(proj, max))) max = x;
            } else {
                if (std::invoke(comp, std::invoke(proj, x), std::invoke(proj, min))) min = x;
                if (!std::invoke(comp, std::invoke(proj, *first), std::invoke(proj, max)))
                    max = *first;
            }
        }

        return {min, max};
    }
} minmax;

inline constexpr struct __clamp_t {
    template<
        class T, class Proj = identity,
        indirect_strict_weak_order<projected<T const*, Proj>> Comp = ranges::less>
    constexpr T const&
    operator()(T const& v, T const& lo, T const& hi, Comp comp = {}, Proj proj = {}) const {
        auto&& pv = std::invoke(proj, v);

        if (std::invoke(comp, std::forward<decltype(pv)>(pv), std::invoke(proj, lo))) return lo;

        if (std::invoke(comp, std::invoke(proj, hi), std::forward<decltype(pv)>(pv))) return hi;

        return v;
    }
} clamp;

}  // namespace ranges

}  // namespace std
