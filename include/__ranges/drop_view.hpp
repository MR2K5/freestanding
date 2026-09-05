#pragma once

#include <__algorithm/minmax.hpp>
#include <__iterator/concepts.hpp>
#include <__ranges/core.hpp>
#include <__ranges/iota_view.hpp>
#include <__ranges/subrange.hpp>
#include <__ranges/repeat.hpp>
#include <optional>
#include <ranges>
#include <string_view>

namespace std::ranges {

namespace __detail {

template<class T> inline constexpr bool __subrange_stores_size = false;
template<class I, class S, subrange_kind K>
inline constexpr bool __subrange_stores_size<subrange<I, S, K>> =
    (K == subrange_kind::sized && !sized_sentinel_for<S, I>);

}  // namespace __detail

// =============================================================================
// Class template drop_view [range.drop.view]
// =============================================================================

template<view V> class drop_view: public view_interface<drop_view<V>> {
private:
    static constexpr bool __needs_cache =
        forward_range<V> && !(random_access_range<V> && sized_range<V>);

    using __cache_t =
        conditional_t<__needs_cache, __detail::non_propagating_cache<iterator_t<V>>, __empty>;

    [[no_unique_address]] V base_ = V();
    range_difference_t<V> count_  = 0;
    [[no_unique_address]] __cache_t cached_begin_{};

public:
    drop_view() requires default_initializable<V> = default;

    constexpr explicit drop_view(V base, range_difference_t<V> count)
        : base_(std::move(base)), count_(count) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin() requires(!(
        __detail::__simple_view<V> && random_access_range<V const> && sized_range<V const>
    )) {
        if constexpr (__needs_cache) {
            if (!cached_begin_.has_value()) {
                cached_begin_.emplace(
                    ranges::next(ranges::begin(base_), count_, ranges::end(base_))
                );
            }
            return *cached_begin_;
        } else {
            return ranges::next(ranges::begin(base_), count_, ranges::end(base_));
        }
    }

    constexpr auto begin() const requires random_access_range<V const> && sized_range<V const> {
        return ranges::next(ranges::begin(base_), count_, ranges::end(base_));
    }

    constexpr auto end() requires(!__detail::__simple_view<V>) { return ranges::end(base_); }

    constexpr auto end() const requires range<V const> { return ranges::end(base_); }

    constexpr auto size() requires sized_range<V> {
        auto const s = ranges::size(base_);
        auto const c = static_cast<decltype(s)>(count_);
        return s < c ? 0 : s - c;
    }

    constexpr auto size() const requires sized_range<V const> {
        auto const s = ranges::size(base_);
        auto const c = static_cast<decltype(s)>(count_);
        return s < c ? 0 : s - c;
    }

    constexpr auto reserve_hint() requires approximately_sized_range<V> {
        auto const s = static_cast<range_difference_t<V>>(ranges::reserve_hint(base_));
        return __detail::__to_unsigned_like(s < count_ ? 0 : s - count_);
    }

    constexpr auto reserve_hint() const requires approximately_sized_range<V const> {
        auto const s = static_cast<range_difference_t<V const>>(ranges::reserve_hint(base_));
        return __detail::__to_unsigned_like(s < count_ ? 0 : s - count_);
    }
};

  template<class T>
    constexpr bool enable_borrowed_range<drop_view<T>> =
      enable_borrowed_range<T>;

// Deduction Guide [range.drop.view]
template<class R> drop_view(R&&, range_difference_t<R>) -> drop_view<views::all_t<R>>;

namespace views {

// FIXME expr-equivalency
inline constexpr struct __drop_fn {
    template<class R, class Count> requires requires(R&& r, Count&& count) {
        requires convertible_to<Count, range_difference_t<R>>;
    } constexpr auto operator()(R&& r, Count&& count) const {
        using T = remove_cvref_t<R>;
        using D = range_difference_t<R>;

        // (2.1) empty_view specialization
        if constexpr (__is_specialization_of_v<empty_view, T>) {
            return auto(FWD(r));
        } else if constexpr (__is_specialization_of_v<optional, T> && view<T>) {
            return static_cast<D>(FWD(count)) == D() ? auto(FWD(r)) : T();
        }
        // (2.3) Sized random_access ranges: iota_view or subrange with StoreSize == false
        else if constexpr (
            random_access_range<T> && sized_range<T>
            && (__is_specialization_of_v<basic_string_view, T> || __is_span_v<T>
                || __is_iota_view_v<T>
                || (__is_subrange_v<T> && !__detail::__subrange_stores_size<T>))
        ) {
            auto b = begin(r) + std::min<D>(distance(r), count);
            if constexpr (__is_span_v<T>) {
                using U = span<typename T::element_type>;
                return U(b, end(r));
            } else {
                return T(b, end(r));
            }
        }
        // (2.4) Sized random_access subrange with StoreSize == true
        else if constexpr (random_access_range<T> && sized_range<T> && __is_subrange_v<T>) {
            auto dst = distance(r);
            auto d   = std::min<D>(dst, count);
            return T(begin(r) + d, end(r), __detail::__to_unsigned_like(dst - d));
        } else if constexpr (__is_specialization_of_v<repeat_view, T>) {
            if constexpr (sized_range<T>) {
                auto d = distance(r);
                return views::repeat(*r.__value(), d - std::min<D>(d, count));
            } else {
                return auto(FWD(r));
            }
        }
        // (2.6) General fallback
        else {
            return drop_view(FWD(r), static_cast<D>(count));
        }
    }

    template<class Count>
    requires is_object_v<decay_t<Count>> && constructible_from<decay_t<Count>, Count>
    constexpr auto operator()(Count&& count) const {
        return __range_adaptor_closure_fn(bind_back<__drop_fn>(FWD(count)));
    }
} drop;

}  // namespace views

}  // namespace std::ranges
