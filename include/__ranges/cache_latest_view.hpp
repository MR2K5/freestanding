#pragma once

#include <__ranges/core.hpp>
#include <optional>

namespace std::ranges {
template<input_range V> requires view<V>
class cache_latest_view: public view_interface<cache_latest_view<V>> {
    V base_       = V();
    using cache_t = conditional_t<
        is_reference_v<range_reference_t<V>>, add_pointer_t<range_reference_t<V>>,
        range_reference_t<V>>;

    __detail::non_propagating_cache<cache_t> cache_;

    // [range.cache.latest.iterator], class cache_latest_view​::​iterator
    class iterator {
        cache_latest_view* parent_;
        iterator_t<V> current_;

        constexpr explicit iterator(cache_latest_view& parent)
            : parent_(&parent), current_(ranges::begin(parent.base_)) {}

        friend cache_latest_view;
        friend iterator;

    public:
        using difference_type  = range_difference_t<V>;
        using value_type       = range_value_t<V>;
        using iterator_concept = input_iterator_tag;

        iterator(iterator&&)            = default;
        iterator& operator=(iterator&&) = default;

        constexpr iterator_t<V> base() && { return std::move(current_); }
        constexpr iterator_t<V> const& base() const& noexcept { return current_; }

        constexpr range_reference_t<V>& operator*() const {
            if constexpr (is_reference_v<range_reference_t<V>>) {
                if (!parent_->cache_) { parent_->cache_ = std::addressof(__as_lvalue(*current_)); }
                return **parent_->cache_;
            } else {
                if (!parent_->cache_) { parent_->cache_.emplace - deref(current_); }
                return *parent_->cache_;
            }
        }

        constexpr iterator& operator++() {
            parent_->cache_.reset();
            ++current_;
            return *this;
        }
        constexpr void operator++(int) { ++*this; }

        friend constexpr range_rvalue_reference_t<V>
        iter_move(iterator const& i) noexcept(noexcept(ranges::iter_move(i.current_))) {
            return ranges::iter_move(i.current_);
        }

        friend constexpr void iter_swap(iterator const& x, iterator const& y) noexcept(
            noexcept(ranges::iter_swap(x.current_, y.current_))
        ) requires indirectly_swappable<iterator_t<V>> {
            ranges::iter_swap(x.current_, y.current_);
        }
    };

    // [range.cache.latest.sentinel], class cache_latest_view​::​sentinel
    class sentinel {
        sentinel_t<V> end_ = sentinel_t<V>();

        constexpr explicit sentinel(cache_latest_view& parent): end_(ranges::end(parent.base_)) {}
        friend cache_latest_view;

    public:
        sentinel() = default;

        constexpr sentinel_t<V> base() const { return end_; }

        friend constexpr bool operator==(iterator const& x, sentinel const& y) {
            return x.base() == y.end_;
        }

        friend constexpr range_difference_t<V> operator-(iterator const& x, sentinel const& y)
            requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>> {
            return x.base() - y.end_;
        }
        friend constexpr range_difference_t<V> operator-(sentinel const& x, iterator const& y)
            requires sized_sentinel_for<sentinel_t<V>, iterator_t<V>> {
            return x.end_ - y.base();
        }
    };

public:
    cache_latest_view() requires default_initializable<V> = default;
    constexpr explicit cache_latest_view(V base): base_(std::move(base)) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin() { return iterator(*this); }
    constexpr auto end() { return sentinel(*this); }

    constexpr auto size() requires sized_range<V> { return ranges::size(base_); }
    constexpr auto size() const requires sized_range<V const> { return ranges::size(base_); }

    constexpr auto reserve_hint() requires approximately_sized_range<V> {
        return ranges::reserve_hint(base_);
    }
    constexpr auto reserve_hint() const requires approximately_sized_range<V const> {
        return ranges::reserve_hint(base_);
    }
};

template<class R> cache_latest_view(R&&) -> cache_latest_view<views::all_t<R>>;

namespace views {

inline constexpr struct __cache_latest_fn: range_adaptor_closure<__cache_latest_fn> {
    static constexpr auto operator()(auto&& e) _STD_RETURN(cache_latest_view(FWD(e)));

} cache_latest;
}  // namespace views

}  // namespace std::ranges
