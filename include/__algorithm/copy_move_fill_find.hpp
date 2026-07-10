#pragma once
// code: language=c++
// IWYU pragma: private: include <algorithm>

#include <compare>
#include <functional>
#include <__iterator/concepts.hpp>
#include <__memory/base.hpp>
#include <__ranges/core.hpp>
#include <__ranges/result_types.hpp>
#include <__ranges/subrange.hpp>

#include <concepts>
#include <cstddef>
#include <cstring>
#include <optional>
#include <type_traits>

namespace std::ranges {

template<class I, class F> using for_each_result      = ranges::in_fun_result<I, F>;
template<class I, class O> using copy_n_result        = ranges::in_out_result<I, O>;
template<class I, class O> using copy_result          = ranges::in_out_result<I, O>;
template<class I, class O> using copy_backward_result = ranges::in_out_result<I, O>;
template<class I, class O> using move_backward_result = ranges::in_out_result<I, O>;
template<class I, class O> using reverse_copy_result  = ranges::in_out_result<I, O>;
template<class I, class O> using move_n_result        = ranges::in_out_result<I, O>;
template<class I, class O> using move_result          = ranges::in_out_result<I, O>;
template<class I1, class I2> using mismatch_result    = ranges::in_in_result<I1, I2>;

inline namespace __cpo {

inline constexpr struct __for_each {
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
        return operator()(ranges::begin(r), ranges::end(r), std::move(f), std::ref(proj));
    }
} for_each;

template<class I, class O>
concept __can_memcpy = contiguous_iterator<I> && contiguous_iterator<O>
                    && same_as<remove_cv_t<iter_value_t<I>>, iter_value_t<O>>
                    && is_trivially_copyable_v<iter_value_t<O>>;

inline constexpr struct __fill_fn {
    template<class O, std::sentinel_for<O> S, class T = std::iter_value_t<O>>
    requires std::output_iterator<O, T const&>
    static constexpr O operator()(O first, S last, T const& value) {
        if constexpr (contiguous_iterator<O> && sized_sentinel_for<O, S>
                      && is_arithmetic_v<iter_value_t<O>>) {
            if (value == 0) {
                ::memset(std::to_address(first, 0, sizeof(iter_value_t<O>) * last - first));
                return first + (last - first);
            }
        }

        while (first != last) *first++ = value;

        return first;
    }

    template<class R, class T = ranges::range_value_t<R>> requires ranges::output_range<R, T const&>
    static constexpr ranges::borrowed_iterator_t<R> operator()(R&& r, T const& value) {
        return operator()(ranges::begin(r), ranges::end(r), value);
    }
} fill;

inline constexpr struct __fill_n_fn {
    template<class O, class T = std::iter_value_t<O>> requires std::output_iterator<O, T const&>
    static constexpr O operator()(O first, std::iter_difference_t<O> n, T const& value) {
        if !consteval {
            if constexpr (contiguous_iterator<O> && is_arithmetic_v<iter_value_t<O>>) {
                if (value == 0 || sizeof(iter_value_t<O>) == 1) {
                    __builtin_memset(
                        std::to_address(first), value, size_t(n) * sizeof(iter_value_t<O>)
                    );
                    return first + n;
                }
            }
        }
        for (std::iter_difference_t<O> i{}; i != n; ++first, ++i) *first = value;
        return first;
    }
} fill_n;

inline constexpr struct __copy_n_fn {
    template<input_iterator I, weakly_incrementable O> requires indirectly_copyable<I, O>
    static constexpr ranges::copy_n_result<I, O>
    operator()(I first, iter_difference_t<I> n, O result) {
        if constexpr (__can_memcpy<I, O>) {
            ::memmove(
                std::to_address(result), std::to_address(first), size_t(n) * sizeof(iter_value_t<O>)
            );
            return {first + n, result + n};
        }
        for (iter_difference_t<I> i{}; i != n; ++i, ++first, ++result) *result = *first;
        return {std::move(first), std::move(result)};
    }
} copy_n;

inline constexpr struct __copy_fn {
    template<input_iterator I, sentinel_for<I> S, weakly_incrementable O>
    requires indirectly_copyable<I, O>
    static constexpr copy_result<I, O> operator()(I first, S last, O result) {
        if constexpr (__can_memcpy<I, O> && sized_sentinel_for<S, I>) {
            return copy_n(first, last - first, result);
        }

        for (; first != last; ++first, (void)++result) *result = *first;
        return {std::move(first), std::move(result)};
    }

    template<ranges::input_range R, std::weakly_incrementable O>
    requires std::indirectly_copyable<ranges::iterator_t<R>, O>
    static constexpr ranges::copy_result<ranges::borrowed_iterator_t<R>, O>
    operator()(R&& r, O result) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(result));
    }
} copy;

inline constexpr struct __reverse_copy_fn {
    template<bidirectional_iterator I, sentinel_for<I> S, weakly_incrementable O>
    requires indirectly_copyable<I, O>
    static constexpr ranges::reverse_copy_result<I, O> operator()(I first, S last, O result) {
        auto i    = ranges::next(first, last);
        auto tail = i;
        while (first != tail) {
            --tail;
            *result = *tail;
            ++result;
        }
        return {i, std::move(result)};
    }

    template<ranges::bidirectional_range R, std::weakly_incrementable O>
    requires std::indirectly_copyable<ranges::iterator_t<R>, O>
    static constexpr ranges::reverse_copy_result<ranges::borrowed_iterator_t<R>, O>
    operator()(R&& r, O result) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(result));
    }
} reverse_copy;

inline constexpr struct {
    template<bidirectional_iterator I, sentinel_for<I> S> requires permutable<I>
    static constexpr I operator()(I first, S last) {
        auto last2{ranges::next(first, last)};
        for (auto tail{last2}; !(first == tail || first == --tail); ++first)
            ranges::iter_swap(first, tail);
        return last2;
    }

    template<bidirectional_range R> requires permutable<iterator_t<R>>
    static constexpr borrowed_iterator_t<R> operator()(R&& r) {
        return operator()(ranges::begin(r), ranges::end(r));
    }
} reverse;

inline constexpr struct __move_fn {
    template<std::input_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O>
    requires std::indirectly_movable<I, O>
    static constexpr ranges::move_result<I, O> operator()(I first, S last, O result) {
        if (__can_memcpy<I, O> && sized_sentinel_for<S, I>) {
            return copy_n(first, last - first, result);
        }
        for (; first != last; ++first, ++result) *result = ranges::iter_move(first);
        return {std::move(first), std::move(result)};
    }
    template<ranges::input_range R, std::weakly_incrementable O>
    requires std::indirectly_movable<ranges::iterator_t<R>, O>
    static constexpr ranges::move_result<ranges::borrowed_iterator_t<R>, O>
    operator()(R&& r, O result) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(result));
    }
} move;

inline constexpr struct __copy_backward_fn {
    template<
        std::bidirectional_iterator I1, std::sentinel_for<I1> S1, std::bidirectional_iterator I2>
    requires std::indirectly_copyable<I1, I2>
    static constexpr ranges::copy_backward_result<I1, I2> operator()(I1 first, S1 last, I2 result) {
        if (__can_memcpy<I1, I2> && sized_sentinel_for<S1, I1>) {
            auto n = last - first;
            ::memmove(
                std::to_address(result) - n, std::to_address(first), n * sizeof(iter_value_t<I1>)
            );
            return {first + n, result - n};
        }
        I1 last1{ranges::next(first, std::move(last))};
        for (I1 i{last1}; i != first;) *--result = *--i;
        return {std::move(last1), std::move(result)};
    }

    template<ranges::bidirectional_range R, std::bidirectional_iterator I>
    requires std::indirectly_copyable<ranges::iterator_t<R>, I>
    static constexpr ranges::copy_backward_result<ranges::borrowed_iterator_t<R>, I>
    operator()(R&& r, I result) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(result));
    }
} copy_backward;

inline constexpr struct __move_backward_fn {
    template<
        std::bidirectional_iterator I1, std::sentinel_for<I1> S1, std::bidirectional_iterator I2>
    requires std::indirectly_movable<I1, I2>
    static constexpr ranges::move_backward_result<I1, I2> operator()(I1 first, S1 last, I2 result) {
        if constexpr (__can_memcpy<I1, I2> && sized_sentinel_for<S1, I1>) {
            return copy_backward(first, last, result);
        }
        auto i{last};
        for (; i != first; *--result = ranges::iter_move(--i)) {}
        return {std::move(last), std::move(result)};
    }

    template<ranges::bidirectional_range R, std::bidirectional_iterator I>
    requires std::indirectly_movable<ranges::iterator_t<R>, I>
    static constexpr ranges::move_backward_result<ranges::borrowed_iterator_t<R>, I>
    operator()(R&& r, I result) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(result));
    }
} move_backward;

inline constexpr struct {
    template<
        input_iterator I, sentinel_for<I> S, class Proj = identity,
        class T = projected_value_t<I, Proj>>
    requires indirect_binary_predicate<equal_to, projected<I, Proj>, T const*>
    static constexpr I operator()(I first, S last, T const& value, Proj proj = {}) {
        for (; first != last; ++first)
            if (std::invoke(proj, *first) == value) return first;
        return first;
    }

    template<ranges::input_range R, class T, class Proj = std::identity>
    requires std::indirect_binary_predicate<
        ranges::equal_to, std::projected<ranges::iterator_t<R>, Proj>, T const*>
    static constexpr ranges::borrowed_iterator_t<R>
    operator()(R&& r, T const& value, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), value, std::ref(proj));
    }
} find;

inline constexpr struct {
    template<
        std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
        class T = std::projected_value_t<iterator_t<I>, Proj>>
    requires std::indirect_binary_predicate<ranges::equal_to, std::projected<I, Proj>, T const*>
    static constexpr ranges::subrange<I>
    operator()(I first, S last, T const& value, Proj proj = {}) {
        // Note: if I is mere forward_iterator, we may only go from begin to end.
        std::optional<I> found;
        for (; first != last; ++first)
            if (std::invoke(proj, *first) == value) found = first;

        if (!found) return {first, first};

        return {*found, std::ranges::next(*found, last)};
    }

    template<
        ranges::forward_range R, class Proj = std::identity,
        class T = std::projected_value_t<iterator_t<R>, Proj>>
    requires std::indirect_binary_predicate<
        ranges::equal_to, std::projected<ranges::iterator_t<R>, Proj>, T const*>
    static constexpr ranges::borrowed_subrange_t<R>
    operator()(R&& r, T const& value, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), value, std::ref(proj));
    }
} find_last;

inline constexpr struct {
    template<
        std::forward_iterator I1, std::sentinel_for<I1> S1, std::forward_iterator I2,
        std::sentinel_for<I2> S2, class Pred = ranges::equal_to, class Proj1 = std::identity,
        class Proj2 = std::identity>
    requires std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
    static constexpr ranges::subrange<I1> operator()(
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
        ranges::forward_range R1, ranges::forward_range R2, class Pred = ranges::equal_to,
        class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<
        ranges::iterator_t<R1>, ranges::iterator_t<R2>, Pred, Proj1, Proj2>
    static constexpr ranges::borrowed_subrange_t<R1>
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::move(pred),
            std::move(proj1), std::move(proj2)
        );
    }
} search;

inline constexpr struct {
    template<
        std::forward_iterator I1, std::sentinel_for<I1> S1, std::forward_iterator I2,
        std::sentinel_for<I2> S2, class Pred = ranges::equal_to, class Proj1 = std::identity,
        class Proj2 = std::identity>
    requires std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
    static constexpr ranges::subrange<I1> operator()(
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
        ranges::forward_range R1, ranges::forward_range R2, class Pred = ranges::equal_to,
        class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<
        ranges::iterator_t<R1>, ranges::iterator_t<R2>, Pred, Proj1, Proj2>
    static constexpr ranges::borrowed_subrange_t<R1>
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::move(pred),
            std::move(proj1), std::move(proj2)
        );
    }
} find_end;

inline struct find_first_of_fn {
    template<
        std::input_iterator I1, std::sentinel_for<I1> S1, std::forward_iterator I2,
        std::sentinel_for<I2> S2, class Pred = ranges::equal_to, class Proj1 = std::identity,
        class Proj2 = std::identity>
    requires std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2> static constexpr I1 operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        for (; first1 != last1; ++first1)
            for (auto i = first2; i != last2; ++i)
                if (std::invoke(pred, std::invoke(proj1, *first1), std::invoke(proj2, *i)))
                    return first1;
        return first1;
    }

    template<
        ranges::input_range R1, ranges::forward_range R2, class Pred = ranges::equal_to,
        class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<
        ranges::iterator_t<R1>, ranges::iterator_t<R2>, Pred, Proj1, Proj2>
    static constexpr ranges::borrowed_iterator_t<R1>
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::move(pred),
            std::move(proj1), std::move(proj2)
        );
    }
} find_first_of;

inline constexpr struct {
    template<
        std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
        class T = std::projected_value_t<I, Proj>>
    requires std::indirect_binary_predicate<ranges::equal_to, std::projected<I, Proj>, T const*>
    static constexpr bool operator()(I first, S last, T const& value, Proj proj = {}) {
        return ranges::find(std::move(first), last, value, proj) != last;
    }

    template<
        ranges::input_range R, class Proj = std::identity,
        class T = std::projected_value_t<ranges::iterator_t<R>, Proj>>
    requires std::indirect_binary_predicate<
        ranges::equal_to, std::projected<ranges::iterator_t<R>, Proj>, T const*>
    static constexpr bool operator()(R&& r, T const& value, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(value), proj);
    }
} contains;

inline constexpr struct {
    template<
        std::forward_iterator I1, std::sentinel_for<I1> S1, std::forward_iterator I2,
        std::sentinel_for<I2> S2, class Pred = ranges::equal_to, class Proj1 = std::identity,
        class Proj2 = std::identity>
    requires std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
    static constexpr bool operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        return (first2 == last2)
            || !ranges::search(first1, last1, first2, last2, pred, proj1, proj2).empty();
    }

    template<
        ranges::forward_range R1, ranges::forward_range R2, class Pred = ranges::equal_to,
        class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<
        ranges::iterator_t<R1>, ranges::iterator_t<R2>, Pred, Proj1, Proj2> static constexpr bool
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::move(pred),
            std::move(proj1), std::move(proj2)
        );
    }
} contains_subrange;

inline constexpr struct {
    template<
        std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2,
        std::sentinel_for<I2> S2, class Pred = ranges::equal_to, class Proj1 = std::identity,
        class Proj2 = std::identity>
    requires std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
    static constexpr mismatch_result<I1, I2> operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        for (; first1 != last1 && first2 != last2; ++first1, (void)++first2)
            if (not std::invoke(pred, std::invoke(proj1, *first1), std::invoke(proj2, *first2)))
                break;

        return {first1, first2};
    }

    template<
        ranges::input_range R1, ranges::input_range R2, class Pred = ranges::equal_to,
        class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<
        ranges::iterator_t<R1>, ranges::iterator_t<R2>, Pred, Proj1, Proj2>
    static constexpr ranges::mismatch_result<
        ranges::borrowed_iterator_t<R1>, ranges::borrowed_iterator_t<R2>>
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::ref(pred),
            std::ref(proj1), std::ref(proj2)
        );
    }
} mismatch;

inline constexpr struct {
    template<
        std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2,
        std::sentinel_for<I2> S2, class Pred = ranges::equal_to, class Proj1 = std::identity,
        class Proj2 = std::identity>
    requires std::indirectly_comparable<I1, I2, Pred, Proj1, Proj2>
    static constexpr bool operator()(
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
        ranges::input_range R1, ranges::input_range R2, class Pred = ranges::equal_to,
        class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_comparable<
        ranges::iterator_t<R1>, ranges::iterator_t<R2>, Pred, Proj1, Proj2> static constexpr bool
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::move(pred),
            std::move(proj1), std::move(proj2)
        );
    }
} starts_with;

inline constexpr struct {
    template<
        input_iterator I1, sentinel_for<I1> S1, input_iterator I2, sentinel_for<I2> S2,
        class Pred = ranges::equal_to, class Proj1 = identity, class Proj2 = identity>
    requires indirectly_comparable<I1, I2, Pred, Proj1, Proj2> static constexpr bool operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        if constexpr (sized_sentinel_for<S1, I1> and sized_sentinel_for<S2, I2>)
            if (ranges::distance(first1, last1) != ranges::distance(first2, last2)) return false;

        for (; first1 != last1; ++first1, (void)++first2)
            if (!std::invoke(pred, std::invoke(proj1, *first1), std::invoke(proj2, *first2)))
                return false;
        return true;
    }

    template<
        input_range R1, input_range R2, class Pred = equal_to, class Proj1 = identity,
        class Proj2 = identity>
    requires indirectly_comparable<iterator_t<R1>, iterator_t<R2>, Pred, Proj1, Proj2>
    static constexpr bool
    operator()(R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            begin(r1), end(r1), begin(r2), end(r2), std::ref(pred), std::ref(proj1), std::ref(proj2)
        );
    }
} equal;

inline constexpr struct {
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
        return operator()(ranges::begin(r), ranges::end(r), std::ref(pred), std::ref(proj));
    }
} find_if;

inline constexpr struct {
    template<
        permutable I, sentinel_for<I> S, class Proj = identity,
        class T = projected_value_t<I, Proj>>
    requires indirect_binary_predicate<ranges::equal_to, projected<I, Proj>, T const*>
    static constexpr subrange<I> operator()(I first, S last, T const& value, Proj proj = {}) {
        first = ranges::find(std::move(first), last, value, proj);
        if (first != last) {
            for (I i{std::next(first)}; i != last; ++i)
                if (value != std::invoke(proj, *i)) {
                    *first = ranges::iter_move(i);
                    ++first;
                }
        }
        return {first, last};
    }

    template<
        forward_range R, class Proj = identity, class T = projected_value_t<iterator_t<R>, Proj>>
    requires permutable<iterator_t<R>>
          && indirect_binary_predicate<ranges::equal_to, projected<iterator_t<R>, Proj>, T const*>
    static constexpr borrowed_subrange_t<R> operator()(R&& r, T const& value, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), value, std::move(proj));
    }
} remove;

inline constexpr struct {
    template<
        permutable I, sentinel_for<I> S, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    static constexpr subrange<I> operator()(I first, S last, Pred pred, Proj proj = {}) {
        first = ranges::find_if(std::move(first), last, pred, proj);
        if (first != last) {
            for (I i{std::next(first)}; i != last; ++i)
                if (!std::invoke(pred, std::invoke(proj, *i))) {
                    *first = ranges::iter_move(i);
                    ++first;
                }
        }
        return {first, last};
    }

    template<
        forward_range R, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    requires permutable<iterator_t<R>>
    static constexpr borrowed_subrange_t<R> operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), pred, std::move(proj));
    }
} remove_if;

}  // namespace __cpo

}  // namespace std::ranges

namespace std {

template<class I1, class I2, class Cmp>
constexpr auto lexicographical_compare_three_way(I1 f1, I1 l1, I2 f2, I2 l2, Cmp comp)
    -> decltype(comp(*f1, *f2)) {
    using ret_t = decltype(comp(*f1, *f2));
    static_assert(
        disjunction_v<
            is_same<ret_t, strong_ordering>, is_same<ret_t, weak_ordering>,
            is_same<ret_t, partial_ordering>>,
        "The return type must be a comparison category type."
    );

    bool exhaust1 = (f1 == l1);
    bool exhaust2 = (f2 == l2);
    for (; !exhaust1 && !exhaust2; exhaust1 = (++f1 == l1), exhaust2 = (++f2 == l2))
        if (auto c = comp(*f1, *f2); c != 0) return c;

    return !exhaust1 ? strong_ordering::greater
         : !exhaust2 ? strong_ordering::less
                     : strong_ordering::equal;
}

template<class InputIt1, class InputIt2>

constexpr auto lexicographical_compare_three_way(
    InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2
) {
    return lexicographical_compare_three_way(first1, last1, first2, last2, compare_three_way{});
}

}  // namespace std
