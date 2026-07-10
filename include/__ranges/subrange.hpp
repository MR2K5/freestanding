#pragma once
// code: language=c++
// IWYU pragma: private: include <ranges>

#include <__iterator/concepts.hpp>
#include <__ranges/core.hpp>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace std::ranges {

template<
    input_or_output_iterator I, sentinel_for<I> S = I,
    subrange_kind K = sized_sentinel_for<S, I> ? subrange_kind::sized : subrange_kind::unsized>
requires(K == subrange_kind::sized || !sized_sentinel_for<S, I>) class subrange;
}

namespace std {
template<class I, class S, ranges::subrange_kind K>
inline constexpr bool __is_subrange_v<ranges::subrange<I, S, K>> = true;
template<class I, class S, ranges::subrange_kind K>
inline constexpr bool __enable_tuple_like<ranges::subrange<I, S, K>> = true;
}  // namespace std

namespace std::ranges {

namespace __detail {
template<class From, class To>
concept __uses_nonqualification_pointer_conversion =
    is_pointer_v<From> && is_pointer_v<To>
    && !convertible_to<remove_pointer_t<From> (*)[], remove_pointer_t<To> (*)[]>;

template<class From, class To>
concept __convertible_to_non_slicing =
    convertible_to<From, To>
    && !__uses_nonqualification_pointer_conversion<decay_t<From>, decay_t<To>>;

template<class T, class U, class V>
concept __pair_like_convertible_from =
    !range<T> && !is_reference_v<T> && __pair_like<T> && constructible_from<T, U, V>
    && __convertible_to_non_slicing<U, tuple_element_t<0, T>>
    && convertible_to<V, tuple_element_t<1, T>>;

}  // namespace __detail

template<input_or_output_iterator I, sentinel_for<I> S, subrange_kind K>
requires(K == subrange_kind::sized || !sized_sentinel_for<S, I>)
class subrange: public view_interface<subrange<I, S, K>> {
    static constexpr bool store_size = K == subrange_kind::sized && !sized_sentinel_for<S, I>;

    I begin_ = {};
    S end_   = {};

    using __size_type = __detail::__make_unsigned_like<iter_difference_t<I>>;
    [[no_unique_address]] conditional_t<store_size, __size_type, __empty> size_;

public:
    subrange() requires default_initializable<I> = default;

    constexpr subrange(__detail::__convertible_to_non_slicing<I> auto i, S s) requires(!store_size)
        : begin_(std::move(i)), end_(s) {}

    constexpr subrange(__detail::__convertible_to_non_slicing<I> auto i, S s, __size_type n)
        requires(K == subrange_kind::sized)
        : begin_(std::move(i)), end_(s), size_(n) {}

    template<__different_from<subrange> R>
    requires borrowed_range<R> && __detail::__convertible_to_non_slicing<iterator_t<R>, I>
              && convertible_to<sentinel_t<R>, S>
              && (!store_size || sized_range<R>)constexpr subrange(R&& r)
        : begin_(std::move(ranges::begin(r))), end_(ranges::end(r)), size_([&r] {
              if constexpr (store_size) return static_cast<__size_type>(ranges::size(r));
              return 0;
          }()) {}

    template<borrowed_range R>
    requires __detail::__convertible_to_non_slicing<iterator_t<R>, I>
          && convertible_to<sentinel_t<R>, S>
             constexpr subrange(R&& r, __detail::__make_unsigned_like<iter_difference_t<I>> n)
                 requires(K == subrange_kind::sized)
        : subrange{ranges::begin(r), ranges::end(r), n} {}

    template<__different_from<subrange> Pair>
    requires __detail::__pair_like_convertible_from<Pair, I const&, S const&>
    constexpr operator Pair() const {
        return Pair(begin_, end_);
    }

    constexpr I begin() const requires copyable<I> { return begin_; }
    constexpr I begin() requires(!copyable<I>) { return std::move(begin_); }
    constexpr S end() const { return end_; }
    constexpr bool empty() const { return begin_ == end_; }
    constexpr __size_type size() const requires(K == subrange_kind::sized) {
        if constexpr (store_size) {
            return size_;
        } else {
            return __detail::__to_unsigned_like(end_ - begin_);
        }
    }

    constexpr subrange& advance(iter_difference_t<I> n) {
        if constexpr (bidirectional_iterator<I>) {
            if (n < 0) {
                ranges::advance(begin_, n);
                if constexpr (store_size) size_ += __detail::__to_unsigned_like(-n);
                return *this;
            }
        }
        assert(n >= 0);
        auto d = n - ranges::advance(begin_, n, end_);
        if constexpr (store_size) size_ -= __detail::__to_unsigned_like(d);
        return *this;
    }

    constexpr subrange prev(iter_difference_t<I> n = 1) const requires bidirectional_iterator<I> {
        auto tmp = *this;
        tmp.advance(-n);
        return tmp;
    }

    constexpr subrange next(iter_difference_t<I> n = 1) const& requires forward_iterator<I> {
        auto tmp = *this;
        tmp.advance(n);
        return tmp;
    }
    constexpr subrange next(iter_difference_t<I> n = 1) && {
        advance(n);
        return std::move(*this);
    }
};

template<input_or_output_iterator I, sentinel_for<I> S> subrange(I, S) -> subrange<I, S>;

template<input_or_output_iterator I, sentinel_for<I> S>
subrange(I, S, __detail::__make_unsigned_like<iter_difference_t<I>>)
    -> subrange<I, S, subrange_kind::sized>;

template<borrowed_range R>
subrange(R&&) -> subrange<
    iterator_t<R>, sentinel_t<R>,
    (sized_range<R> || sized_sentinel_for<sentinel_t<R>, iterator_t<R>>) ? subrange_kind::sized
                                                                         : subrange_kind::unsized>;

template<borrowed_range R>
subrange(R&&, __detail::__make_unsigned_like<range_difference_t<R>>)
    -> subrange<iterator_t<R>, sentinel_t<R>, subrange_kind::sized>;

template<size_t N, class I, class S, subrange_kind K> requires((N == 0 && copyable<I>) || N == 1)
constexpr auto get(subrange<I, S, K> const& s) {
    if constexpr (N == 0) {
        return s.begin();
    } else {
        return s.end();
    }
}
template<size_t N, class I, class S, subrange_kind K> requires(N < 2)
constexpr auto get(subrange<I, S, K>&& s) {
    if constexpr (N == 0) {
        return std::move(s).begin();
    } else {
        return s.end();
    }
}

template<class I, class S, subrange_kind K>
constexpr bool enable_borrowed_range<subrange<I, S, K>> = true;

template<range R>
using borrowed_subrange_t = conditional_t<borrowed_range<R>, subrange<iterator_t<R>>, dangling>;

}  // namespace std::ranges

namespace std {
using ranges::get;

template<class I, class S, ranges::subrange_kind K>
struct tuple_size<ranges::subrange<I, S, K>>: integral_constant<size_t, 2> {};

template<size_t Id, class I, class S, ranges::subrange_kind K> requires(Id <= 2)
struct tuple_element<Id, ranges::subrange<I, S, K>> {
    using type = conditional_t<Id == 0, I, S>;
};
template<size_t Id, class I, class S, ranges::subrange_kind K> requires(Id <= 2)
struct tuple_element<Id, ranges::subrange<I, S, K> const> {
    // subrange returns iterators by value, ignore const
    using type = conditional_t<Id == 0, I, S>;
};

}  // namespace std
