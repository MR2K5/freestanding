#pragma once

#include <__algorithm/nonmodifying.hpp>
#include <__iterator/concepts.hpp>
#include <__ranges/core.hpp>
#include <__ranges/result_types.hpp>
#include <__ranges/subrange.hpp>
#include <cstring>

namespace std::ranges {

template<class I, class O>
concept __can_memcpy = contiguous_iterator<I> && contiguous_iterator<O>
                    && same_as<remove_cv_t<iter_value_t<I>>, iter_value_t<O>>
                    && is_trivially_copyable_v<iter_value_t<O>>;

inline constexpr struct __copy_n_fn {
    template<input_iterator I, weakly_incrementable O> requires indirectly_copyable<I, O>
    static constexpr copy_n_result<I, O> operator()(I first, iter_difference_t<I> n, O result) {
        if constexpr (__can_memcpy<I, O>) {
            std::memmove(
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

    template<input_range R, weakly_incrementable O>
    requires std::indirectly_copyable<iterator_t<R>, O>
    static constexpr copy_result<borrowed_iterator_t<R>, O> operator()(R&& r, O result) {
        return operator()(begin(r), end(r), std::move(result));
    }
} copy;

inline constexpr struct __copy_if_fn {
    template<
        input_iterator I, sentinel_for<I> S, weakly_incrementable O, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    requires indirectly_copyable<I, O> static constexpr copy_if_result<I, O>
    operator()(I first, S last, O d_first, Pred pred, Proj proj = {}) {
        for (; first != last; ++first)
            if (std::invoke(pred, std::invoke(proj, *first))) {
                *d_first = *first;
                ++d_first;
            }
        return {std::move(first), std::move(d_first)};
    }

    template<
        input_range R, weakly_incrementable O, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    requires indirectly_copyable<iterator_t<R>, O>
    static constexpr copy_if_result<borrowed_iterator_t<R>, O>
    operator()(R&& r, O d_first, Pred pred, Proj proj = {}) {
        return operator()(begin(r), end(r), std::move(d_first), std::move(pred), std::move(proj));
    }

    template<
        forward_range R, weakly_incrementable O, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    requires indirectly_copyable<iterator_t<R>, O>
    static constexpr copy_if_result<borrowed_iterator_t<R>, O>
    operator()(R&& r, O d_first, Pred pred, Proj proj = {}) {
        return operator()(
            ranges::begin(r), ranges::next(ranges::begin(r), ranges::end(r)), std::move(d_first),
            std::move(pred), std::move(proj)
        );
    }
} copy_if;

inline constexpr struct __copy_backward_fn {
    template<bidirectional_iterator I1, sentinel_for<I1> S1, bidirectional_iterator I2>
    requires indirectly_copyable<I1, I2>
    static constexpr ranges::copy_backward_result<I1, I2> operator()(I1 first, S1 last, I2 result) {
        if (__can_memcpy<I1, I2> && sized_sentinel_for<S1, I1>) {
            auto n = last - first;
            std::memmove(
                std::to_address(result) - n, std::to_address(first), n * sizeof(iter_value_t<I1>)
            );
            return {first + n, result - n};
        }
        I1 last1{ranges::next(first, std::move(last))};
        for (I1 i{last1}; i != first;) *--result = *--i;
        return {std::move(last1), std::move(result)};
    }

    template<bidirectional_range R, bidirectional_iterator I>
    requires indirectly_copyable<iterator_t<R>, I>
    static constexpr copy_backward_result<borrowed_iterator_t<R>, I> operator()(R&& r, I result) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(result));
    }
} copy_backward;

inline constexpr struct __move_fn {
    template<input_iterator I, sentinel_for<I> S, weakly_incrementable O>
    requires indirectly_movable<I, O>
    static constexpr ranges::move_result<I, O> operator()(I first, S last, O result) {
        if (__can_memcpy<I, O> && sized_sentinel_for<S, I>) {
            return copy_n(first, last - first, result);
        }
        for (; first != last; ++first, ++result) *result = ranges::iter_move(first);
        return {std::move(first), std::move(result)};
    }
    template<input_range R, weakly_incrementable O> requires indirectly_movable<iterator_t<R>, O>
    static constexpr move_result<borrowed_iterator_t<R>, O> operator()(R&& r, O result) {
        return operator()(begin(r), end(r), std::move(result));
    }
} move;

inline constexpr struct __move_backward_fn {
    template<bidirectional_iterator I1, sentinel_for<I1> S1, bidirectional_iterator I2>
    requires indirectly_movable<I1, I2>
    static constexpr move_backward_result<I1, I2> operator()(I1 first, S1 last, I2 result) {
        if constexpr (__can_memcpy<I1, I2> && sized_sentinel_for<S1, I1>) {
            return copy_backward(first, last, result);
        }
        auto i{last};
        for (; i != first; *--result = ranges::iter_move(--i)) {}
        return {std::move(last), std::move(result)};
    }

    template<bidirectional_range R, bidirectional_iterator I>
    requires indirectly_movable<iterator_t<R>, I>
    static constexpr move_backward_result<borrowed_iterator_t<R>, I> operator()(R&& r, I result) {
        return operator()(begin(r), end(r), std::move(result));
    }
} move_backward;

inline constexpr struct __swap_ranges_fn {
    template<input_iterator I1, sentinel_for<I1> S1, input_iterator I2, sentinel_for<I2> S2>
    requires indirectly_swappable<I1, I2> static constexpr swap_ranges_result<I1, I2>
    operator()(I1 first1, S1 last1, I2 first2, S2 last2) {
        for (; !(first1 == last1 or first2 == last2); ++first1, ++first2) iter_swap(first1, first2);
        return {std::move(first1), std::move(first2)};
    }

    template<input_range R> static constexpr auto get_end(R&& r) { return end(r); }

    template<forward_range R> static constexpr auto get_end(R&& r) {
        return next(begin(r), end(r));
    }

    template<input_range R1, input_range R2>
    requires indirectly_swappable<iterator_t<R1>, iterator_t<R2>>
    static constexpr swap_ranges_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>>
    operator()(R1&& r1, R2&& r2) {
        return operator()(begin(r1), get_end(r1), begin(r2), get_end(r2));
    }
} swap_ranges;

inline constexpr struct __transform_fn {
    // First version
    template<
        input_iterator I, sentinel_for<I> S, weakly_incrementable O, copy_constructible F,
        class Proj = identity>
    requires indirectly_writable<O, indirect_result_t<F&, projected<I, Proj>>>
    static constexpr unary_transform_result<I, O>
    operator()(I first1, S last1, O d_first, F unary_op, Proj proj1 = {}) {
        for (; first1 != last1; ++first1, (void)++d_first)
            *d_first = std::invoke(unary_op, std::invoke(proj1, *first1));

        return {std::move(first1), std::move(d_first)};
    }

    template<input_range R> static constexpr auto get_end(R&& r) { return end(r); }

    template<forward_range R> static constexpr auto get_end(R&& r) {
        return next(begin(r), end(r));
    }

    // Second version
    template<input_range R, weakly_incrementable O, copy_constructible F, class Proj = identity>
    requires indirectly_writable<O, indirect_result_t<F&, projected<iterator_t<R>, Proj>>>
    static constexpr unary_transform_result<borrowed_iterator_t<R>, O>
    operator()(R&& r1, O d_first, F unary_op, Proj proj1 = {}) {
        return operator()(
            begin(r1), get_end(r1), std::move(d_first), std::move(unary_op), std::move(proj1)
        );
    }

    // Third version
    template<
        input_iterator I1, sentinel_for<I1> S1, input_iterator I2, sentinel_for<I2> S2,
        weakly_incrementable O, copy_constructible F, class Proj1 = identity,
        class Proj2 = identity>
    requires indirectly_writable<
        O, indirect_result_t<F&, projected<I1, Proj1>, projected<I2, Proj2>>>
    static constexpr binary_transform_result<I1, I2, O> operator()(
        I1 first1, S1 last1, I2 first2, S2 last2, O d_first, F binary_op, Proj1 proj1 = {},
        Proj2 proj2 = {}
    ) {
        for (; first1 != last1 && first2 != last2; ++first1, (void)++first2, (void)++d_first)
            *d_first =
                std::invoke(binary_op, std::invoke(proj1, *first1), std::invoke(proj2, *first2));

        return {std::move(first1), std::move(first2), std::move(d_first)};
    }

    // Fourth version
    template<
        input_range R1, input_range R2, weakly_incrementable O, copy_constructible F,
        class Proj1 = identity, class Proj2 = identity>
    requires indirectly_writable<
        O,
        indirect_result_t<F&, projected<iterator_t<R1>, Proj1>, projected<iterator_t<R2>, Proj2>>>
    static constexpr binary_transform_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>, O>
    operator()(R1&& r1, R2&& r2, O d_first, F binary_op, Proj1 proj1 = {}, Proj2 proj2 = {}) {
        return operator()(
            begin(r1), get_end(r1), begin(r2), get_end(r2), std::move(d_first),
            std::move(binary_op), std::move(proj1), std::move(proj2)
        );
    }
} transform;

inline constexpr struct __replace_fn {
    template<
        input_iterator I, sentinel_for<I> S, class Proj = identity,
        class T1 = projected_value_t<I, Proj>, class T2 = T1>
    requires indirectly_writable<I, const T2&>
          && indirect_binary_predicate<equal_to, projected<I, Proj>, const T1*> static constexpr I
    operator()(I first, S last, const T1& old_value, const T2& new_value, Proj proj = {}) {
        for (; first != last; ++first)
            if (old_value == std::invoke(proj, *first)) *first = new_value;
        return first;
    }

    template<
        input_range R, class Proj = identity, class T1 = projected_value_t<iterator_t<R>, Proj>,
        class T2 = T1>
    requires indirectly_writable<iterator_t<R>, const T2&>
          && indirect_binary_predicate<equal_to, projected<iterator_t<R>, Proj>, const T1*>
    static constexpr borrowed_iterator_t<R>
    operator()(R&& r, const T1& old_value, const T2& new_value, Proj proj = {}) {
        return operator()(begin(r), next(r), old_value, new_value, std::move(proj));
    }

    template<
        forward_range R, class Proj = identity, class T1 = projected_value_t<iterator_t<R>, Proj>,
        class T2 = T1>
    requires indirectly_writable<iterator_t<R>, const T2&>
          && indirect_binary_predicate<equal_to, projected<iterator_t<R>, Proj>, const T1*>
    static constexpr borrowed_iterator_t<R>
    operator()(R&& r, const T1& old_value, const T2& new_value, Proj proj = {}) {
        return operator()(begin(r), next(begin(r), end(r)), old_value, new_value, std::move(proj));
    }
} replace;

inline constexpr struct __replace_if_fn {
    template<
        input_iterator I, sentinel_for<I> S, class Proj = identity,
        class T = projected_value_t<I, Proj>, indirect_unary_predicate<projected<I, Proj>> Pred>
    requires indirectly_writable<I, T const&>
    static constexpr I operator()(I first, S last, Pred pred, T const& new_value, Proj proj = {}) {
        for (; first != last; ++first)
            if (!!std::invoke(pred, std::invoke(proj, *first))) *first = new_value;
        return std::move(first);
    }

    template<
        input_range R, class Proj = identity, class T = projected_value_t<iterator_t<R>, Proj>,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    requires indirectly_writable<iterator_t<R>, T const&> static constexpr borrowed_iterator_t<R>
    operator()(R&& r, Pred pred, T const& new_value, Proj proj = {}) {
        return operator()(begin(r), end(r), std::move(pred), new_value, std::move(proj));
    }

    template<
        forward_range R, class Proj = identity, class T = projected_value_t<iterator_t<R>, Proj>,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    requires indirectly_writable<iterator_t<R>, T const&> static constexpr borrowed_iterator_t<R>
    operator()(R&& r, Pred pred, T const& new_value, Proj proj = {}) {
        return operator()(
            begin(r), next(begin(r), end(r)), std::move(pred), new_value, std::move(proj)
        );
    }
} replace_if;

inline constexpr struct __replace_copy_fn {
    template<
        input_iterator I, sentinel_for<I> S, class O, class Proj = identity,
        class T1 = projected_value_t<I, Proj>, class T2 = iter_value_t<O>>
    requires indirectly_copyable<I, O>
          && indirect_binary_predicate<equal_to, projected<I, Proj>, const T1*>
          && output_iterator<O, const T2&>
    static constexpr replace_copy_result<I, O> operator()(
        I first, S last, O result, const T1& old_value, const T2& new_value, Proj proj = {}
    ) {
        for (; first != last; ++first, ++result)
            *result = (std::invoke(proj, *first) == old_value) ? new_value : *first;
        return {std::move(first), std::move(result)};
    }

    template<
        input_range R, class O, class Proj = identity,
        class T1 = projected_value_t<iterator_t<R>, Proj>, class T2 = iter_value_t<O>>
    requires indirectly_copyable<iterator_t<R>, O>
          && indirect_binary_predicate<equal_to, projected<iterator_t<R>, Proj>, const T1*>
    static constexpr replace_copy_result<borrowed_iterator_t<R>, O>
    operator()(R&& r, O result, const T1& old_value, const T2& new_value, Proj proj = {}) {
        return operator()(
            begin(r), end(r), std::move(result), old_value, new_value, std::move(proj)
        );
    }

    template<
        forward_range R, class O, class Proj = identity,
        class T1 = projected_value_t<iterator_t<R>, Proj>, class T2 = iter_value_t<O>>
    requires indirectly_copyable<iterator_t<R>, O>
          && indirect_binary_predicate<equal_to, projected<iterator_t<R>, Proj>, const T1*>
    static constexpr replace_copy_result<borrowed_iterator_t<R>, O>
    operator()(R&& r, O result, const T1& old_value, const T2& new_value, Proj proj = {}) {
        return operator()(
            begin(r), next(begin(r), end(r)), std::move(result), old_value, new_value,
            std::move(proj)
        );
    }
} replace_copy;

inline constexpr struct __replace_copy_if_fn {
    template<
        input_iterator I, sentinel_for<I> S, class O, class T = iter_value_t<O>,
        class Proj = identity, indirect_unary_predicate<projected<I, Proj>> Pred>
    requires indirectly_copyable<I, O> && output_iterator<O, T const&>
    static constexpr replace_copy_if_result<I, O>
    operator()(I first, S last, O result, Pred pred, T const& new_value, Proj proj = {}) {
        for (; first != last; ++first, ++result)
            *result = std::invoke(pred, std::invoke(proj, *first)) ? new_value : *first;
        return {std::move(first), std::move(result)};
    }

    template<
        input_range R, class O, class T = iter_value_t<O>, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    requires indirectly_copyable<iterator_t<R>, O> && output_iterator<O, T const&>
    static constexpr replace_copy_if_result<borrowed_iterator_t<R>, O>
    operator()(R&& r, O result, Pred pred, T const& new_value, Proj proj = {}) {
        return operator()(
            begin(r), end(r), std::move(result), std::move(pred), new_value, std::move(proj)
        );
    }

    template<
        forward_range R, class O, class T = iter_value_t<O>, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    requires indirectly_copyable<iterator_t<R>, O> && output_iterator<O, T const&>
    static constexpr replace_copy_if_result<borrowed_iterator_t<R>, O>
    operator()(R&& r, O result, Pred pred, T const& new_value, Proj proj = {}) {
        return operator()(
            begin(r), next(begin(r), end(r)), std::move(result), std::move(pred), new_value,
            std::move(proj)
        );
    }
} replace_copy_if;

inline constexpr struct __fill_fn {
    template<class O, sentinel_for<O> S, class T = iter_value_t<O>>
    requires output_iterator<O, T const&>
    static constexpr O operator()(O first, S last, T const& value) {
        if constexpr (
            contiguous_iterator<O> && sized_sentinel_for<O, S> && is_arithmetic_v<iter_value_t<O>>
        ) {
            if (value == 0) {
                std::memset(std::to_address(first, 0, sizeof(iter_value_t<O>) * last - first));
                return first + (last - first);
            }
        }

        while (first != last) *first++ = value;

        return first;
    }

    template<class R, class T = range_value_t<R>> requires output_range<R, T const&>
    static constexpr borrowed_iterator_t<R> operator()(R&& r, T const& value) {
        return operator()(begin(r), end(r), value);
    }
} fill;

inline constexpr struct __fill_n_fn {
    template<class O, class T = iter_value_t<O>> requires output_iterator<O, T const&>
    static constexpr O operator()(O first, iter_difference_t<O> n, T const& value) {
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
        for (iter_difference_t<O> i{}; i != n; ++first, ++i) *first = value;
        return first;
    }
} fill_n;

inline constexpr struct __generate_fn {
    template<input_or_output_iterator O, sentinel_for<O> S, copy_constructible F>
    requires invocable<F&> && indirectly_writable<O, invoke_result_t<F&>>
    static constexpr O operator()(O first, S last, F gen) {
        for (; first != last; *first = std::invoke(gen), ++first) {}
        return first;
    }

    template<class R, copy_constructible F>
    requires invocable<F&> && output_range<R, invoke_result_t<F&>>
    static constexpr borrowed_iterator_t<R> operator()(R&& r, F gen) {
        return operator()(begin(r), end(r), std::move(gen));
    }

    template<forward_range R, copy_constructible F>
    requires invocable<F&> && output_range<R, invoke_result_t<F&>>
    static constexpr borrowed_iterator_t<R> operator()(R&& r, F gen) {
        return operator()(begin(r), next(begin(r), end(r)), std::move(gen));
    }
} generate;

inline constexpr struct __generate_n_fn {
    template<input_or_output_iterator O, copy_constructible F>
    requires invocable<F&> && indirectly_writable<O, invoke_result_t<F&>>
    static constexpr O operator()(O first, iter_difference_t<O> count, F gen) {
        for (; count-- > 0; *first = std::invoke(gen), ++first) {}
        return first;
    }
} generate_n;

inline constexpr struct __remove_fn {
    template<
        permutable I, sentinel_for<I> S, class Proj = identity,
        class T = projected_value_t<I, Proj>>
    requires indirect_binary_predicate<equal_to, projected<I, Proj>, T const*>
    static constexpr subrange<I> operator()(I first, S last, T const& value, Proj proj = {}) {
        first = ranges::find(std::move(first), last, value, proj);
        if (first != last) {
            for (I i{std::next(first)}; i != last; ++i)
                if (value != std::invoke(proj, *i)) {
                    *first = iter_move(i);
                    ++first;
                }
        }
        return {first, last};
    }

    template<
        forward_range R, class Proj = identity, class T = projected_value_t<iterator_t<R>, Proj>>
    requires permutable<iterator_t<R>>
          && indirect_binary_predicate<equal_to, projected<iterator_t<R>, Proj>, T const*>
    static constexpr borrowed_subrange_t<R> operator()(R&& r, T const& value, Proj proj = {}) {
        return operator()(ranges::begin(r), ranges::end(r), value, std::move(proj));
    }
} remove;

inline constexpr struct __remove_if_fn {
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

inline constexpr struct __remove_copy_fn {
    template<
        input_iterator I, sentinel_for<I> S, weakly_incrementable O, class Proj = identity,
        class T = projected_value_t<I, Proj>>
    requires indirectly_copyable<I, O>
          && indirect_binary_predicate<equal_to, projected<I, Proj>, T const*>
    static constexpr remove_copy_result<I, O>
    operator()(I first, S last, O result, T const& value, Proj proj = {}) {
        for (; !(first == last); ++first)
            if (value != std::invoke(proj, *first)) {
                *result = *first;
                ++result;
            }
        return {std::move(first), std::move(result)};
    }

    template<
        input_range R, weakly_incrementable O, class Proj = identity,
        class T = projected_value_t<iterator_t<R>, Proj>>
    requires indirectly_copyable<iterator_t<R>, O>
          && indirect_binary_predicate<equal_to, projected<iterator_t<R>, Proj>, T const*>
    static constexpr remove_copy_result<borrowed_iterator_t<R>, O>
    operator()(R&& r, O result, T const& value, Proj proj = {}) {
        return operator()(begin(r), end(r), std::move(result), value, std::move(proj));
    }

    template<
        forward_range R, weakly_incrementable O, class Proj = identity,
        class T = projected_value_t<iterator_t<R>, Proj>>
    requires indirectly_copyable<iterator_t<R>, O>
          && indirect_binary_predicate<equal_to, projected<iterator_t<R>, Proj>, T const*>
    static constexpr remove_copy_result<borrowed_iterator_t<R>, O>
    operator()(R&& r, O result, T const& value, Proj proj = {}) {
        return operator()(
            begin(r), next(begin(r), end(r)), std::move(result), value, std::move(proj)
        );
    }
} remove_copy;

inline constexpr struct __remove_copy_if_fn {
    template<
        input_iterator I, sentinel_for<I> S, weakly_incrementable O, class Proj = identity,
        indirect_unary_predicate<projected<I, Proj>> Pred>
    requires indirectly_copyable<I, O> static constexpr remove_copy_if_result<I, O>
    operator()(I first, S last, O result, Pred pred, Proj proj = {}) {
        for (; first != last; ++first)
            if (false == std::invoke(pred, std::invoke(proj, *first))) {
                *result = *first;
                ++result;
            }
        return {std::move(first), std::move(result)};
    }

    template<
        input_range R, weakly_incrementable O, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    requires indirectly_copyable<iterator_t<R>, O>
    static constexpr remove_copy_if_result<borrowed_iterator_t<R>, O>
    operator()(R&& r, O result, Pred pred, Proj proj = {}) {
        return operator()(begin(r), end(r), std::move(result), std::move(pred), std::move(proj));
    }

    template<
        forward_range R, weakly_incrementable O, class Proj = identity,
        indirect_unary_predicate<projected<iterator_t<R>, Proj>> Pred>
    requires indirectly_copyable<iterator_t<R>, O>
    static constexpr remove_copy_if_result<borrowed_iterator_t<R>, O>
    operator()(R&& r, O result, Pred pred, Proj proj = {}) {
        return operator()(
            begin(r), next(begin(r), end(r)), std::move(result), std::move(pred), std::move(proj)
        );
    }
} remove_copy_if;

inline constexpr struct __unique_fn {
    template<
        permutable I, sentinel_for<I> S, class Proj = identity,
        indirect_equivalence_relation<projected<I, Proj>> C = equal_to>
    static constexpr subrange<I> operator()(I first, S last, C comp = {}, Proj proj = {}) {
        first = adjacent_find(first, last, comp, proj);
        if (first == last) return {first, first};
        auto i{first};
        ++first;
        while (++first != last)
            if (!std::invoke(comp, std::invoke(proj, *i), std::invoke(proj, *first)))
                *++i = iter_move(first);
        return {++i, first};
    }

    template<
        forward_range R, class Proj = identity,
        indirect_equivalence_relation<projected<iterator_t<R>, Proj>> C = equal_to>
    requires permutable<iterator_t<R>>
    static constexpr borrowed_subrange_t<R> operator()(R&& r, C comp = {}, Proj proj = {}) {
        return operator()(begin(r), next(begin(r), end(r)), std::move(comp), std::move(proj));
    }
} unique;

inline constexpr struct __unique_copy_fn {
    template<
        input_iterator I, sentinel_for<I> S, weakly_incrementable O, class Proj = identity,
        indirect_equivalence_relation<projected<I, Proj>> C = equal_to>
    requires indirectly_copyable<I, O>
          && (forward_iterator<I>
              || (input_iterator<O> && same_as<iter_value_t<I>, iter_value_t<O>>)
              || indirectly_copyable_storable<I, O>)static constexpr unique_copy_result<I, O>
    operator()(I first, S last, O result, C comp = {}, Proj proj = {}) {
        if (!(first == last)) {
            std::iter_value_t<I> value = *first;
            *result                    = value;
            ++result;
            while (!(++first == last)) {
                auto&& value2 = *first;
                if (!std::invoke(comp, std::invoke(proj, value2), std::invoke(proj, value))) {
                    value   = std::forward<decltype(value2)>(value2);
                    *result = value;
                    ++result;
                }
            }
        }

        return {std::move(first), std::move(result)};
    }

    template<
        input_range R, weakly_incrementable O, class Proj = identity,
        indirect_equivalence_relation<projected<iterator_t<R>, Proj>> C = equal_to>
    requires indirectly_copyable<iterator_t<R>, O>
          && (forward_iterator<iterator_t<R>>
              || (input_iterator<O> && same_as<range_value_t<R>, iter_value_t<O>>)
              || indirectly_copyable_storable<iterator_t<R>, O>)
    static constexpr unique_copy_result<borrowed_iterator_t<R>, O>
    operator()(R&& r, O result, C comp = {}, Proj proj = {}) {
        return operator()(begin(r), end(r), std::move(result), std::move(comp), std::move(proj));
    }

    template<
        forward_range R, weakly_incrementable O, class Proj = identity,
        indirect_equivalence_relation<projected<iterator_t<R>, Proj>> C = equal_to>
    requires indirectly_copyable<iterator_t<R>, O>
          && (forward_iterator<iterator_t<R>>
              || (input_iterator<O> && same_as<range_value_t<R>, iter_value_t<O>>)
              || indirectly_copyable_storable<iterator_t<R>, O>)
    static constexpr unique_copy_result<borrowed_iterator_t<R>, O>
    operator()(R&& r, O result, C comp = {}, Proj proj = {}) {
        return operator()(
            begin(r), next(begin(r), end(r)), std::move(result), std::move(comp), std::move(proj)
        );
    }
} unique_copy;

inline constexpr struct __reverse_copy_fn {
    template<bidirectional_iterator I, sentinel_for<I> S, weakly_incrementable O>
    requires indirectly_copyable<I, O>
    static constexpr reverse_copy_result<I, O> operator()(I first, S last, O result) {
        auto i    = next(first, last);
        auto tail = i;
        while (first != tail) {
            --tail;
            *result = *tail;
            ++result;
        }
        return {i, std::move(result)};
    }

    template<bidirectional_range R, weakly_incrementable O>
    requires indirectly_copyable<iterator_t<R>, O>
    static constexpr reverse_copy_result<borrowed_iterator_t<R>, O> operator()(R&& r, O result) {
        return operator()(begin(r), end(r), std::move(result));
    }
} reverse_copy;

inline constexpr struct __reverse_fn {
    template<bidirectional_iterator I, sentinel_for<I> S> requires permutable<I>
    static constexpr I operator()(I first, S last) {
        auto last2{next(first, last)};
        for (auto tail{last2}; !(first == tail || first == --tail); ++first) iter_swap(first, tail);
        return last2;
    }

    template<bidirectional_range R> requires permutable<iterator_t<R>>
    static constexpr borrowed_iterator_t<R> operator()(R&& r) {
        return operator()(begin(r), end(r));
    }
} reverse;

inline constexpr struct __rotate_fn {
    template<permutable I, sentinel_for<I> S>
    static constexpr subrange<I> operator()(I first, I middle, S last) {
        if (first == middle) {
            auto last_it = ranges::next(first, last);
            return {last_it, last_it};
        }
        if (middle == last) return {std::move(first), std::move(middle)};

        if constexpr (std::bidirectional_iterator<I>) {
            ranges::reverse(first, middle);
            auto last_it = ranges::next(first, last);
            ranges::reverse(middle, last_it);

            if constexpr (std::random_access_iterator<I>) {
                ranges::reverse(first, last_it);
                return {first + (last_it - middle), std::move(last_it)};
            } else {
                auto mid_last = last_it;
                do {
                    ranges::iter_swap(first, --mid_last);
                    ++first;
                } while (first != middle && mid_last != middle);
                ranges::reverse(first, mid_last);

                if (first == middle)
                    return {std::move(mid_last), std::move(last_it)};
                else
                    return {std::move(first), std::move(last_it)};
            }
        } else {  // I is merely a forward_iterator
            auto next_it = middle;
            do {  // rotate the first cycle
                ranges::iter_swap(first, next_it);
                ++first;
                ++next_it;
                if (first == middle) middle = next_it;
            } while (next_it != last);

            auto new_first = first;
            while (middle != last) {  // rotate subsequent cycles
                next_it = middle;
                do {
                    ranges::iter_swap(first, next_it);
                    ++first;
                    ++next_it;
                    if (first == middle) middle = next_it;
                } while (next_it != last);
            }

            return {std::move(new_first), std::move(middle)};
        }
    }

    template<forward_range R> requires std::permutable<iterator_t<R>>
    static constexpr borrowed_subrange_t<R> operator()(R&& r, iterator_t<R> middle) {
        return operator()(
            ranges::begin(r), std::move(middle), ranges::next(ranges::begin(r), ranges::end(r))
        );
    }
} rotate;

inline constexpr struct __rotate_copy_fn {
    template<forward_iterator I, sentinel_for<I> S, weakly_incrementable O>
    requires indirectly_copyable<I, O>
    static constexpr rotate_copy_result<I, O> operator()(I first, I middle, S last, O result) {
        auto c1{ranges::copy(middle, std::move(last), std::move(result))};
        auto c2{ranges::copy(std::move(first), std::move(middle), std::move(c1.out))};
        return {std::move(c1.in), std::move(c2.out)};
    }

    template<ranges::forward_range R, weakly_incrementable O>
    requires indirectly_copyable<ranges::iterator_t<R>, O>
    static constexpr ranges::rotate_copy_result<ranges::borrowed_iterator_t<R>, O>
    operator()(R&& r, ranges::iterator_t<R> middle, O result) {
        return operator()(
            ranges::begin(r), std::move(middle), ranges::next(ranges::begin(r), ranges::end(r)),
            std::move(result)
        );
    }
} rotate_copy;

inline constexpr struct __shift_left_fn {
    template<permutable I, sentinel_for<I> S>
    static constexpr subrange<I> operator()(I first, S last, iter_difference_t<I> n) {
        if (n <= 0) {
            auto end_it = ranges::next(first, last);
            return {first, end_it};
        }

        auto mid = ranges::next(first, n, last);
        if (mid == last) { return {first, first}; }

        auto result = ranges::move(mid, last, first);
        return {first, std::move(result.out)};
    }

    template<forward_range R> requires permutable<iterator_t<R>>
    static constexpr borrowed_subrange_t<R> operator()(R&& r, range_difference_t<R> n) {
        return operator()(ranges::begin(r), ranges::end(r), n);
    }
} shift_left;

inline constexpr struct __shift_right_fn {
    template<permutable I, sentinel_for<I> S>
    static constexpr subrange<I> operator()(I first, S last, iter_difference_t<I> n) {
        if (n <= 0) {
            auto end_it = ranges::next(first, last);
            return {first, end_it};
        }

        if constexpr (bidirectional_iterator<I>) {
            auto end_it = ranges::next(first, last);
            auto mid    = ranges::prev(end_it, n, first);
            if (mid == first) { return {end_it, end_it}; }
            auto result = ranges::move_backward(first, mid, end_it);
            return {std::move(result.out), std::move(end_it)};
        } else {
            // Forward iterator support: lead-and-trail pointer advancement with rotation
            auto lead = ranges::next(first, n, last);
            if (lead == last) {
                auto end_it = ranges::next(lead, last);
                return {end_it, end_it};
            }
            auto trail = first;
            while (lead != last) {
                ++trail;
                ++lead;
            }
            auto end_it = lead;
            auto ret    = ranges::rotate(first, trail, end_it);
            return {std::move(ret.begin()), std::move(ret.end())};
        }
    }

    template<forward_range R> requires permutable<iterator_t<R>>
    static constexpr borrowed_subrange_t<R> operator()(R&& r, range_difference_t<R> n) {
        return operator()(ranges::begin(r), ranges::end(r), n);
    }
} shift_right;

// TODO ranges::sample, ranges::shuffle

}  // namespace std::ranges
