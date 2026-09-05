#pragma once

#include <__ranges/core.hpp>
#include <__ranges/result_types.hpp>
#include <__algorithm/mutating.hpp>

namespace std::ranges {

// =============================================================================
// ranges::includes
// =============================================================================

inline constexpr struct __includes_fn {
    template<
        input_iterator I1, sentinel_for<I1> S1, input_iterator I2, sentinel_for<I2> S2,
        class Proj1 = identity, class Proj2 = identity,
        indirect_strict_weak_order<projected<I1, Proj1>, projected<I2, Proj2>> Comp = ranges::less>
    static constexpr bool operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        while (first1 != last1 && first2 != last2) {
            if (std::invoke(comp, std::invoke(proj2, *first2), std::invoke(proj1, *first1))) {
                // Element in R2 is smaller than current element in R1 -> R1 does not contain it
                return false;
            }
            if (std::invoke(comp, std::invoke(proj1, *first1), std::invoke(proj2, *first2))) {
                ++first1;
            } else {
                // Equivalent element found; advance both
                ++first1;
                ++first2;
            }
        }
        return first2 == last2;
    }

    template<
        input_range R1, input_range R2, class Proj1 = identity, class Proj2 = identity,
        indirect_strict_weak_order<
            projected<iterator_t<R1>, Proj1>, projected<iterator_t<R2>, Proj2>>
            Comp = ranges::less>
    static constexpr bool
    operator()(R1&& r1, R2&& r2, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2), std::move(comp),
            std::move(proj1), std::move(proj2)
        );
    }
} includes;

// =============================================================================
// ranges::set_union
// =============================================================================

inline constexpr struct __set_union_fn {
    template<
        input_iterator I1, sentinel_for<I1> S1, input_iterator I2, sentinel_for<I2> S2,
        weakly_incrementable O, class Comp = ranges::less, class Proj1 = identity,
        class Proj2 = identity>
    requires mergeable<I1, I2, O, Comp, Proj1, Proj2>
    static constexpr set_union_result<I1, I2, O> operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, O result, Comp comp = {}, Proj1 proj1 = {},
        Proj2 proj2 = {}
    ) {
        while (first1 != last1 && first2 != last2) {
            if (std::invoke(comp, std::invoke(proj1, *first1), std::invoke(proj2, *first2))) {
                *result = *first1;
                ++first1;
            } else if (
                std::invoke(comp, std::invoke(proj2, *first2), std::invoke(proj1, *first1))
            ) {
                *result = *first2;
                ++first2;
            } else {
                *result = *first1;
                ++first1;
                ++first2;
            }
            ++result;
        }

        auto res1 = ranges::copy(std::move(first1), std::move(last1), std::move(result));
        auto res2 = ranges::copy(std::move(first2), std::move(last2), std::move(res1.out));

        return {std::move(res1.in), std::move(res2.in), std::move(res2.out)};
    }

    template<
        input_range R1, input_range R2, weakly_incrementable O, class Comp = ranges::less,
        class Proj1 = identity, class Proj2 = identity>
    requires mergeable<iterator_t<R1>, iterator_t<R2>, O, Comp, Proj1, Proj2>
    static constexpr set_union_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>, O>
    operator()(R1&& r1, R2&& r2, O result, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2),
            std::move(result), std::move(comp), std::move(proj1), std::move(proj2)
        );
    }
} set_union;

// =============================================================================
// ranges::set_intersection
// =============================================================================

inline constexpr struct __set_intersection_fn {
    template<
        input_iterator I1, sentinel_for<I1> S1, input_iterator I2, sentinel_for<I2> S2,
        weakly_incrementable O, class Comp = ranges::less, class Proj1 = identity,
        class Proj2 = identity>
    requires mergeable<I1, I2, O, Comp, Proj1, Proj2>
    static constexpr set_intersection_result<I1, I2, O> operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, O result, Comp comp = {}, Proj1 proj1 = {},
        Proj2 proj2 = {}
    ) {
        while (!(first1 == last1 or first2 == last2)) {
            if (std::invoke(comp, std::invoke(proj1, *first1), std::invoke(proj2, *first2)))
                ++first1;
            else if (std::invoke(comp, std::invoke(proj2, *first2), std::invoke(proj1, *first1)))
                ++first2;
            else
                *result = *first1, ++first1, ++first2, ++result;
        }
        return {
            ranges::next(std::move(first1), std::move(last1)),
            ranges::next(std::move(first2), std::move(last2)), std::move(result)
        };
    }

    template<
        input_range R1, input_range R2, weakly_incrementable O, class Comp = ranges::less,
        class Proj1 = identity, class Proj2 = identity>
    requires mergeable<iterator_t<R1>, iterator_t<R2>, O, Comp, Proj1, Proj2>
    static constexpr set_intersection_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>, O>
    operator()(R1&& r1, R2&& r2, O result, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2),
            std::move(result), std::move(comp), std::move(proj1), std::move(proj2)
        );
    }
} set_intersection;

// =============================================================================
// ranges::set_difference
// =============================================================================

inline constexpr struct __set_difference_fn {
    template<
        input_iterator I1, sentinel_for<I1> S1, input_iterator I2, sentinel_for<I2> S2,
        weakly_incrementable O, class Comp = ranges::less, class Proj1 = identity,
        class Proj2 = identity>
    requires mergeable<I1, I2, O, Comp, Proj1, Proj2>
    static constexpr set_difference_result<I1, O> operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, O result, Comp comp = {}, Proj1 proj1 = {},
        Proj2 proj2 = {}
    ) {
        while (first1 != last1 && first2 != last2) {
            if (std::invoke(comp, std::invoke(proj1, *first1), std::invoke(proj2, *first2))) {
                *result = *first1;
                ++first1;
                ++result;
            } else if (
                std::invoke(comp, std::invoke(proj2, *first2), std::invoke(proj1, *first1))
            ) {
                ++first2;
            } else {
                // Equivalent element found in R2; exclude from output
                ++first1;
                ++first2;
            }
        }
        auto res = ranges::copy(std::move(first1), std::move(last1), std::move(result));
        return {std::move(res.in), std::move(res.out)};
    }

    template<
        input_range R1, input_range R2, weakly_incrementable O, class Comp = ranges::less,
        class Proj1 = identity, class Proj2 = identity>
    requires mergeable<iterator_t<R1>, iterator_t<R2>, O, Comp, Proj1, Proj2>
    static constexpr set_difference_result<borrowed_iterator_t<R1>, O>
    operator()(R1&& r1, R2&& r2, O result, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2),
            std::move(result), std::move(comp), std::move(proj1), std::move(proj2)
        );
    }
} set_difference;

// =============================================================================
// ranges::set_symmetric_difference
// =============================================================================

inline constexpr struct __set_symmetric_difference_fn {
    template<
        input_iterator I1, sentinel_for<I1> S1, input_iterator I2, sentinel_for<I2> S2,
        weakly_incrementable O, class Comp = ranges::less, class Proj1 = identity,
        class Proj2 = identity>
    requires mergeable<I1, I2, O, Comp, Proj1, Proj2>
    static constexpr set_symmetric_difference_result<I1, I2, O> operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, O result, Comp comp = {}, Proj1 proj1 = {},
        Proj2 proj2 = {}
    ) {
        while (first1 != last1 && first2 != last2) {
            if (std::invoke(comp, std::invoke(proj1, *first1), std::invoke(proj2, *first2))) {
                *result = *first1;
                ++first1;
                ++result;
            } else if (
                std::invoke(comp, std::invoke(proj2, *first2), std::invoke(proj1, *first1))
            ) {
                *result = *first2;
                ++first2;
                ++result;
            } else {
                // Equivalent elements in both; exclude from output
                ++first1;
                ++first2;
            }
        }

        auto res1 = ranges::copy(std::move(first1), std::move(last1), std::move(result));
        auto res2 = ranges::copy(std::move(first2), std::move(last2), std::move(res1.out));

        return {std::move(res1.in), std::move(res2.in), std::move(res2.out)};
    }

    template<
        input_range R1, input_range R2, weakly_incrementable O, class Comp = ranges::less,
        class Proj1 = identity, class Proj2 = identity>
    requires mergeable<iterator_t<R1>, iterator_t<R2>, O, Comp, Proj1, Proj2>
    static constexpr set_symmetric_difference_result<
        borrowed_iterator_t<R1>, borrowed_iterator_t<R2>, O>
    operator()(R1&& r1, R2&& r2, O result, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2),
            std::move(result), std::move(comp), std::move(proj1), std::move(proj2)
        );
    }
} set_symmetric_difference;

}  // namespace std::ranges
