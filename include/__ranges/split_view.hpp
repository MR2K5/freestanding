#pragma once

#include <__algorithm/nonmodifying.hpp>
#include <__ranges/core.hpp>
#include <__ranges/single.hpp>
#include <__ranges/subrange.hpp>
#include <functional>
#include <optional>

namespace std::ranges {

template<forward_range V, forward_range Pattern>
requires view<V> && view<Pattern>
      && indirectly_comparable<iterator_t<V>, iterator_t<Pattern>, ranges::equal_to>
class split_view: public view_interface<split_view<V, Pattern>> {
private:
    V base_          = V();
    Pattern pattern_ = Pattern();

    __detail::non_propagating_cache<subrange<iterator_t<V>>> cache_;

    // [range.split.iterator], class split_view​::​iterator
    struct iterator {
    private:
        split_view* parent_           = nullptr;
        iterator_t<V> cur_            = iterator_t<V>();
        subrange<iterator_t<V>> next_ = subrange<iterator_t<V>>();
        bool trailing_empty_          = false;

        constexpr iterator(split_view& parent, iterator_t<V> current, subrange<iterator_t<V>> next)
            : parent_(&parent), cur_(std::move(current)), next_(std::move(next)) {}

        friend split_view;
        friend class sentinel;

    public:
        using iterator_concept  = forward_iterator_tag;
        using iterator_category = input_iterator_tag;
        using value_type        = subrange<iterator_t<V>>;
        using difference_type   = range_difference_t<V>;

        iterator() = default;

        constexpr iterator_t<V> base() const { return cur_; }
        constexpr value_type operator*() const { return {cur_, next_.begin()}; }

        constexpr iterator& operator++() {
            cur_ = next_.begin();
            if (cur_ != ranges::end(parent_->base_)) {
                cur_ = next_.end();
                if (cur_ == ranges::end(parent_->base_)) {
                    trailing_empty_ = true;
                    next_           = {cur_, cur_};
                } else {
                    next_ = parent_->find - next(cur_);
                }
            } else {
                trailing_empty_ = false;
            }
            return *this;
        }
        iterator operator++(int) {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        friend constexpr bool operator==(iterator const& x, iterator const& y) {
            return x.cur_ == y.cur_ && x.trailing_empty_ == y.trailing_empty_;
        }
    };

    // [range.split.sentinel], class split_view​::​sentinel
    class sentinel {
    private:
        sentinel_t<V> end_ = sentinel_t<V>();
        constexpr explicit sentinel(split_view& parent): end_(ranges::end(parent.base_)) {}

        friend split_view;

    public:
        sentinel() = default;

        friend constexpr bool operator==(iterator const& x, sentinel const& y) {
            return x.cur_ = y.end_;
        }
    };

public:
    split_view() requires default_initializable<V> && default_initializable<Pattern> = default;
    constexpr explicit split_view(V base, Pattern pattern)
        : base_(std::move(base)), pattern_(std::move(pattern)) {}

    template<forward_range R>
    requires constructible_from<V, views::all_t<R>>
              && constructible_from<Pattern, single_view<range_value_t<R>>>
    constexpr explicit split_view(R&& r, range_value_t<R> e)
        : base_(views::all(FWD(r))), pattern_(views::single(std::move(e))) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr iterator begin() {
        if (!cache_) cache_.emplace(__find_next(ranges::begin(base_)));
        return {*this, ranges::begin(base_), *cache_};
    }

    constexpr auto end() {
        if constexpr (common_range<V>) {
            return iterator{*this, ranges::end(base_), {}};
        } else {
            return sentinel{*this};
        }
    }

    constexpr subrange<iterator_t<V>> __find_next(iterator_t<V> it) {
        auto [b, e] = ranges::search(subrange(it, ranges::end(base_)), pattern_);
        if (b != ranges::end(base_) && ranges::empty(pattern_)) {
            ++b;
            ++e;
        }
        return {b, e};
    }
};

template<class R, class P> split_view(R&&, P&&) -> split_view<views::all_t<R>, views::all_t<P>>;

template<forward_range R>
split_view(R&&, range_value_t<R>) -> split_view<views::all_t<R>, single_view<range_value_t<R>>>;

namespace views {

inline constexpr struct __split_fn {
    static constexpr auto operator()(auto&& e, auto&& f) _STD_RETURN(split_view(FWD(e), FWD(f)));

    static constexpr auto operator()(auto&& f) noexcept {
        return __range_adaptor_closure_fn(std::bind_back<__split_fn>(FWD(f)));
    }
} split;

}  // namespace views

}  // namespace std::ranges
