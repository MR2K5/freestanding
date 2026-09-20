#pragma once

#include <__iterator/concepts.hpp>
#include <__memory/base.hpp>
#include <__ranges/core.hpp>
#include <__ranges/result_types.hpp>
#include <__ranges/subrange.hpp>
#include <compare>
#include <optional>
#include <utility>

namespace std::ranges {

inline constexpr struct __all_of_fn {
    template<
        input_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    static constexpr bool operator()(I first, S last, Pred pred, Proj proj = {}) {
        for (; first != last; ++first) {
            if (!std::invoke(pred, std::invoke(proj, *first))) return false;
        }
        return true;
    }

    template<
        input_range R, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    static constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(pred), std::move(proj));
    }
} all_of;

inline constexpr struct __any_of_fn {
    template<
        input_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    static constexpr bool operator()(I first, S last, Pred pred, Proj proj = {}) {
        for (; first != last; ++first) {
            if (std::invoke(pred, std::invoke(proj, *first))) return true;
        }
        return false;
    }

    template<
        input_range R, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    static constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(pred), std::move(proj));
    }
} any_of;

inline constexpr struct __none_of_fn {
    template<
        input_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    static constexpr bool operator()(I first, S last, Pred pred, Proj proj = {}) {
        for (; first != last; ++first) {
            if (std::invoke(pred, std::invoke(proj, *first))) return false;
        }
        return true;
    }

    template<
        input_range R, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    static constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(pred), std::move(proj));
    }
} none_of;

// FInding

inline constexpr struct __find_fn {
    template<
        input_iterator I, sentinel_for<I> S, class Proj = identity,
        class T = projected_value_t<I, Proj>>
    requires indirect_binary_predicate<equal_to, projected<I, Proj>, T const*>
    static constexpr I operator()(I first, S last, T const& value, Proj proj = {}) {
        for (; first != last; ++first)
            if (std::invoke(proj, *first) == value) return first;
        return first;
    }

    template<input_range R, class T, class Proj = identity>
    requires indirect_binary_predicate<equal_to, projected<iterator_t<R>, Proj>, T const*>
    static constexpr ranges::borrowed_iterator_t<R>
    operator()(R&& r, T const& value, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), value, std::move(proj));
    }
} find;

inline constexpr struct __find_if_fn {
    template<
        input_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    static constexpr I operator()(I first, S last, Pred pred, Proj proj = {}) {
        for (; first != last; ++first)
            if (std::invoke(pred, std::invoke(proj, *first))) return first;
        return first;
    }

    template<
        input_range R, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    static constexpr borrowed_iterator_t<R> operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(pred), std::move(proj));
    }
} find_if;

inline constexpr struct __find_if_not_fn {
    template<
        input_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    static constexpr I operator()(I first, S last, Pred pred, Proj proj = {}) {
        for (; first != last; ++first)
            if (!std::invoke(pred, std::invoke(proj, *first))) return first;
        return first;
    }

    template<
        input_range R, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    static constexpr borrowed_iterator_t<R> operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(pred), std::move(proj));
    }

    template<
        forward_range R, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    static constexpr borrowed_iterator_t<R> operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(
            ranges::begin(r), ranges::next(ranges::begin(r), ranges::end(r)), std::move(pred),
            std::move(proj)
        );
    }
} find_if_not;

inline constexpr struct __find_last_fn {
    template<
        forward_iterator I, sentinel_for<I> S, class Proj = identity,
        class T = projected_value_t<iterator_t<I>, Proj>>
    requires indirect_binary_predicate<equal_to, projected<I, Proj>, T const*>
    static constexpr subrange<I> operator()(I first, S last, T const& value, Proj proj = {}) {
        // Note: if I is mere forward_iterator, we may only go from begin to end.
        optional<I> found;
        for (; first != last; ++first)
            if (std::invoke(proj, *first) == value) found = first;

        if (!found) return {first, first};

        return {*found, ranges::next(*found, last)};
    }

    template<
        forward_range R, class Proj = identity, class T = projected_value_t<iterator_t<R>, Proj>>
    requires indirect_binary_predicate<equal_to, projected<iterator_t<R>, Proj>, T const*>
    static constexpr borrowed_subrange_t<R> operator()(R&& r, T const& value, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), value, std::move(proj));
    }
} find_last;

inline constexpr struct __find_last_if_fn {
    template<
        forward_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    static constexpr subrange<I> operator()(I first, S last, Pred pred, Proj proj = {}) {
        // Note: if I is mere forward_iterator, we may only go from begin to end.
        std::optional<I> found;
        for (; first != last; ++first)
            if (std::invoke(pred, std::invoke(proj, *first))) found = first;

        if (!found) return {first, first};

        return {*found, ranges::next(*found, last)};
    }

    template<
        forward_range R, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    static constexpr borrowed_subrange_t<R> operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(
            ranges::begin(r), ranges::next(ranges::begin(r), ranges::end(r)), std::move(pred),
            std::move(proj)
        );
    }
} find_last_if;

inline constexpr struct __find_last_if_not_fn {
    template<
        forward_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    static constexpr subrange<I> operator()(I first, S last, Pred pred, Proj proj = {}) {
        // Note: if I is mere forward_iterator, we may only go from begin to end.
        std::optional<I> found;
        for (; first != last; ++first)
            if (!std::invoke(pred, std::invoke(proj, *first))) found = first;

        if (!found) return {first, first};

        return {*found, ranges::next(*found, last)};
    }

    template<
        forward_range R, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    static constexpr borrowed_subrange_t<R> operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(
            ranges::begin(r), ranges::next(ranges::begin(r), ranges::end(r)), std::move(pred),
            std::move(proj)
        );
    }
} find_last_if_not;

inline constexpr struct __search_fn {
    template<
        forward_iterator I1, sentinel_for<I1> S1, forward_iterator I2, sentinel_for<I2> S2,
        class Pred = equal_to, class Proj1 = identity, class Proj2 = identity>
    requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
    static constexpr subrange<I1> operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        for (;; ++first1) {
            I1 it1 = first1;
            for (I2 it2 = first2;; ++it1, ++it2) {
                if (it2 == last2) return {first1, it1};
                if (it1 == last1) return {it1, it1};
                if (!std::invoke(pred, std::invoke(proj1, *it1), std::invoke(proj2, *it2))) break;
            }
        }
    }

    template<
        forward_range R1, forward_range R2, class Pred = equal_to, class Proj1 = identity,
        class Proj2 = identity>
    requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
    static constexpr borrowed_subrange_t<R1>
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::move(pred),
            std::move(proj1), std::move(proj2)
        );
    }
} search;

inline constexpr struct __find_end_fn {
    template<
        forward_iterator I1, sentinel_for<I1> S1, forward_iterator I2, sentinel_for<I2> S2,
        class Pred = equal_to, class Proj1 = identity, class Proj2 = identity>
    requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
    static constexpr subrange<I1> operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        if (first2 == last2) {
            auto last_it = ranges::next(first1, last1);
            return {last_it, last_it};
        }
        auto result = ranges::search(std::move(first1), last1, first2, last2, pred, proj1, proj2);

        if (result.empty()) return result;

        for (;;) {
            auto new_result =
                ranges::search(std::next(result.begin()), last1, first2, last2, pred, proj1, proj2);
            if (new_result.empty())
                return result;
            else
                result = std::move(new_result);
        }
    }

    template<
        forward_range R1, forward_range R2, class Pred = equal_to, class Proj1 = identity,
        class Proj2 = identity>
    requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
    static constexpr borrowed_subrange_t<R1>
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::move(pred),
            std::move(proj1), std::move(proj2)
        );
    }
} find_end;

inline struct find_first_of_fn {
    template<
        input_iterator I1, sentinel_for<I1> S1, forward_iterator I2, sentinel_for<I2> S2,
        class Pred = equal_to, class Proj1 = identity, class Proj2 = identity>
    requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2> static constexpr I1 operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        for (; first1 != last1; ++first1)
            for (auto i = first2; i != last2; ++i)
                if (std::invoke(pred, std::invoke(proj1, *first1), std::invoke(proj2, *i)))
                    return first1;
        return first1;
    }

    template<
        input_range R1, forward_range R2, class Pred = equal_to, class Proj1 = identity,
        class Proj2 = identity>
    requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
    static constexpr borrowed_iterator_t<R1>
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::move(pred),
            std::move(proj1), std::move(proj2)
        );
    }
} find_first_of;

inline constexpr struct __adjacent_find_fn {
    template<
        forward_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_binary_predicate<projected<I, Proj>, projected<I, Proj>> Pred = equal_to>
    static constexpr I operator()(I first, S last, Pred pred = {}, Proj proj = {}) {
        if (first == last) return first;
        auto next = ranges::next(first);
        for (; next != last; ++next, ++first)
            if (std::invoke(pred, std::invoke(proj, *first), std::invoke(proj, *next)))
                return first;
        return next;
    }

    template<
        forward_range R, class Proj = identity,
        indirect_binary_predicate<projected<iterator_t<R>, Proj>, projected<iterator_t<R>, Proj>>
            Pred = equal_to>
    static constexpr borrowed_iterator_t<R> operator()(R&& r, Pred pred = {}, Proj proj = {}) {
        return operator()(
            ranges::begin(r), ranges::next(ranges::begin(r), ranges::end(r)), std::move(pred),
            std::move(proj)
        );
    }
} adjacent_find;

inline constexpr struct __contains_fn {
    template<
        input_iterator I, sentinel_for<I> S, class Proj = identity,
        class T = projected_value_t<I, Proj>>
    requires indirect_binary_predicate<equal_to, projected<I, Proj>, T const*>
    static constexpr bool operator()(I first, S last, T const& value, Proj proj = {}) {
        return ranges::find(std::move(first), last, value, std::move(proj)) != last;
    }

    template<input_range R, class Proj = identity, class T = projected_value_t<iterator_t<R>, Proj>>
    requires indirect_binary_predicate<equal_to, projected<iterator_t<R>, Proj>, T const*>
    static constexpr bool operator()(R&& r, T const& value, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(value), std::move(proj));
    }
} contains;

inline constexpr struct __contains_subrange_fn {
    template<
        forward_iterator I1, sentinel_for<I1> S1, forward_iterator I2, sentinel_for<I2> S2,
        class Pred = equal_to, class Proj1 = identity, class Proj2 = identity>
    requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2> static constexpr bool operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        return (first2 == last2)
            || !ranges::search(
                    first1, last1, first2, last2, std::move(pred), std::move(proj1),
                    std::move(proj2)
            )
                    .empty();
    }

    template<
        forward_range R1, forward_range R2, class Pred = equal_to, class Proj1 = identity,
        class Proj2 = identity>
    requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
    static constexpr bool
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::move(pred),
            std::move(proj1), std::move(proj2)
        );
    }
} contains_subrange;

inline constexpr struct __for_each_fn {
    template<
        input_iterator I, sentinel_for<I> S, class Proj = identity,
        indirectly_unary_invocable<projected<I, Proj>> Fun>
    static constexpr for_each_result<I, Fun> operator()(I first, S last, Fun f, Proj proj = {}) {
        for (; first != last; ++first) std::invoke(f, std::invoke(proj, *first));
        return {std::move(first), std::move(f)};
    }

    template<
        input_range R, class Proj = identity,
        indirectly_unary_invocable<projected<iterator_t<R>, Proj>> Fun>
    static constexpr for_each_result<borrowed_iterator_t<R>, Fun>
    operator()(R&& r, Fun f, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(f), std::move(proj));
    }
} for_each;

inline constexpr struct __mismatch_fn {
    template<
        input_iterator I1, sentinel_for<I1> S1, input_iterator I2, sentinel_for<I2> S2,
        class Pred = equal_to, class Proj1 = identity, class Proj2 = identity>
    requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
    static constexpr mismatch_result<I1, I2> operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        for (; first1 != last1 && first2 != last2; ++first1, (void)++first2)
            if (not std::invoke(pred, std::invoke(proj1, *first1), std::invoke(proj2, *first2)))
                break;

        return {first1, first2};
    }

    template<
        input_range R1, input_range R2, class Pred = equal_to, class Proj1 = identity,
        class Proj2 = identity>
    requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
    static constexpr mismatch_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>>
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::move(pred),
            std::move(proj1), std::move(proj2)
        );
    }
} mismatch;

inline constexpr struct __starts_with_fn {
    template<
        input_iterator I1, sentinel_for<I1> S1, input_iterator I2, sentinel_for<I2> S2,
        class Pred = equal_to, class Proj1 = identity, class Proj2 = identity>
    requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2> static constexpr bool operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        return ranges::mismatch(
                   std::move(first1), last1, std::move(first2), last2, std::move(pred),
                   std::move(proj1), std::move(proj2)
               )
                   .in2
            == last2;
    }

    template<
        input_range R1, input_range R2, class Pred = equal_to, class Proj1 = identity,
        class Proj2 = identity>
    requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
    static constexpr bool
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::move(pred),
            std::move(proj1), std::move(proj2)
        );
    }
} starts_with;

inline constexpr struct __equal_fn {
    template<
        input_iterator I1, sentinel_for<I1> S1, input_iterator I2, sentinel_for<I2> S2,
        class Pred = ranges::equal_to, class Proj1 = identity, class Proj2 = identity>
    requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2> static constexpr bool operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        // Fast path: both sentinels are sized
        if constexpr (sized_sentinel_for<S1, I1> && sized_sentinel_for<S2, I2>) {
            if ((last1 - first1) != (last2 - first2)) return false;

            for (; first1 != last1; ++first1, (void)++first2) {
                if (!std::invoke(pred, std::invoke(proj1, *first1), std::invoke(proj2, *first2)))
                    return false;
            }
            return true;
        } else {
            // General path: must guard both iterators
            for (; first1 != last1 && first2 != last2; ++first1, (void)++first2) {
                if (!std::invoke(pred, std::invoke(proj1, *first1), std::invoke(proj2, *first2)))
                    return false;
            }
            return first1 == last1 && first2 == last2;
        }
    }

    template<
        input_range R1, input_range R2, class Pred = ranges::equal_to, class Proj1 = identity,
        class Proj2 = identity>
    requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
    static constexpr bool
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        // Sized-range optimization before iterator decay
        if constexpr (sized_range<R1> && sized_range<R2>) {
            if (ranges::distance(r1) != ranges::distance(r2)) return false;
        }

        return __equal_fn::operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::move(pred),
            std::move(proj1), std::move(proj2)
        );
    }
} equal;

inline constexpr struct __ends_with_fn {
    template<
        input_iterator I1, sentinel_for<I1> S1, input_iterator I2, sentinel_for<I2> S2,
        class Pred = ranges::equal_to, class Proj1 = identity, class Proj2 = identity>
    requires(forward_iterator<I1> || sized_sentinel_for<S1, I1>)
         && (forward_iterator<I2> || sized_sentinel_for<S2, I2>)
         && indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
    static constexpr bool operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        auto const n1 = ranges::distance(first1, last1);
        auto const n2 = ranges::distance(first2, last2);
        if (n1 < n2) return false;
        ranges::advance(first1, n1 - n2);
        return ranges::equal(
            std::move(first1), last1, std::move(first2), last2, std::move(pred), std::move(proj1),
            std::move(proj2)
        );
    }

    template<
        ranges::input_range R1, ranges::input_range R2, class Pred = ranges::equal_to,
        class Proj1 = identity, class Proj2 = identity>
    requires(ranges::forward_range<R1> || ranges::sized_range<R1>)
         && (ranges::forward_range<R2> || ranges::sized_range<R2>)
         && indirectly_comparable<
                ranges::iterator_t<R1>, ranges::iterator_t<R2>, Pred, Proj1, Proj2>
    static constexpr bool
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::move(pred),
            std::move(proj1), std::move(proj2)
        );
    }
} ends_with;

inline constexpr struct __count_fn {
    template<
        input_iterator I, sentinel_for<I> S, class Proj = identity,
        class T = projected_value_t<I, Proj>>
    requires indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, T const*>
    static constexpr iter_difference_t<I>
    operator()(I first, S last, T const& value, Proj proj = {}) {
        iter_difference_t<I> counter = 0;
        for (; first != last; ++first)
            if (std::invoke(proj, *first) == value) ++counter;
        return counter;
    }

    template<
        ranges::input_range R, class Proj = identity,
        class T = projected_value_t<ranges::iterator_t<R>, Proj>>
    requires indirect_binary_predicate<
        ranges::equal_to, projected<ranges::iterator_t<R>, Proj>, T const*>
    static constexpr ranges::range_difference_t<R>
    operator()(R&& r, T const& value, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), value, std::move(proj));
    }

    template<
        ranges::forward_range R, class Proj = identity,
        class T = projected_value_t<ranges::iterator_t<R>, Proj>>
    requires indirect_binary_predicate<
        ranges::equal_to, projected<ranges::iterator_t<R>, Proj>, T const*>
    static constexpr ranges::range_difference_t<R>
    operator()(R&& r, T const& value, Proj proj = {}) {
        return operator()(
            ranges::begin(r), ranges::next(ranges::begin(r), ranges::end(r)), value, std::move(proj)
        );
    }
} count;

inline constexpr struct __count_if_fn {
    template<
        input_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    static constexpr iter_difference_t<I> operator()(I first, S last, Pred pred, Proj proj = {}) {
        iter_difference_t<I> counter = 0;
        for (; first != last; ++first)
            if (std::invoke(pred, std::invoke(proj, *first))) ++counter;
        return counter;
    }

    template<
        input_range R, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    static constexpr range_difference_t<R> operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(pred), std::move(proj));
    }

    template<
        ranges::forward_range R, class Proj = identity,
        indirect_unary_predicate<projected<ranges::iterator_t<R>, Proj>> Pred>
    static constexpr ranges::range_difference_t<R> operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(
            ranges::begin(r), ranges::next(ranges::begin(r), ranges::end(r)), std::move(pred),
            std::move(proj)
        );
    }
} count_if;

}  // namespace std::ranges
