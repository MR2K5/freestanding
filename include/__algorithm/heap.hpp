#pragma once

#include <__ranges/core.hpp>

namespace std::ranges {

inline constexpr struct __is_heap_until_fn {
    template<
        random_access_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_strict_weak_order<projected<I, Proj>> Comp = ranges::less>
    static constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) {
        auto end_it = ranges::next(first, last);
        auto len    = end_it - first;
        if (len < 2) return end_it;

        // Tracks (child - 1) / 2 without explicit division:
        // Left child is always odd, right child is always even.
        // Parent only advances when an even child (right child) is finished.
        iter_difference_t<I> parent = 0;
        for (iter_difference_t<I> child = 1; child < len; ++child) {
            if (std::invoke(
                    comp, std::invoke(proj, *(first + parent)), std::invoke(proj, *(first + child))
                )) {
                return first + child;
            }
            if ((child & 1) == 0) { ++parent; }
        }
        return end_it;
    }

    template<
        ranges::random_access_range R, class Proj = identity,
        indirect_strict_weak_order<projected<ranges::iterator_t<R>, Proj>> Comp = ranges::less>
    static constexpr ranges::borrowed_iterator_t<R>
    operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(comp), std::move(proj));
    }
} is_heap_until;

inline constexpr struct __is_heap_fn {
    template<
        random_access_iterator I, sentinel_for<I> S, class Comp = ranges::less,
        class Proj = identity>
    requires indirect_strict_weak_order<Comp, projected<I, Proj>>
    static constexpr bool operator()(I first, S last, Comp comp = {}, Proj proj = {}) {
        return ranges::is_heap_until(first, last, comp, proj) == ranges::next(first, last);
    }

    template<random_access_range R, class Comp = ranges::less, class Proj = identity>
    requires indirect_strict_weak_order<Comp, projected<iterator_t<R>, Proj>>
    static constexpr bool operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(comp), std::move(proj));
    }
} is_heap;

namespace __heap_detail {

// Sift-up: percolates the element at `first + hole` up the tree
template<class I, class Comp, class Proj>
constexpr void __sift_up(I first, iter_difference_t<I> hole, Comp& comp, Proj& proj) {
    if (hole <= 0) return;

    iter_value_t<I> val = ranges::iter_move(first + hole);
    while (hole > 0) {
        iter_difference_t<I> parent = (hole - 1) / 2;
        if (std::invoke(comp, std::invoke(proj, *(first + parent)), std::invoke(proj, val))) {
            *(first + hole) = ranges::iter_move(first + parent);
            hole            = parent;
        } else {
            break;
        }
    }
    *(first + hole) = std::move(val);
}

// Sift-down: restores heap property from `parent` downwards within bounds [0, len)
template<class I, class Comp, class Proj>
constexpr void __sift_down(
    I first, iter_difference_t<I> parent, iter_difference_t<I> len, Comp& comp, Proj& proj
) {
    iter_value_t<I> val = ranges::iter_move(first + parent);
    while (true) {
        iter_difference_t<I> child = 2 * parent + 1;
        if (child >= len) break;

        // Choose the larger of the two children
        if (child + 1 < len
            && std::invoke(
                comp, std::invoke(proj, *(first + child)), std::invoke(proj, *(first + (child + 1)))
            )) {
            ++child;
        }

        // If the larger child is strictly greater than the floating value, bubble it up
        if (std::invoke(comp, std::invoke(proj, val), std::invoke(proj, *(first + child)))) {
            *(first + parent) = ranges::iter_move(first + child);
            parent            = child;
        } else {
            break;
        }
    }
    *(first + parent) = std::move(val);
}

}  // namespace __heap_detail

// =============================================================================
// ranges::push_heap
// =============================================================================

inline constexpr struct __push_heap_fn {
    template<
        random_access_iterator I, sentinel_for<I> S, class Comp = ranges::less,
        class Proj = identity>
    requires permutable<I> && indirect_strict_weak_order<Comp, projected<I, Proj>>
    static constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) {
        auto end_it = ranges::next(first, last);
        auto len    = end_it - first;
        if (len > 1) { __heap_detail::__sift_up(first, len - 1, comp, proj); }
        return end_it;
    }

    template<random_access_range R, class Comp = ranges::less, class Proj = identity>
    requires permutable<iterator_t<R>>
          && indirect_strict_weak_order<Comp, projected<iterator_t<R>, Proj>>
    static constexpr borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(comp), std::move(proj));
    }
} push_heap;

// =============================================================================
// ranges::pop_heap
// =============================================================================

inline constexpr struct __pop_heap_fn {
    template<
        random_access_iterator I, sentinel_for<I> S, class Comp = ranges::less,
        class Proj = identity>
    requires permutable<I> && indirect_strict_weak_order<Comp, projected<I, Proj>>
    static constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) {
        auto end_it = ranges::next(first, last);
        auto len    = end_it - first;
        if (len > 1) {
            ranges::iter_swap(first, first + (len - 1));
            __heap_detail::__sift_down(first, 0, len - 1, comp, proj);
        }
        return end_it;
    }

    template<random_access_range R, class Comp = ranges::less, class Proj = identity>
    requires permutable<iterator_t<R>>
          && indirect_strict_weak_order<Comp, projected<iterator_t<R>, Proj>>
    static constexpr borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(comp), std::move(proj));
    }
} pop_heap;

// =============================================================================
// ranges::make_heap
// =============================================================================

inline constexpr struct __make_heap_fn {
    template<
        random_access_iterator I, sentinel_for<I> S, class Comp = ranges::less,
        class Proj = identity>
    requires permutable<I> && indirect_strict_weak_order<Comp, projected<I, Proj>>
    static constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) {
        auto end_it = ranges::next(first, last);
        auto len    = end_it - first;
        if (len > 1) {
            // Floyd's bottom-up construction: O(N) comparisons
            for (auto i = len / 2; i > 0; --i) {
                __heap_detail::__sift_down(first, i - 1, len, comp, proj);
            }
        }
        return end_it;
    }

    template<random_access_range R, class Comp = ranges::less, class Proj = identity>
    requires permutable<iterator_t<R>>
          && indirect_strict_weak_order<Comp, projected<iterator_t<R>, Proj>>
    static constexpr borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(comp), std::move(proj));
    }
} make_heap;

// =============================================================================
// ranges::sort_heap
// =============================================================================

inline constexpr struct __sort_heap_fn {
    template<
        random_access_iterator I, sentinel_for<I> S, class Comp = ranges::less,
        class Proj = identity>
    requires permutable<I> && indirect_strict_weak_order<Comp, projected<I, Proj>>
    static constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) {
        auto end_it = ranges::next(first, last);
        for (auto cur = end_it; cur - first > 1; --cur) {
            ranges::pop_heap(first, cur, comp, proj);
        }
        return end_it;
    }

    template<random_access_range R, class Comp = ranges::less, class Proj = identity>
    requires permutable<iterator_t<R>>
          && indirect_strict_weak_order<Comp, projected<iterator_t<R>, Proj>>
    static constexpr borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(comp), std::move(proj));
    }
} sort_heap;

}  // namespace std::ranges
