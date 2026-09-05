#pragma once

#include <__algorithm/minmax.hpp>
#include <__ranges/core.hpp>
#include <__ranges/iota_view.hpp>
#include <__ranges/repeat.hpp>
#include <__ranges/subrange.hpp>
#include <concepts>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

namespace std::ranges {

// [range.take.view], class template take_view
template<view V> class take_view: public view_interface<take_view<V>> {
private:
    [[no_unique_address]] V base_ = V();
    range_difference_t<V> count_  = 0;

    // [range.take.sentinel], class template take_view::sentinel
    template<bool Const> class sentinel {
    private:
        using Base = __maybe_const<Const, V>;
        template<bool OtherConst>
        using CI = counted_iterator<iterator_t<__maybe_const<OtherConst, V>>>;

        sentinel_t<Base> end_ = sentinel_t<Base>();

        friend class take_view;
        template<bool> friend class sentinel;

        constexpr explicit sentinel(sentinel_t<Base> end): end_(std::move(end)) {}

    public:
        sentinel() = default;

        constexpr sentinel(sentinel<!Const> s)
            requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
            : end_(std::move(s.end_)) {}

        constexpr sentinel_t<Base> base() const { return end_; }

        friend constexpr bool operator==(const CI<Const>& y, sentinel const& x) {
            return y.count() == 0 || y.base() == x.end_;
        }

        template<bool OtherConst = !Const>
        requires sentinel_for<sentinel_t<Base>, iterator_t<__maybe_const<OtherConst, V>>>
        friend constexpr bool operator==(const CI<OtherConst>& y, sentinel const& x) {
            return y.count() == 0 || y.base() == x.end_;
        }
    };

public:
    take_view() requires default_initializable<V> = default;

    constexpr explicit take_view(V base, range_difference_t<V> count)
        : base_(std::move(base)), count_(count) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin() requires(!__detail::__simple_view<V>) {
        if constexpr (sized_range<V>) {
            if constexpr (random_access_range<V>) {
                return ranges::begin(base_);
            } else {
                auto sz = range_difference_t<V>(size());
                return counted_iterator(ranges::begin(base_), sz);
            }
        } else if constexpr (sized_sentinel_for<sentinel_t<V>, iterator_t<V>>) {
            auto it = ranges::begin(base_);
            auto sz = std::min(count_, ranges::end(base_) - it);
            return counted_iterator(std::move(it), sz);
        } else {
            return counted_iterator(ranges::begin(base_), count_);
        }
    }

    constexpr auto begin() const requires range<V const> {
        if constexpr (sized_range<V const>) {
            if constexpr (random_access_range<V const>) {
                return ranges::begin(base_);
            } else {
                auto sz = range_difference_t<V const>(size());
                return counted_iterator(ranges::begin(base_), sz);
            }
        } else if constexpr (sized_sentinel_for<sentinel_t<V const>, iterator_t<V const>>) {
            auto it = ranges::begin(base_);
            auto sz = std::min(count_, ranges::end(base_) - it);
            return counted_iterator(std::move(it), sz);
        } else {
            return counted_iterator(ranges::begin(base_), count_);
        }
    }

    constexpr auto end() requires(!__detail::__simple_view<V>) {
        if constexpr (sized_range<V>) {
            if constexpr (random_access_range<V>) {
                return ranges::begin(base_) + range_difference_t<V>(size());
            } else {
                return default_sentinel;
            }
        } else if constexpr (sized_sentinel_for<sentinel_t<V>, iterator_t<V>>) {
            return default_sentinel;
        } else {
            return sentinel<false>{ranges::end(base_)};
        }
    }

    constexpr auto end() const requires range<V const> {
        if constexpr (sized_range<V const>) {
            if constexpr (random_access_range<V const>) {
                return ranges::begin(base_) + range_difference_t<V const>(size());
            } else {
                return default_sentinel;
            }
        } else if constexpr (sized_sentinel_for<sentinel_t<V const>, iterator_t<V const>>) {
            return default_sentinel;
        } else {
            return sentinel<true>{ranges::end(base_)};
        }
    }

    constexpr auto size() requires sized_range<V> {
        auto n = ranges::size(base_);
        return std::min(n, static_cast<decltype(n)>(count_));
    }

    constexpr auto size() const requires sized_range<V const> {
        auto n = ranges::size(base_);
        return std::min(n, static_cast<decltype(n)>(count_));
    }
};

  template<class T>
    constexpr bool enable_borrowed_range<take_view<T>> =
      enable_borrowed_range<T>;

// Deduction Guide
template<class R> take_view(R&&, range_difference_t<R>) -> take_view<views::all_t<R>>;

namespace __detail {

template<class T> struct __subrange_or_span_target;

template<class ElementType, size_t Extent>
struct __subrange_or_span_target<std::span<ElementType, Extent>> {
    using type = std::span<ElementType>;
};

template<class CharT, class Traits>
struct __subrange_or_span_target<std::basic_string_view<CharT, Traits>> {
    using type = std::basic_string_view<CharT, Traits>;
};

template<class I, class S, subrange_kind K> struct __subrange_or_span_target<subrange<I, S, K>> {
    using type = subrange<I>;
};

}  // namespace __detail

namespace views {

inline constexpr struct __take_fn {
private:
    // (2.1) empty_view specialization
    template<class E, class F, class T = remove_cvref_t<E>, class D = range_difference_t<E>>
    requires __is_specialization_of_v<empty_view, T> && convertible_to<F, D> static constexpr auto
    __impl(E&& e, F&&, __detail::__priority_tag<5>) noexcept(noexcept(auto(FWD(e))))
        -> decltype(auto(FWD(e))) {
        return auto(FWD(e));
    }

    // (2.2) optional specialization modeling view
    template<class E, class F, class T = remove_cvref_t<E>, class D = range_difference_t<E>>
    requires(__is_specialization_of_v<optional, T> && view<T>) && convertible_to<F, D>
    static constexpr auto __impl(E&& e, F&& f, __detail::__priority_tag<4>) noexcept(
        noexcept(static_cast<D>(f) == D() ? ((void)e, T()) : auto(FWD(e)))
    ) -> decltype(static_cast<D>(f) == D() ? ((void)e, T()) : auto(FWD(e))) {
        return static_cast<D>(f) == D() ? ((void)e, T()) : auto(FWD(e));
    }

    // (2.3) span, basic_string_view, or subrange specialization
    template<class E, class F, class T = remove_cvref_t<E>, class D = range_difference_t<E>>
    requires(random_access_range<T> && sized_range<T>
             && (__is_span_v<T> || __is_specialization_of_v<basic_string_view, T>
                 || __is_subrange_v<T>))
         && convertible_to<F, D>
    static constexpr auto __impl(E&& e, F&& f, __detail::__priority_tag<3>) noexcept(
        noexcept(typename __detail::__subrange_or_span_target<T>::type(
            ranges::begin(e), ranges::begin(e) + std::min<D>(ranges::distance(e), static_cast<D>(f))
        ))
    ) -> typename __detail::__subrange_or_span_target<T>::type {
        using U    = typename __detail::__subrange_or_span_target<T>::type;
        auto b     = ranges::begin(e);
        auto d     = ranges::distance(e);
        auto count = std::min<D>(d, static_cast<D>(f));
        return U(b, b + count);
    }

    // (2.4) iota_view specialization
    template<class E, class F, class T = remove_cvref_t<E>, class D = range_difference_t<E>>
    requires(__is_iota_view_v<T> && random_access_range<T> && sized_range<T>)
         && convertible_to<F, D> static constexpr auto
    __impl(E&& e, F&& f, __detail::__priority_tag<2>) noexcept(noexcept(iota_view(
        *ranges::begin(e), *(ranges::begin(e) + std::min<D>(ranges::distance(e), static_cast<D>(f)))
    )))
        -> decltype(iota_view(
            *ranges::begin(e),
            *(ranges::begin(e) + std::min<D>(ranges::distance(e), static_cast<D>(f)))
        )) {
        auto b     = ranges::begin(e);
        auto d     = ranges::distance(e);
        auto count = std::min<D>(d, static_cast<D>(f));
        return iota_view(*b, *(b + count));
    }

    // (2.5) repeat_view specialization
    template<class E, class F, class T = remove_cvref_t<E>, class D = range_difference_t<E>>
    requires __is_specialization_of_v<repeat_view, T> && convertible_to<F, D>
    static constexpr auto __impl(E&& e, F&& f, __detail::__priority_tag<1>) {
        if constexpr (sized_range<T>) {
            auto d = ranges::distance(e);
            return views::repeat(*e.value_, std::min<D>(d, static_cast<D>(f)));
        } else {
            return views::repeat(*e.value_, static_cast<D>(f));
        }
    }

    // (2.6) Fallback: take_view(E, F)
    template<class E, class F, class D = range_difference_t<E>>
    requires convertible_to<F, D> && requires { take_view(std::declval<E>(), std::declval<F>()); }
    static constexpr auto
    __impl(E&& e, F&& f, __detail::__priority_tag<0>) noexcept(noexcept(take_view(FWD(e), FWD(f))))
        -> decltype(take_view(FWD(e), FWD(f))) {
        return take_view(FWD(e), FWD(f));
    }

public:
    template<viewable_range E, typename F> requires requires {
        __impl(std::declval<E>(), std::declval<F>(), __detail::__priority_tag<5>{});
    }
    constexpr auto operator()(E&& e, F&& f) const noexcept(
        noexcept(__impl(FWD(e), FWD(f), __detail::__priority_tag<5>{}))
    ) -> decltype(__impl(FWD(e), FWD(f), __detail::__priority_tag<5>{})) {
        return __impl(FWD(e), FWD(f), __detail::__priority_tag<5>{});
    }

    template<typename F> constexpr auto operator()(F&& f) const {
        return __range_adaptor_closure_fn(std::bind_back(*this, FWD(f)));
    }
} take;

}  // namespace views

}  // namespace std::ranges
