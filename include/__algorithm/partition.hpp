#pragma once

#include <__algorithm/mutating.hpp>
#include <__ranges/core.hpp>
#include <__ranges/result_types.hpp>

namespace std::ranges {

// =============================================================================
// Result Types
// =============================================================================

template<class I, class O1, class O2> struct partition_copy_result {
    [[no_unique_address]] I in;
    [[no_unique_address]] O1 out1;
    [[no_unique_address]] O2 out2;

    template<class II, class OO1, class OO2> requires convertible_to<I const&, II>
                                                   && convertible_to<const O1&, OO1>
                                                   && convertible_to<const O2&, OO2>
    constexpr operator partition_copy_result<II, OO1, OO2>() const& {
        return {in, out1, out2};
    }

    template<class II, class OO1, class OO2>
    requires convertible_to<I, II> && convertible_to<O1, OO1> && convertible_to<O2, OO2>
    constexpr operator partition_copy_result<II, OO1, OO2>() && {
        return {std::move(in), std::move(out1), std::move(out2)};
    }
};

// =============================================================================
// ranges::is_partitioned
// =============================================================================

inline constexpr struct __is_partitioned_fn {
    template<
        input_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    static constexpr bool operator()(I first, S last, Pred pred, Proj proj = {}) {
        // Skip elements that satisfy the predicate
        while (first != last && std::invoke(pred, std::invoke(proj, *first))) { ++first; }
        // Ensure no remaining elements satisfy the predicate
        while (first != last) {
            if (std::invoke(pred, std::invoke(proj, *first))) { return false; }
            ++first;
        }
        return true;
    }

    template<
        input_range R, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    static constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(pred), std::move(proj));
    }
} is_partitioned;

// =============================================================================
// ranges::partition
// =============================================================================

inline constexpr struct __partition_fn {
    template<
        permutable I, sentinel_for<I> S, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    static constexpr subrange<I> operator()(I first, S last, Pred pred, Proj proj = {}) {
        // Fast-forward to the first element that violates the predicate
        while (first != last && std::invoke(pred, std::invoke(proj, *first))) { ++first; }

        if (first == last) { return {first, first}; }

        if constexpr (bidirectional_iterator<I> && same_as<I, S>) {
            // Bidirectional fast path: Two-pointer Hoare-style partitioning (minimizes swaps)
            I tail = last;
            while (true) {
                --tail;
                while (first != tail && !std::invoke(pred, std::invoke(proj, *tail))) { --tail; }
                if (first == tail) { break; }

                ranges::iter_swap(first, tail);
                ++first;

                while (first != tail && std::invoke(pred, std::invoke(proj, *first))) { ++first; }
                if (first == tail) { break; }
            }
            return {first, last};
        } else {
            // Forward iterator fallback: Single-pass Lomuto-style partition
            for (I cur = ranges::next(first); cur != last; ++cur) {
                if (std::invoke(pred, std::invoke(proj, *cur))) {
                    ranges::iter_swap(first, cur);
                    ++first;
                }
            }
            auto end_it = ranges::next(first, last);
            return {first, std::move(end_it)};
        }
    }

    template<
        forward_range R, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    requires permutable<iterator_t<R>>
    static constexpr borrowed_subrange_t<R> operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(pred), std::move(proj));
    }
} partition;

// =============================================================================
// ranges::partition_point
// =============================================================================

inline constexpr struct __partition_point_fn {
    template<
        forward_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    static constexpr I operator()(I first, S last, Pred pred, Proj proj = {}) {
        auto len = ranges::distance(first, last);

        // Binary search for the first boundary element where pred evaluates to false
        while (len > 0) {
            auto half = len / 2;
            auto mid  = ranges::next(first, half);

            if (std::invoke(pred, std::invoke(proj, *mid))) {
                first  = ++mid;
                len   -= half + 1;
            } else {
                len = half;
            }
        }
        return first;
    }

    template<
        forward_range R, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    static constexpr borrowed_iterator_t<R> operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(pred), std::move(proj));
    }
} partition_point;

// =============================================================================
// ranges::partition_copy
// =============================================================================

inline constexpr struct __partition_copy_fn {
    template<
        input_iterator I, sentinel_for<I> S, weakly_incrementable O1, weakly_incrementable O2,
        class Proj = identity, indirect_unary_predicate<projected<I, Proj>> Pred>
    requires indirectly_copyable<I, O1> && indirectly_copyable<I, O2>
    static constexpr partition_copy_result<I, O1, O2>
    operator()(I first, S last, O1 out_true, O2 out_false, Pred pred, Proj proj = {}) {
        while (first != last) {
            if (std::invoke(pred, std::invoke(proj, *first))) {
                *out_true = *first;
                ++out_true;
            } else {
                *out_false = *first;
                ++out_false;
            }
            ++first;
        }
        return {std::move(first), std::move(out_true), std::move(out_false)};
    }

    template<
        input_range R, weakly_incrementable O1, weakly_incrementable O2, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    requires indirectly_copyable<iterator_t<R>, O1> && indirectly_copyable<iterator_t<R>, O2>
    static constexpr partition_copy_result<borrowed_iterator_t<R>, O1, O2>
    operator()(R&& r, O1 out_true, O2 out_false, Pred pred, Proj proj = {}) {
        return operator()(
            ranges::begin(r), ranges::end(r), std::move(out_true), std::move(out_false),
            std::move(pred), std::move(proj)
        );
    }
} partition_copy;

// =============================================================================
// ranges::stable_partition
// =============================================================================

namespace __detail {

template<class I, class Pred, class Proj>
constexpr I __stable_partition_inplace(I first, iter_difference_t<I> len, Pred& pred, Proj& proj) {
    if (len == 0) return first;
    if (len == 1) { return first + (std::invoke(pred, std::invoke(proj, *first)) ? 1 : 0); }

    auto half = len / 2;
    I mid     = ranges::next(first, half);

    // Recursively partition both halves
    I p1 = __stable_partition_inplace(first, half, pred, proj);
    I p2 = __stable_partition_inplace(mid, len - half, pred, proj);

    // Rotate false-elements of the left half past the true-elements of the right half:
    // [first ... p1 (false) ... mid ... p2 (true) ... last]
    // Rotating [p1, mid, p2) brings [mid, p2) before [p1, mid).
    auto new_mid = ranges::rotate(p1, mid, p2);

    return new_mid.begin();
}

}  // namespace __detail

inline constexpr struct __stable_partition_fn {
    template<
        bidirectional_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    requires permutable<I>
    static constexpr subrange<I> operator()(I first, S last, Pred pred, Proj proj = {}) {
        auto end_it = ranges::next(first, last);
        auto len    = ranges::distance(first, end_it);

        I split = __detail::__stable_partition_inplace(first, len, pred, proj);
        return {split, end_it};
    }

    template<
        bidirectional_range R, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    requires permutable<iterator_t<R>>
    static constexpr borrowed_subrange_t<R> operator()(R&& r, Pred pred, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(pred), std::move(proj));
    }
} stable_partition;

}  // namespace std::ranges
