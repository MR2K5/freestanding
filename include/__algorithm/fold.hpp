#pragma once

#include <__ranges/core.hpp>
#include <__ranges/result_types.hpp>
#include <iterator>
#include <optional>
#include <type_traits>

namespace std::ranges {

namespace __detail {

template<class F> class __flipped {
    F f;

public:
    template<class T, class U> requires invocable<F&, U, T>
    invoke_result_t<F&, U, T> operator()(T&&, U&&);
};

template<class F, class T, class I, class U>
concept __indirectly_binary_left_foldable_impl =
    movable<T> && movable<U> && convertible_to<T, U> && invocable<F&, U, iter_reference_t<I>>
    && assignable_from<U&, invoke_result_t<F&, U, iter_reference_t<I>>>;

template<class F, class T, class I>
concept __indirectly_binary_left_foldable =
    copy_constructible<F> && indirectly_readable<I> && invocable<F&, T, iter_reference_t<I>>
    && convertible_to<
        invoke_result_t<F&, T, iter_reference_t<I>>,
        decay_t<invoke_result_t<F&, T, iter_reference_t<I>>>>
    && __indirectly_binary_left_foldable_impl<
        F, T, I, decay_t<invoke_result_t<F&, T, iter_reference_t<I>>>>;

template<class F, class T, class I>
concept __indirectly_binary_right_foldable = __indirectly_binary_left_foldable<__flipped<F>, T, I>;

}  // namespace __detail

inline constexpr struct __fold_left_with_iter_fn {
    template<
        input_iterator I, sentinel_for<I> S, class F, class T = iter_value_t<I>,
        class U = decay_t<invoke_result_t<F&, T, iter_reference_t<I>>>>
    requires __detail::__indirectly_binary_left_foldable<F, T, I>
    static constexpr fold_left_with_iter_result<I, U> operator()(I first, S last, T init, F f) {
        if (first == last) return {std::move(first), U(std::move(init))};
        U accum = invoke(f, std::move(init), *first);
        for (++first; first != last; ++first) accum = invoke(f, std::move(accum), *first);
        return {std::move(first), std::move(accum)};
    }

    template<
        input_range R, class F, class T = range_value_t<R>,
        class U = decay_t<invoke_result_t<F&, T, range_reference_t<R>>>>
    requires __detail::__indirectly_binary_left_foldable<F, T, iterator_t<R>>
    static constexpr fold_left_with_iter_result<borrowed_iterator_t<R>, U>
    operator()(R&& r, T init, F f) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(init), std::move(f));
    }
} fold_left_with_iter;

inline constexpr struct __fold_left_fn {
    template<
        input_iterator I, sentinel_for<I> S, class T = iter_value_t<I>,
        __detail::__indirectly_binary_left_foldable<T, I> F>
    static constexpr auto operator()(I first, S last, T init, F f) {
        return fold_left_with_iter(std::move(first), last, std::move(init), std::move(f)).value;
    }

    template<
        input_range R, class T = range_value_t<R>,
        __detail::__indirectly_binary_left_foldable<T, iterator_t<R>> F>
    static constexpr auto operator()(R&& r, T init, F f) {
        return fold_left_with_iter(ranges::begin(r), ranges::end(r), std::move(init), std::move(f));
    }
} fold_left;

inline constexpr struct __fold_left_first_with_iter_fn {
    template<
        input_iterator I, sentinel_for<I> S,
        __detail::__indirectly_binary_left_foldable<iter_value_t<I>, I> F,
        class U = decltype(ranges::fold_left(
            declval<I>(), declval<S&>, iter_value_t<I>(*declval<I&>()), declval<F&>()
        ))>
    requires constructible_from<iter_value_t<I>, iter_reference_t<I>>
    static constexpr fold_left_first_with_iter_result<I, U> operator()(I first, S last, F f) {
        if (first == last) return {std::move(first), optional<U>()};
        optional<U> init(in_place, *first);
        for (++first; first != last; ++first) *init = invoke(f, std::move(*init), *first);
        return {std::move(first), std::move(init)};
    }

    template<
        input_range R,
        __detail::__indirectly_binary_left_foldable<range_value_t<R>, iterator_t<R>> F>
    requires constructible_from<range_value_t<R>, range_reference_t<R>>
    static constexpr auto operator()(R&& r, F f) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(f));
    }
} fold_left_first_with_iter;

inline constexpr struct __fold_left_first_fn {
    template<
        input_iterator I, sentinel_for<I> S,
        __detail::__indirectly_binary_left_foldable<iter_value_t<I>, I> F>
    requires constructible_from<iter_value_t<I>, iter_reference_t<I>>
    static constexpr auto operator()(I first, S last, F f) {
        return fold_left_first_with_iter(std::move(first), last, std::move(f)).value;
    }

    template<
        input_range R,
        __detail::__indirectly_binary_left_foldable<range_value_t<R>, iterator_t<R>> F>
    requires constructible_from<range_value_t<R>, range_reference_t<R>>
    static constexpr auto operator()(R&& r, F f) {
        return fold_left_first_with_iter(r, std::move(f)).value;
    }
} fold_left_first;

inline constexpr struct __fold_right_fn {
    template<
        bidirectional_iterator I, sentinel_for<I> S, class T = iter_value_t<I>,
        __detail::__indirectly_binary_right_foldable<T, I> F>
    static constexpr auto operator()(I first, S last, T init, F f) {
        using U = decay_t<invoke_result_t<F&, iter_reference_t<I>, T>>;
        if (first == last) return U(std::move(init));
        I tail  = ranges::next(first, last);
        U accum = std::invoke(f, *--tail, std::move(init));
        while (first != tail) accum = std::invoke(f, *--tail, std::move(accum));
        return accum;
    }

    template<
        bidirectional_range R, class T = range_value_t<R>,
        __detail::__indirectly_binary_right_foldable<T, iterator_t<R>> F>
    static constexpr auto operator()(R&& r, T init, F f) {
        return operator()(ranges::begin(r), ranges::end(r), std::move(init), std::move(f));
    }
} fold_right;

inline constexpr struct __fold_right_last_fn {
    template<
        bidirectional_iterator I, sentinel_for<I> S,
        __detail::__indirectly_binary_right_foldable<iter_value_t<I>, I> F>
    requires constructible_from<iter_value_t<I>, iter_reference_t<I>>
    constexpr auto operator()(I first, S last, F f) {
        using U = decltype(ranges::fold_right(first, last, iter_value_t<I>(*first), f));
        if (first == last) return optional<U>();

        I tail = prev(next(first, std::move(last)));
        return optional<U>(
            in_place, fold_right(std::move(first), tail, iter_value_t<I>(*tail), std::move(f))
        );
    }

    template<
        bidirectional_range R,
        __detail::__indirectly_binary_right_foldable<range_value_t<R>, iterator_t<R>> F>
    requires constructible_from<range_value_t<R>, range_reference_t<R>>
    constexpr auto operator()(R&& r, F f) {
        return operator()(begin(r), end(r), std::move(f));
    }
} fold_right_last;

}  // namespace std::ranges
