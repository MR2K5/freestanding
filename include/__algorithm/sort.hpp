#pragma once

#include <__algorithm/heap.hpp>
#include <__algorithm/mutating.hpp>
#include <__memory/algorithms.hpp>
#include <__memory/unique_ptr.hpp>
#include <__ranges/core.hpp>
#include <__ranges/result_types.hpp>
#include <__ranges/subrange.hpp>
#include <cstddef>
#include <new>

namespace std::ranges {

inline constexpr struct __is_sorted_until_fn {
    template<
        forward_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_strict_weak_order<projected<I, Proj>> Comp = less>
    static constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) {
        if (first == last) return first;

        for (auto next = first; ++next != last; first = next)
            if (std::invoke(comp, std::invoke(proj, *next), std::invoke(proj, *first))) return next;

        return first;
    }

    template<
        forward_range R, class Proj = identity,
        indirect_strict_weak_order<projected<iterator_t<R>, Proj>> Comp = less>
    static constexpr borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        return operator()(begin(r), end(r), std::ref(comp), std::ref(proj));
    }
} is_sorted_until;

inline constexpr struct __is_sorted_fn {
    template<
        forward_iterator I, sentinel_for<I> S, class Proj = identity,
        indirect_strict_weak_order<projected<I, Proj>> Comp = less>
    constexpr bool operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
        return is_sorted_until(first, last, comp, proj) == last;
    }

    template<
        forward_range R, class Proj = identity,
        indirect_strict_weak_order<projected<iterator_t<R>, Proj>> Comp = less>
    constexpr bool operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
        return (*this)(begin(r), next(begin(r), end(r)), std::ref(comp), std::ref(proj));
    }
} is_sorted;

inline constexpr struct __lower_bound_fn {
    template<
        forward_iterator I, sentinel_for<I> S, class Proj = identity,
        class T                                                       = projected_value_t<I, Proj>,
        indirect_strict_weak_order<T const*, projected<I, Proj>> Comp = ranges::less>
    static constexpr I operator()(I first, S last, T const& value, Comp comp = {}, Proj proj = {}) {
        I it;
        std::iter_difference_t<I> count, step;
        count = ranges::distance(first, last);

        while (count > 0) {
            it   = first;
            step = count / 2;
            ranges::advance(it, step, last);
            if (comp(std::invoke(proj, *it), value)) {
                first  = ++it;
                count -= step + 1;
            } else
                count = step;
        }
        return first;
    }

    template<
        ranges::forward_range R, class Proj = identity,
        class T = projected_value_t<ranges::iterator_t<R>, Proj>,
        indirect_strict_weak_order<T const*, projected<ranges::iterator_t<R>, Proj>> Comp =
            ranges::less>
    static constexpr ranges::borrowed_iterator_t<R>
    operator()(R&& r, T const& value, Comp comp = {}, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), value, std::ref(comp), std::ref(proj));
    }
} lower_bound;

inline constexpr struct __upper_bound_fn {
    template<
        forward_iterator I, sentinel_for<I> S, class Proj = identity,
        class T                                                       = projected_value_t<I, Proj>,
        indirect_strict_weak_order<T const*, projected<I, Proj>> Comp = ranges::less>
    static constexpr I operator()(I first, S last, T const& value, Comp comp = {}, Proj proj = {}) {
        I it;
        std::iter_difference_t<I> count, step;
        count = ranges::distance(first, last);

        while (count > 0) {
            it   = first;
            step = count / 2;
            ranges::advance(it, step, last);
            if (!comp(value, std::invoke(proj, *it))) {
                first  = ++it;
                count -= step + 1;
            } else
                count = step;
        }
        return first;
    }

    template<
        ranges::forward_range R, class Proj = identity,
        class T = projected_value_t<ranges::iterator_t<R>, Proj>,
        indirect_strict_weak_order<T const*, projected<ranges::iterator_t<R>, Proj>> Comp =
            ranges::less>
    static constexpr ranges::borrowed_iterator_t<R>
    operator()(R&& r, T const& value, Comp comp = {}, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), value, std::ref(comp), std::ref(proj));
    }
} upper_bound;

inline constexpr struct __equal_range_fn {
    template<
        forward_iterator I, sentinel_for<I> S, class Proj = identity,
        class T                                                       = projected_value_t<I, Proj>,
        indirect_strict_weak_order<T const*, projected<I, Proj>> Comp = ranges::less>
    static constexpr ranges::subrange<I>
    operator()(I first, S last, T const& value, Comp comp = {}, Proj proj = {}) {
        return ranges::subrange(
            ranges::lower_bound(first, last, value, std::ref(comp), std::ref(proj)),
            ranges::upper_bound(first, last, value, std::ref(comp), std::ref(proj))
        );
    }

    template<
        ranges::forward_range R, class Proj = identity,
        class T = projected_value_t<ranges::iterator_t<R>, Proj>,
        indirect_strict_weak_order<T const*, projected<ranges::iterator_t<R>, Proj>> Comp =
            ranges::less>
    static constexpr ranges::borrowed_subrange_t<R>
    operator()(R&& r, T const& value, Comp comp = {}, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), value, std::ref(comp), std::ref(proj));
    }
} equal_range;

inline constexpr struct __binary_search_fn {
    template<
        forward_iterator I, sentinel_for<I> S, class Proj = identity,
        class T                                                       = projected_value_t<I, Proj>,
        indirect_strict_weak_order<T const*, projected<I, Proj>> Comp = ranges::less>
    static constexpr bool
    operator()(I first, S last, T const& value, Comp comp = {}, Proj proj = {}) {
        auto x = ranges::lower_bound(first, last, value, comp, proj);
        return (!(x == last) && !(std::invoke(comp, value, std::invoke(proj, *x))));
    }

    template<
        ranges::forward_range R, class Proj = identity,
        class T = projected_value_t<ranges::iterator_t<R>, Proj>,
        indirect_strict_weak_order<T const*, projected<ranges::iterator_t<R>, Proj>> Comp =
            ranges::less>
    static constexpr bool operator()(R&& r, T const& value, Comp comp = {}, Proj proj = {}) {
        return operator()(
            ranges::begin(r), ranges::end(r), value, std::move(comp), std::move(proj)
        );
    }
} binary_search;

namespace __detail {

// Binary Insertion Sort using upper_bound + rotate
template<class I, class Comp, class Proj>
constexpr void __insertion_sort(I first, I last, Comp&& comp, Proj&& proj) {
    for (I cur = first + 1; cur != last; ++cur) {
        auto dest = ranges::upper_bound(first, cur, std::invoke(proj, *cur), comp, proj);
        ranges::rotate(dest, cur, cur + 1);
    }
}

// Median-of-3 pivot selector
template<class I, class Cmp> constexpr I __median_of_3(I a, I b, I c, Cmp&& cmp) {
    if (cmp(*a, *b)) {
        if (cmp(*b, *c)) return b;
        return cmp(*a, *c) ? c : a;
    }
    if (cmp(*a, *c)) return a;
    return cmp(*b, *c) ? c : b;
}

// Two-way Hoare Partition
template<class I, class Cmp> constexpr I __partition(I first, I last, Cmp&& cmp) {
    I mid = first + (last - first) / 2;
    ranges::iter_swap(first, __median_of_3(first, mid, last - 1, cmp));

    I i = first + 1;
    I j = last - 1;
    while (true) {
        while (i <= j && cmp(*i, *first)) ++i;
        while (i <= j && cmp(*first, *j)) --j;
        if (i >= j) break;
        ranges::iter_swap(i++, j--);
    }
    ranges::iter_swap(first, j);
    return j;
}

// Core Introsort Loop
template<class I, class Comp, class Proj, class CmpWrapper>
constexpr void
__introsort(I first, I last, size_t max_depth, Comp&& comp, Proj&& proj, CmpWrapper&& cmp) {
    while (last - first > 16) {
        if (max_depth == 0) {
            // Simplified: leverage standard heap algorithms
            ranges::make_heap(first, last, comp, proj);
            ranges::sort_heap(first, last, comp, proj);
            return;
        }
        --max_depth;
        I pivot = __partition(first, last, cmp);

        if (pivot - first < last - (pivot + 1)) {
            __introsort(first, pivot, max_depth, comp, proj, cmp);
            first = pivot + 1;
        } else {
            __introsort(pivot + 1, last, max_depth, comp, proj, cmp);
            last = pivot;
        }
    }
}

}  // namespace __detail

inline constexpr struct __sort_fn {
    template<
        random_access_iterator I, sentinel_for<I> S, class Comp = ranges::less,
        class Proj = identity>
    requires sortable<I, Comp, Proj>
    static constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) {
        auto end_it = ranges::next(first, last);
        auto len    = end_it - first;
        if (len <= 1) return end_it;

        auto cmp = [&](auto&& a, auto&& b) -> bool {
            return std::invoke(
                comp, std::invoke(proj, std::forward<decltype(a)>(a)),
                std::invoke(proj, std::forward<decltype(b)>(b))
            );
        };

        size_t depth_limit = 2 * (std::bit_width(static_cast<size_t>(len)) - 1);

        __detail::__introsort(
            first, end_it, depth_limit, std::ref(comp), std::ref(proj), std::ref(cmp)
        );
        __detail::__insertion_sort(first, end_it, std::ref(comp), std::ref(proj));

        return end_it;
    }

    template<random_access_range R, class Comp = ranges::less, class Proj = identity>
    requires sortable<iterator_t<R>, Comp, Proj>
    static constexpr borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(comp), std::move(proj));
    }
} sort;

inline constexpr struct partial_sort_fn {
    template<
        random_access_iterator I, sentinel_for<I> S, class Comp = ranges::less,
        class Proj = identity>
    requires sortable<I, Comp, Proj>
    static constexpr I operator()(I first, I middle, S last, Comp comp = {}, Proj proj = {}) {
        if (first == middle) return ranges::next(first, last);
        ranges::make_heap(first, middle, std::ref(comp), std::ref(proj));
        auto it{middle};
        for (; it != last; ++it) {
            if (std::invoke(comp, std::invoke(proj, *it), std::invoke(proj, *first))) {
                ranges::pop_heap(first, middle, std::ref(comp), std::ref(proj));
                ranges::iter_swap(middle - 1, it);
                ranges::push_heap(first, middle, std::ref(comp), std::ref(proj));
            }
        }
        ranges::sort_heap(first, middle, std::ref(comp), std::ref(proj));
        return it;
    }

    template<ranges::random_access_range R, class Comp = ranges::less, class Proj = std::identity>
    requires std::sortable<ranges::iterator_t<R>, Comp, Proj>
    static constexpr ranges::borrowed_iterator_t<R>
    operator()(R&& r, ranges::iterator_t<R> middle, Comp comp = {}, Proj proj = {}) {
        return operator()(
            ranges::begin(r), std::move(middle), ranges::end(r), std::move(comp), std::move(proj)
        );
    }
} partial_sort;

inline constexpr struct __partial_sort_copy_fn {
    template<
        input_iterator I1, sentinel_for<I1> S1, random_access_iterator I2, sentinel_for<I2> S2,
        class Comp = ranges::less, class Proj1 = identity, class Proj2 = identity>
    requires indirectly_copyable<I1, I2> && sortable<I2, Comp, Proj2>
          && indirect_strict_weak_order<Comp, projected<I1, Proj1>, projected<I2, Proj2>>
    static constexpr ranges::partial_sort_copy_result<I1, I2> operator()(
        I1 first, S1 last, I2 d_first, S2 d_last, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}
    ) {
        if (d_first == d_last)
            return {std::move(ranges::next(std::move(first), std::move(last))), std::move(d_first)};

        auto out_last{d_first};
        // copy first N elements
        for (; !(first == last or out_last == d_last); ++out_last, ++first) *out_last = *first;

        // convert N copied elements into a max-heap
        ranges::make_heap(d_first, out_last, std::ref(comp), std::ref(proj2));

        // process the rest of the input range (if any), preserving the heap property
        for (; first != last; ++first) {
            if (std::invoke(comp, std::invoke(proj1, *first), std::invoke(proj2, *d_first))) {
                // pop out the biggest item and push in a newly found smaller one
                ranges::pop_heap(d_first, out_last, std::ref(comp), std::ref(proj2));
                *(out_last - 1) = *first;
                ranges::push_heap(d_first, out_last, std::ref(comp), std::ref(proj2));
            }
        }

        // first N elements in the output range is still
        // a heap - convert it into a sorted range
        ranges::sort_heap(d_first, out_last, std::ref(comp), std::ref(proj2));

        return {std::move(first), std::move(out_last)};
    }

    template<ranges::input_range R> static constexpr auto get_end(R&& r) { return ranges::end(r); }

    template<ranges::forward_range R> static constexpr auto get_end(R&& r) {
        return ranges::next(ranges::begin(r), ranges::end(r));
    }

    template<
        ranges::input_range R1, ranges::random_access_range R2, class Comp = ranges::less,
        class Proj1 = identity, class Proj2 = identity>
    requires indirectly_copyable<ranges::iterator_t<R1>, ranges::iterator_t<R2>>
          && sortable<ranges::iterator_t<R2>, Comp, Proj2>
          && indirect_strict_weak_order<
                 Comp, projected<ranges::iterator_t<R1>, Proj1>,
                 projected<ranges::iterator_t<R2>, Proj2>>
    static constexpr ranges::partial_sort_copy_result<
        ranges::borrowed_iterator_t<R1>, ranges::borrowed_iterator_t<R2>>
    operator()(R1&& r, R2&& d_r, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r), get_end(r), ranges::begin(d_r), get_end(d_r), std::move(comp),
            std::move(proj1), std::move(proj2)
        );
    }
} partial_sort_copy;

inline constexpr struct __merge_fn {
    template<
        input_iterator I1, sentinel_for<I1> S1, input_iterator I2, sentinel_for<I2> S2,
        weakly_incrementable O, class Comp = ranges::less, class Proj1 = identity,
        class Proj2 = identity>
    requires mergeable<I1, I2, O, Comp, Proj1, Proj2>
    static constexpr ranges::merge_result<I1, I2, O> operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, O result, Comp comp = {}, Proj1 proj1 = {},
        Proj2 proj2 = {}
    ) {
        for (; !(first1 == last1 || first2 == last2); ++result) {
            if (std::invoke(comp, std::invoke(proj2, *first2), std::invoke(proj1, *first1)))
                *result = *first2, ++first2;
            else
                *result = *first1, ++first1;
        }
        auto ret1{ranges::copy(std::move(first1), std::move(last1), std::move(result))};
        auto ret2{ranges::copy(std::move(first2), std::move(last2), std::move(ret1.out))};
        return {std::move(ret1.in), std::move(ret2.in), std::move(ret2.out)};
    }

    template<
        ranges::input_range R1, ranges::input_range R2, weakly_incrementable O,
        class Comp = ranges::less, class Proj1 = identity, class Proj2 = identity>
    requires mergeable<ranges::iterator_t<R1>, ranges::iterator_t<R2>, O, Comp, Proj1, Proj2>
    static constexpr ranges::merge_result<
        ranges::borrowed_iterator_t<R1>, ranges::borrowed_iterator_t<R2>, O>
    operator()(R1&& r1, R2&& r2, O result, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            ranges::begin(r1), ranges::end(r1), ranges::begin(r2), ranges::end(r2),
            std::move(result), std::move(comp), std::move(proj1), std::move(proj2)
        );
    }
} merge;

// TODO stable_sort

inline constexpr struct __inplace_merge_fn {
private:
    // Fallback: O(N log N) divide-and-conquer via rotations (0 bytes allocated)
    template<bidirectional_iterator I, class Comp, class Proj>
    static constexpr void inplace_merge_slow(
        I first, I middle, I last, iter_difference_t<I> n1, iter_difference_t<I> n2, Comp comp,
        Proj proj
    ) {
        if (n1 == 0 || n2 == 0) return;

        // Base case: 1 element in each half
        if (n1 + n2 == 2) {
            if (std::invoke(comp, std::invoke(proj, *middle), std::invoke(proj, *first))) {
                ranges::iter_swap(first, middle);
            }
            return;
        }

        I cut1                  = first;
        I cut2                  = middle;
        iter_difference_t<I> d1 = 0;
        iter_difference_t<I> d2 = 0;

        if (n1 >= n2) {
            // Bisect left partition
            d1 = n1 / 2;
            ranges::advance(cut1, d1);

            // Find first element in [middle, last) where !(proj(elem) < proj(*cut1))
            // Pass the PROJECTED value to ranges::lower_bound
            auto&& projected_val = std::invoke(proj, *cut1);
            cut2                 = ranges::lower_bound(middle, last, projected_val, comp, proj);
            d2                   = ranges::distance(middle, cut2);
        } else {
            // Bisect right partition
            d2 = n2 / 2;
            ranges::advance(cut2, d2);

            // Find first element in [first, middle) where proj(*cut2) < proj(elem)
            auto&& projected_val = std::invoke(proj, *cut2);
            cut1                 = ranges::upper_bound(first, middle, projected_val, comp, proj);
            d1                   = ranges::distance(first, cut1);
        }

        // Rotate Block A and Block B into relative order
        I new_middle = ranges::rotate(cut1, middle, cut2).begin();

        // Recurse on both halves
        inplace_merge_slow(first, cut1, new_middle, d1, d2, std::ref(comp), std::ref(proj));
        inplace_merge_slow(
            new_middle, cut2, last, n1 - d1, n2 - d2, std::ref(comp), std::ref(proj)
        );
    }

    template<typename T> struct ScratchBuffer {
        T* ptr                  = nullptr;
        std::size_t capacity    = 0;
        std::size_t constructed = 0;

        // Allocate uninitialized memory without constructing any objects
        constexpr explicit ScratchBuffer(std::size_t n) noexcept {
            // Use std::nothrow so failure falls back gracefully to O(N log N)
            if constexpr (alignof(T) > alignof(max_align_t))
                ptr = static_cast<T*>(
                    ::operator new(n * sizeof(T), std::align_val_t(alignof(T)), std::nothrow)
                );
            else
                ptr = static_cast<T*>(::operator new(n * sizeof(T), std::nothrow));
            if (ptr) { capacity = n; }
        }

        // Move-construct elements from [first, last) into the buffer
        template<std::bidirectional_iterator I> constexpr void move_in(I first, I last) {
            for (auto it = first; it != last; ++it) {
                ::new (static_cast<void*>(ptr + constructed)) T(ranges::iter_move(it));
                ++constructed;
            }
        }

        // Clean up: destroy constructed elements, then free raw memory
        constexpr ~ScratchBuffer() {
            if (ptr) {
                ranges::destroy_n(ptr, constructed);
                if constexpr (alignof(T) > alignof(max_align_t))
                    ::operator delete(ptr, align_val_t(alignof(T)));
                else
                    ::operator delete(ptr);
            }
        }

        ScratchBuffer(ScratchBuffer const&)            = delete;
        ScratchBuffer& operator=(ScratchBuffer const&) = delete;
    };

    template<std::bidirectional_iterator I, class Comp, class Proj>
    static constexpr bool try_inplace_merge_buffered(
        I first, I middle, I last, std::iter_difference_t<I> n1, std::iter_difference_t<I> n2,
        Comp comp, Proj proj
    ) {
        using ValueType = std::iter_value_t<I>;

        // =========================================================================
        // CASE 1: Left partition is smaller or equal (n1 <= n2)
        // Strategy: Buffer [first, middle), merge FORWARD from first to last
        // =========================================================================
        if (n1 <= n2) {
            ScratchBuffer<ValueType> buf(static_cast<std::size_t>(n1));
            if (!buf.ptr) {
                return false;  // Out of memory -> fallback to O(N log N)
            }

            buf.move_in(first, middle);

            ValueType* left_cur = buf.ptr;
            ValueType* left_end = buf.ptr + n1;
            I right_cur         = middle;
            I dest              = first;

            while (left_cur != left_end && right_cur != last) {
                // Stability: if elements are equivalent, take from left first
                if (std::invoke(
                        comp, std::invoke(proj, *right_cur), std::invoke(proj, *left_cur)
                    )) {
                    *dest = ranges::iter_move(right_cur);
                    ++right_cur;
                } else {
                    *dest = std::move(*left_cur);
                    ++left_cur;
                }
                ++dest;
            }

            // Drain any remaining elements from the scratch buffer
            while (left_cur != left_end) {
                *dest = std::move(*left_cur);
                ++left_cur;
                ++dest;
            }
            // Remaining right-side elements are already in place

            return true;
        }

        // =========================================================================
        // CASE 2: Right partition is smaller (n2 < n1)
        // Strategy: Buffer [middle, last), merge BACKWARD from last down to first
        // =========================================================================
        ScratchBuffer<ValueType> buf(static_cast<std::size_t>(n2));
        if (!buf.ptr) {
            return false;  // Out of memory -> fallback to O(N log N)
        }

        buf.move_in(middle, last);

        I left_cur             = middle;
        I left_begin           = first;
        ValueType* right_cur   = buf.ptr + n2;
        ValueType* right_begin = buf.ptr;
        I dest                 = last;

        while (left_cur != left_begin && right_cur != right_begin) {
            auto left_prev   = ranges::prev(left_cur);
            auto* right_prev = right_cur - 1;

            --dest;
            // Stability check in reverse:
            // Only select left if it is STRICTLY greater than right.
            // If left and right are equivalent, we MUST pick right so that
            // the right element ends up further toward the end (preserving stability).
            if (std::invoke(comp, std::invoke(proj, *right_prev), std::invoke(proj, *left_prev))) {
                *dest    = ranges::iter_move(left_prev);
                left_cur = left_prev;
            } else {
                *dest     = std::move(*right_prev);
                right_cur = right_prev;
            }
        }

        // Drain remaining elements from the scratch buffer
        while (right_cur != right_begin) {
            --dest;
            --right_cur;
            *dest = std::move(*right_cur);
        }
        // Remaining left-side elements are already in place in [first, dest)

        return true;
    }

public:
    // (1) Iterator + Sentinel overload
    template<
        bidirectional_iterator I, sentinel_for<I> S, class Comp = ranges::less,
        class Proj = identity>
    requires sortable<I, Comp, Proj>
    constexpr I operator()(I first, I middle, S last, Comp comp = {}, Proj proj = {}) const {
        // Materialize sentinel into a concrete bidirectional iterator
        I last_it = ranges::next(middle, last);

        auto n1 = ranges::distance(first, middle);
        auto n2 = ranges::distance(middle, last_it);

        if (n1 == 0 || n2 == 0) return last_it;

        // TODO We prob can remove this
        if !consteval {
            if (try_inplace_merge_buffered(
                    first, middle, last_it, n1, n2, std::ref(comp), std::ref(proj)
                ))
                return last_it;
        }

        // In standard library implementations:
        // Try fast O(N) path using temporary buffer allocation if runtime / non-constexpr.
        // Fall back to rotation-based O(N log N) if allocation fails or during constexpr
        // evaluation.
        inplace_merge_slow(first, middle, last_it, n1, n2, std::ref(comp), std::ref(proj));

        return last_it;
    }

    // (2) Range overload
    template<ranges::bidirectional_range R, class Comp = ranges::less, class Proj = identity>
    requires sortable<ranges::iterator_t<R>, Comp, Proj> constexpr ranges::borrowed_iterator_t<R>
    operator()(R&& r, ranges::iterator_t<R> middle, Comp comp = {}, Proj proj = {}) const {
        return (*this)(
            ranges::begin(r), std::move(middle), ranges::end(r), std::move(comp),
            std::move(proj)
        );
    }
} inplace_merge;

}  // namespace std::ranges
