#pragma once

#include <__ranges/core.hpp>
#include <__ranges/result_types.hpp>
#include <__algorithm/mutating.hpp>

namespace std::ranges {

// =============================================================================
// Result Types
// =============================================================================

// =============================================================================
// ranges::is_permutation
// =============================================================================

inline constexpr struct __is_permutation_fn {
    template<
        forward_iterator I1, sentinel_for<I1> S1,
        forward_iterator I2, sentinel_for<I2> S2,
        class Proj1 = identity, class Proj2 = identity,
        indirect_equivalence_relation<projected<I1, Proj1>, projected<I2, Proj2>> Pred = ranges::equal_to>
    static constexpr bool operator()(
        I1 first1, S1 last1, I2 first2, S2 last2,
        Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        // Fast path 1: If both ranges are sized, lengths must match
        if constexpr (sized_sentinel_for<S1, I1> && sized_sentinel_for<S2, I2>) {
            if ((last1 - first1) != (last2 - first2)) {
                return false;
            }
        }

        // Fast path 2: Skip identical prefix
        while (first1 != last1 && first2 != last2 &&
               std::invoke(pred, std::invoke(proj1, *first1), std::invoke(proj2, *first2))) {
            ++first1;
            ++first2;
        }

        if (first1 == last1) return first2 == last2;
        if (first2 == last2) return false;

        // If not sized, verify remaining tails have equal length
        if constexpr (!sized_sentinel_for<S1, I1> || !sized_sentinel_for<S2, I2>) {
            if (ranges::distance(first1, last1) != ranges::distance(first2, last2)) {
                return false;
            }
        }

        // Step through remaining elements and verify frequency matches
        for (auto it = first1; it != last1; ++it) {
            auto it_proj = std::invoke(proj1, *it);

            // Avoid recounting if *it was already checked earlier in [first1, it)
            bool already_seen = false;
            for (auto prev = first1; prev != it; ++prev) {
                if (std::invoke(pred, std::invoke(proj1, *prev), it_proj)) {
                    already_seen = true;
                    break;
                }
            }
            if (already_seen) continue;

            // Count occurrences of *it in the rest of range 1
            iter_difference_t<I1> count1 = 0;
            for (auto scan1 = it; scan1 != last1; ++scan1) {
                if (std::invoke(pred, std::invoke(proj1, *scan1), it_proj)) {
                    ++count1;
                }
            }

            // Count matching occurrences in range 2
            iter_difference_t<I2> count2 = 0;
            for (auto scan2 = first2; scan2 != last2; ++scan2) {
                if (std::invoke(pred, it_proj, std::invoke(proj2, *scan2))) {
                    ++count2;
                }
            }

            if (count1 != count2) {
                return false;
            }
        }

        return true;
    }

    template<
        forward_range R1, forward_range R2,
        class Proj1 = identity, class Proj2 = identity,
        indirect_equivalence_relation<
            projected<iterator_t<R1>, Proj1>,
            projected<iterator_t<R2>, Proj2>> Pred = ranges::equal_to>
    static constexpr bool operator()(
        R1&& r1, R2&& r2, Pred pred = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        return operator()(
            ranges::begin(r1), ranges::end(r1),
            ranges::begin(r2), ranges::end(r2),
            std::move(pred), std::move(proj1), std::move(proj2)
        );
    }
} is_permutation;

// =============================================================================
// ranges::next_permutation
// =============================================================================

inline constexpr struct __next_permutation_fn {
    template<
        bidirectional_iterator I, sentinel_for<I> S,
        class Comp = ranges::less, class Proj = identity>
    requires permutable<I> && indirect_strict_weak_order<Comp, projected<I, Proj>>
    static constexpr next_permutation_result<I> operator()(
        I first, S last, Comp comp = {}, Proj proj = {}
    ) {
        auto end_it = ranges::next(first, last);
        if (first == end_it) {
            return {std::move(end_it), false};
        }

        I i = end_it;
        --i;
        if (i == first) {
            return {std::move(end_it), false};
        }

        while (true) {
            I next_i = i;
            --i;

            // Find the rightmost inversion where *i < *(i + 1)
            if (std::invoke(comp, std::invoke(proj, *i), std::invoke(proj, *next_i))) {
                // Find the rightmost element *j greater than *i
                I j = end_it;
                do {
                    --j;
                } while (!std::invoke(comp, std::invoke(proj, *i), std::invoke(proj, *j)));

                ranges::iter_swap(i, j);
                ranges::reverse(next_i, end_it);
                return {std::move(end_it), true};
            }

            if (i == first) {
                // Entire range was in descending order -> wrap to lowest permutation
                ranges::reverse(first, end_it);
                return {std::move(end_it), false};
            }
        }
    }

    template<random_access_range R, class Comp = ranges::less, class Proj = identity>
    requires permutable<iterator_t<R>>
          && indirect_strict_weak_order<Comp, projected<iterator_t<R>, Proj>>
    static constexpr next_permutation_result<borrowed_iterator_t<R>>
    operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(comp), std::move(proj));
    }
} next_permutation;

// =============================================================================
// ranges::prev_permutation
// =============================================================================

inline constexpr struct __prev_permutation_fn {
    template<
        bidirectional_iterator I, sentinel_for<I> S,
        class Comp = ranges::less, class Proj = identity>
    requires permutable<I> && indirect_strict_weak_order<Comp, projected<I, Proj>>
    static constexpr prev_permutation_result<I> operator()(
        I first, S last, Comp comp = {}, Proj proj = {}
    ) {
        auto end_it = ranges::next(first, last);
        if (first == end_it) {
            return {std::move(end_it), false};
        }

        I i = end_it;
        --i;
        if (i == first) {
            return {std::move(end_it), false};
        }

        while (true) {
            I next_i = i;
            --i;

            // Find the rightmost pair where *i > *(i + 1)
            if (std::invoke(comp, std::invoke(proj, *next_i), std::invoke(proj, *i))) {
                // Find the rightmost element *j smaller than *i
                I j = end_it;
                do {
                    --j;
                } while (!std::invoke(comp, std::invoke(proj, *j), std::invoke(proj, *i)));

                ranges::iter_swap(i, j);
                ranges::reverse(next_i, end_it);
                return {std::move(end_it), true};
            }

            if (i == first) {
                // Entire range was in ascending order -> wrap to highest permutation
                ranges::reverse(first, end_it);
                return {std::move(end_it), false};
            }
        }
    }

    template<random_access_range R, class Comp = ranges::less, class Proj = identity>
    requires permutable<iterator_t<R>>
          && indirect_strict_weak_order<Comp, projected<iterator_t<R>, Proj>>
    static constexpr prev_permutation_result<borrowed_iterator_t<R>>
    operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(comp), std::move(proj));
    }
} prev_permutation;

}  // namespace std::ranges