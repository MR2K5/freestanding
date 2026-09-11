#pragma once

#include <__ranges/core.hpp>
#include <__ranges/counted.hpp>
#include <optional>
#include <type_traits>
#include <utility>

namespace std::ranges {

template<class V> concept __slide_caches_nothing = random_access_range<V> && sized_range<V>;

template<class V>
concept slide_caches_last = !__slide_caches_nothing<V> && bidirectional_range<V> && common_range<V>;

template<class V> concept slide_caches_first = !__slide_caches_nothing<V> && !slide_caches_last<V>;

// Helper base to conditionally store last_ele_
template<bool HasLast, class Base> struct __slide_iterator_last_ele {};

template<class Base> struct __slide_iterator_last_ele<true, Base> {
    iterator_t<Base> last_ele_ = iterator_t<Base>();

    constexpr __slide_iterator_last_ele() = default;
    constexpr explicit __slide_iterator_last_ele(iterator_t<Base> last_ele)
        : last_ele_(std::move(last_ele)) {}
};

// =============================================================================
// 25.7.30.2 Class template slide_view [range.slide.view]
// =============================================================================

template<forward_range V> requires view<V> class slide_view: public view_interface<slide_view<V>> {
    V base_                  = V();
    range_difference_t<V> n_ = 0;

    [[no_unique_address]] __detail::non_propagating_cache<iterator_t<V>> cached_begin_;
    [[no_unique_address]] __detail::non_propagating_cache<iterator_t<V>> cached_end_;

    template<bool> class iterator;
    class sentinel;

public:
    slide_view() requires default_initializable<V> = default;

    constexpr explicit slide_view(V base, range_difference_t<V> n): base_(std::move(base)), n_(n) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin()
        requires(!(__detail::__simple_view<V> && __slide_caches_nothing<V const>)) {
        if constexpr (slide_caches_first<V>) {
            if (cached_begin_.has_value()) {
                return iterator<false>(ranges::begin(base_), *cached_begin_, n_);
            } else {
                auto last = ranges::next(ranges::begin(base_), n_ - 1, ranges::end(base_));
                cached_begin_.emplace(last);
                return iterator<false>(ranges::begin(base_), last, n_);
            }
        } else {
            return iterator<false>(ranges::begin(base_), n_);
        }
    }

    constexpr auto begin() const requires __slide_caches_nothing<V const> {
        return iterator<true>(ranges::begin(base_), n_);
    }

    constexpr auto end() requires(!(__detail::__simple_view<V> && __slide_caches_nothing<V const>))
    {
        if constexpr (__slide_caches_nothing<V>) {
            return iterator<false>(ranges::begin(base_) + range_difference_t<V>(size()), n_);
        } else if constexpr (slide_caches_last<V>) {
            if (cached_end_.has_value()) {
                return iterator<false>(*cached_end_, n_);
            } else {
                auto last = ranges::prev(ranges::end(base_), n_ - 1, ranges::begin(base_));
                cached_end_.emplace(last);
                return iterator<false>(last, n_);
            }
        } else if constexpr (common_range<V>) {
            return iterator<false>(ranges::end(base_), ranges::end(base_), n_);
        } else {
            return sentinel(ranges::end(base_));
        }
    }

    constexpr auto end() const requires __slide_caches_nothing<V const> {
        return begin() + range_difference_t<V const>(size());
    }

    constexpr auto size() requires sized_range<V> {
        auto sz = ranges::distance(base_) - n_ + 1;
        if (sz < 0) sz = 0;
        return __to_unsigned_like(sz);
    }

    constexpr auto size() const requires sized_range<V const> {
        auto sz = ranges::distance(base_) - n_ + 1;
        if (sz < 0) sz = 0;
        return __to_unsigned_like(sz);
    }

    constexpr auto reserve_hint() requires approximately_sized_range<V> {
        auto sz = static_cast<range_difference_t<decltype((base_))>>(ranges::reserve_hint(base_))
                - n_ + 1;
        if (sz < 0) sz = 0;
        return __to_unsigned_like(sz);
    }

    constexpr auto reserve_hint() const requires approximately_sized_range<V const> {
        auto sz = static_cast<range_difference_t<decltype((base_))>>(ranges::reserve_hint(base_))
                - n_ + 1;
        if (sz < 0) sz = 0;
        return __to_unsigned_like(sz);
    }
};

template<class R> slide_view(R&&, range_difference_t<R>) -> slide_view<views::all_t<R>>;

// =============================================================================
// 25.7.30.3 Class template slide_view::iterator [range.slide.iterator]
// =============================================================================

template<forward_range V> requires view<V> template<bool Const>
class slide_view<V>::iterator
    : public __slide_iterator_last_ele<
          slide_caches_first<__maybe_const<Const, V>>, __maybe_const<Const, V>> {

    using Base        = __maybe_const<Const, V>;
    using LastEleBase = __slide_iterator_last_ele<slide_caches_first<Base>, Base>;

    iterator_t<Base> current_   = iterator_t<Base>();
    range_difference_t<Base> n_ = 0;

    constexpr iterator(iterator_t<Base> current, range_difference_t<Base> n)
        requires(!slide_caches_first<Base>)
        : current_(std::move(current)), n_(n) {}

    constexpr iterator(
        iterator_t<Base> current, iterator_t<Base> last_ele, range_difference_t<Base> n
    ) requires slide_caches_first<Base>
        : LastEleBase(std::move(last_ele)), current_(std::move(current)), n_(n) {}

    friend class slide_view;
    template<bool> friend class iterator;
    friend class sentinel;

public:
    using iterator_category = input_iterator_tag;
    using iterator_concept  = std::conditional_t<
        random_access_range<Base>, random_access_iterator_tag,
        std::conditional_t<
            bidirectional_range<Base>, bidirectional_iterator_tag, forward_iterator_tag>>;
    using value_type      = decltype(views::counted(current_, n_));
    using difference_type = range_difference_t<Base>;

    iterator() = default;

    constexpr iterator(iterator<!Const> i)
        requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
        : current_(std::move(i.current_)), n_(i.n_) {}

    constexpr auto operator*() const { return views::counted(current_, n_); }

    constexpr iterator& operator++() {
        ++current_;
        if constexpr (slide_caches_first<Base>) { ++this->last_ele_; }
        return *this;
    }

    constexpr iterator operator++(int) {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr iterator& operator--() requires bidirectional_range<Base> {
        --current_;
        if constexpr (slide_caches_first<Base>) { --this->last_ele_; }
        return *this;
    }

    constexpr iterator operator--(int) requires bidirectional_range<Base> {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    constexpr iterator& operator+=(difference_type x) requires random_access_range<Base> {
        current_ += x;
        if constexpr (slide_caches_first<Base>) { this->last_ele_ += x; }
        return *this;
    }

    constexpr iterator& operator-=(difference_type x) requires random_access_range<Base> {
        current_ -= x;
        if constexpr (slide_caches_first<Base>) { this->last_ele_ -= x; }
        return *this;
    }

    constexpr auto operator[](difference_type n) const requires random_access_range<Base> {
        return views::counted(current_ + n, n_);
    }

    friend constexpr bool operator==(iterator const& x, iterator const& y) {
        if constexpr (slide_caches_first<Base>) {
            return x.last_ele_ == y.last_ele_;
        } else {
            return x.current_ == y.current_;
        }
    }

    friend constexpr bool operator<(iterator const& x, iterator const& y)
        requires random_access_range<Base> {
        return x.current_ < y.current_;
    }

    friend constexpr bool operator>(iterator const& x, iterator const& y)
        requires random_access_range<Base> {
        return y < x;
    }

    friend constexpr bool operator<=(iterator const& x, iterator const& y)
        requires random_access_range<Base> {
        return !(y < x);
    }

    friend constexpr bool operator>=(iterator const& x, iterator const& y)
        requires random_access_range<Base> {
        return !(x < y);
    }

    friend constexpr auto operator<=>(iterator const& x, iterator const& y)
        requires random_access_range<Base> && three_way_comparable<iterator_t<Base>> {
        return x.current_ <=> y.current_;
    }

    friend constexpr iterator operator+(iterator const& i, difference_type n)
        requires random_access_range<Base> {
        auto r  = i;
        r      += n;
        return r;
    }

    friend constexpr iterator operator+(difference_type n, iterator const& i)
        requires random_access_range<Base> {
        auto r  = i;
        r      += n;
        return r;
    }

    friend constexpr iterator operator-(iterator const& i, difference_type n)
        requires random_access_range<Base> {
        auto r  = i;
        r      -= n;
        return r;
    }

    friend constexpr difference_type operator-(iterator const& x, iterator const& y)
        requires sized_sentinel_for<iterator_t<Base>, iterator_t<Base>> {
        if constexpr (slide_caches_first<Base>) {
            return x.last_ele_ - y.last_ele_;
        } else {
            return x.current_ - y.current_;
        }
    }
};

// =============================================================================
// 25.7.30.4 Class slide_view::sentinel [range.slide.sentinel]
// =============================================================================

template<forward_range V> requires view<V> class slide_view<V>::sentinel {
    sentinel_t<V> end_ = sentinel_t<V>();

    constexpr explicit sentinel(sentinel_t<V> end): end_(std::move(end)) {}

    friend class slide_view;

public:
    sentinel() = default;

    friend constexpr bool operator==(iterator<false> const& x, sentinel const& y) {
        return x.last_ele_ == y.end_;
    }

    friend constexpr range_difference_t<V> operator-(iterator<false> const& x, sentinel const& y)
        requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>> {
        return x.last_ele_ - y.end_;
    }

    friend constexpr range_difference_t<V> operator-(sentinel const& y, iterator<false> const& x)
        requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>> {
        return y.end_ - x.last_ele_;
    }
};

template<class V> constexpr bool enable_borrowed_range<slide_view<V>> = enable_borrowed_range<V>;

// =============================================================================
// 25.7.30.1 Overview [range.slide.overview] - Range Adaptor Object
// =============================================================================

namespace views {

inline constexpr struct __slide_fn {
    constexpr auto operator()(auto&& n) const {
        return __range_adaptor_closure_fn(bind_back<__slide_fn>(FWD(n)));
    }

    constexpr auto operator()(auto&& E, auto&& N) _STD_RETURN(slide_view(FWD(E), FWD(N)));
} slide;

}  // namespace views

}  // namespace std::ranges
