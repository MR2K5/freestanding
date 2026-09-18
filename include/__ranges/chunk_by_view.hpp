#pragma once

#include <__algorithm/nonmodifying.hpp>
#include <__ranges/core.hpp>
#include <cassert>
#include <optional>
#include <type_traits>

namespace std::ranges {
template<forward_range V, indirect_binary_predicate<iterator_t<V>, iterator_t<V>> Pred>
requires view<V> && is_object_v<Pred>
class chunk_by_view: public view_interface<chunk_by_view<V, Pred>> {
    V base_ = V();
    __detail::movable_box<Pred> pred_;
    __detail::non_propagating_cache<iterator_t<V>> cache_;

    // [range.chunk.by.iter], class chunk_by_view​::​iterator
    class iterator {
        chunk_by_view* parent_ = nullptr;
        iterator_t<V> current_ = iterator_t<V>();
        iterator_t<V> next_    = iterator_t<V>();

        constexpr iterator(chunk_by_view& parent, iterator_t<V> current, iterator_t<V> next)
            : parent_(&parent), current_(std::move(current)), next_(std::move(next)) {}

        friend chunk_by_view;

    public:
        using value_type        = subrange<iterator_t<V>>;
        using difference_type   = range_difference_t<V>;
        using iterator_category = input_iterator_tag;
        using iterator_concept =
            conditional_t<bidirectional_range<V>, bidirectional_iterator_tag, forward_iterator_tag>;

        iterator() = default;

        constexpr value_type operator*() const { return subrange(current_, next_); }
        constexpr iterator& operator++() {
            current_ = next_;
            next_    = parent_->find - next(current_);
            return *this;
        }
        iterator operator++(int) {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        constexpr iterator& operator--() requires bidirectional_range<V> {
            next_    = current_;
            current_ = parent_->find - prev(next_);
            return *this;
        }
        iterator operator--(int) requires bidirectional_range<V> {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        friend constexpr bool operator==(iterator const& x, iterator const& y) {
            return x.current_ == y.current_;
        }
        friend constexpr bool operator==(iterator const& x, default_sentinel_t) {
            return x.current_ == x.next_;
        }
    };

public:
    chunk_by_view() requires default_initializable<V> && default_initializable<Pred> = default;
    constexpr explicit chunk_by_view(V base, Pred pred)
        : base_(std::move(base)), pred_(std::move(pred)) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr Pred const& pred() const { return *pred_; }

    constexpr iterator begin() {
        assert(pred_.has_value);
        if (!cache_.has_value()) cache_ = __find_next(ranges::begin(base_));
        return iterator(*this, ranges::begin(base_), *cache_);
    }
    constexpr auto end() {
        if constexpr (common_range<V>) {
            return iterator(*this, ranges::end(base_), ranges::end(base_));
        } else {
            return default_sentinel;
        }
    }

    constexpr iterator_t<V> __find_next(iterator_t<V> current) {
        return ranges::next(
            ranges::adjacent_find(current, ranges::end(base_), not_fn(ref(*pred_))), 1,
            ranges::end(base_)
        );
    }
    constexpr iterator_t<V> __find_prev(iterator_t<V> current) requires bidirectional_range<V> {
        auto first = ranges::begin(base_);
        auto i     = current;

        while (i != first) {
            auto prev_i = ranges::prev(i);
            // If the pair crossing into `i` fails the predicate, `i` is the chunk boundary
            if (!std::invoke(*pred_, *prev_i, *i)) { return i; }
            i = prev_i;
        }

        return first;
    }
};

template<class R, class Pred> chunk_by_view(R&&, Pred) -> chunk_by_view<views::all_t<R>, Pred>;

namespace views {

inline constexpr struct __chunk_by_fn {
    static constexpr auto operator()(auto&& e, auto&& f) _STD_RETURN(chunk_by_view(FWD(e), FWD(f)));

    static constexpr auto operator()(auto&& f) noexcept {
        return __range_adaptor_closure_fn(std::bind_back<__chunk_by_fn>(FWD(f)));
    }
} chunk_by;

}  // namespace views

}  // namespace std::ranges
