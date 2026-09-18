#pragma once

#include <__algorithm/nonmodifying.hpp>
#include <__ranges/core.hpp>
#include <__ranges/single.hpp>
#include <__ranges/subrange.hpp>

#include <optional>
#include <type_traits>

namespace std::ranges {
template<auto> struct __require_constant;

template<class R>
concept __tiny_range = sized_range<R> && requires {
    typename __require_constant<remove_reference_t<R>::size()>;
} && (remove_reference_t<R>::size() <= 1);

template<input_range V, forward_range Pattern>
requires view<V> && view<Pattern>
      && indirectly_comparable<iterator_t<V>, iterator_t<Pattern>, ranges::equal_to>
      && (forward_range<V> || __tiny_range<Pattern>)class lazy_split_view
    : public view_interface<lazy_split_view<V, Pattern>> {
private:
    V base_          = V();
    Pattern pattern_ = Pattern();

    [[no_unique_address]] conditional_t<
        forward_range<V>, __empty, __detail::non_propagating_cache<iterator_t<V>>>
        current_;  // , present only if forward_range<V> is false

    // [range.lazy.split.outer], class template lazy_split_view​::​outer_iterator
    template<bool> struct __iter_base {};
    template<bool Const> requires forward_range<__maybe_const<Const, V>> struct __iter_base<Const> {
        using iterator_category = input_iterator_tag;
    };

    template<bool> struct inner_iterator;

    template<bool Const> struct outer_iterator: __iter_base<Const> {
    private:
        using Parent    = __maybe_const<Const, lazy_split_view>;
        using Base      = __maybe_const<Const, V>;
        Parent* parent_ = nullptr;

        [[no_unique_address]] conditional_t<forward_range<Base>, iterator_t<Base>, __empty>
            current_ = iterator_t<Base>();  // present only if V models forward_range

        bool trailing_empty_ = false;

        constexpr explicit outer_iterator(Parent& parent) requires(!forward_range<Base>)
            : parent_(&parent) {}
        constexpr outer_iterator(Parent& parent, iterator_t<Base> current)
            requires forward_range<Base>
            : parent_(&parent), current_(std::move(current)) {}

        friend lazy_split_view;
        template<bool> friend struct outer_iterator;

    public:
        using iterator_concept =
            conditional_t<forward_range<Base>, forward_iterator_tag, input_iterator_tag>;

        // [range.lazy.split.outer.value], class
        // lazy_split_view​::​outer_iterator​::​value_type
        struct value_type: view_interface<value_type> {
        private:
            outer_iterator i_ = outer_iterator();

            constexpr explicit value_type(outer_iterator i): i_(std::move(i)) {}
            friend outer_iterator;

        public:
            constexpr inner_iterator<Const> begin() const { return inner_iterator<Const>(i_); }
            constexpr default_sentinel_t end() const noexcept { return {}; }
        };

        using difference_type = range_difference_t<Base>;

        outer_iterator() = default;
        constexpr outer_iterator(outer_iterator<!Const> i)
            requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
            : parent_(i.parent_), current_(std::move(i.current_)),
              trailing_empty_(i.trailing_empty_) {}

        constexpr value_type operator*() const { return value_type(*this); }

        constexpr outer_iterator& operator++() {
            auto const end = ranges::end(parent_->base_);
            if (current_ == end) {
                trailing_empty_ = false;
                return *this;
            }
            auto const [pbegin, pend] = subrange{parent_->pattern_};
            if (pbegin == pend)
                ++current_;
            else if constexpr (__tiny_range<Pattern>) {
                current_ = ranges::find(std::move(current_), end, *pbegin);
                if (current_ != end) {
                    ++current_;
                    if (current_ == end)
                        trailing_empty_ = true;
                    else if constexpr (!forward_range<V>)
                        trailing_empty_ = true;
                }
            } else {
                do {
                    auto [b, p] = ranges::mismatch(current_, end, pbegin, pend);
                    if (p == pend) {
                        current_ = b;
                        if (current_ == end) trailing_empty_ = true;
                        break;  // The pattern matched; skip it
                    }
                } while (++current_ != end);
            }
            return *this;
        }

        constexpr void operator++(int) { ++*this; }
        outer_iterator operator++(int) requires forward_range<Base> {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        friend constexpr bool operator==(outer_iterator const& x, outer_iterator const& y)
            requires forward_range<Base> {
            return x.current_ == y.current_ && x.trailing_empty_ == y.trailing_empty_;
        }

        friend constexpr bool operator==(outer_iterator const& x, default_sentinel_t) {
            return x.current == ranges::end(x.parent_->base_) && !x.trailing_empty_;
        }
    };

    // [range.lazy.split.inner], class template lazy_split_view​::​inner_iterator
    template<bool Const> struct inner_iterator: __iter_base<Const> {
    private:
        using Base               = __maybe_const<Const, V>;
        outer_iterator<Const> i_ = outer_iterator<Const>();
        bool incremented_        = false;

        constexpr explicit inner_iterator(outer_iterator<Const> i): i_(std::move(i)) {}

        template<bool> friend struct outer_iterator;
        friend lazy_split_view;

    public:
        using iterator_concept = outer_iterator<Const>::iterator_concept;

        using value_type      = range_value_t<Base>;
        using difference_type = range_difference_t<Base>;

        inner_iterator() = default;

        constexpr iterator_t<Base> const& base() const& noexcept { return i_.current_; }
        constexpr iterator_t<Base> base() && requires forward_range<V> {
            return std::move(i_.current_);
        }

        constexpr decltype(auto) operator*() const { return *i_.current; }

        constexpr inner_iterator& operator++() {
            incremented_ = true;
            if constexpr (!forward_range<Base>) {
                if constexpr (Pattern::size() == 0) { return *this; }
            }
            ++i_.current;
            return *this;
        }
        constexpr void operator++(int) { ++*this; }
        inner_iterator operator++(int) requires forward_range<Base> {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        friend constexpr bool operator==(inner_iterator const& x, inner_iterator const& y)
            requires forward_range<Base> {
            return x.i_.current_ == y.i_.current_;
        }

        friend constexpr bool operator==(inner_iterator const& x, default_sentinel_t) {
            auto [pcur, pend] = subrange{x.i_.parent_->pattern_};
            auto end          = ranges::end(x.i_.parent_->base_);
            if constexpr (__tiny_range<Pattern>) {
                auto const& cur = x.i_.current;
                if (cur == end) return true;
                if (pcur == pend) return x.incremented_;
                return *cur == *pcur;
            } else {
                auto cur = x.i_.current;
                if (cur == end) return true;
                if (pcur == pend) return x.incremented_;
                do {
                    if (*cur != *pcur) return false;
                    if (++pcur == pend) return true;
                } while (++cur != end);
                return false;
            }
        }

        friend constexpr decltype(auto)
        iter_move(inner_iterator const& i) noexcept(noexcept(ranges::iter_move(i.i_.current))) {
            return ranges::iter_move(i.i_.current);
        }

        friend constexpr void iter_swap(inner_iterator const& x, inner_iterator const& y) noexcept(
            noexcept(ranges::iter_swap(x.i_.current, y.i_.current))
        ) requires indirectly_swappable<iterator_t<Base>> {
            ranges::iter_swap(x.i_.current_, y.i_.current_);
        }
    };

public:
    lazy_split_view() requires default_initializable<V> && default_initializable<Pattern> = default;
    constexpr explicit lazy_split_view(V base, Pattern pattern)
        : base_(std::move(base)), pattern_(std::move(pattern)) {}

    template<input_range R> requires constructible_from<V, views::all_t<R>>
                                      && constructible_from<Pattern, single_view<range_value_t<R>>>
    constexpr explicit lazy_split_view(R&& r, range_value_t<R> e)
        : base_(views::all(FWD(r))), pattern_(views::single(std::move(e))) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin() {
        if constexpr (forward_range<V>) {
            return outer_iterator<__detail::__simple_view<V> && __detail::__simple_view<Pattern>>{
                *this, ranges::begin(base_)
            };
        } else {
            current_ = ranges::begin(base_);
            return outer_iterator<false>{*this};
        }
    }

    constexpr auto begin() const
        requires forward_range<V> && forward_range<V const> && forward_range<Pattern const> {
        return outer_iterator<true>{*this, ranges::begin(base_)};
    }

    constexpr auto end() requires forward_range<V> && common_range<V> {
        return outer_iterator<__detail::__simple_view<V> && __detail::__simple_view<Pattern>>{
            *this, ranges::end(base_)
        };
    }

    constexpr auto end() const {
        if constexpr (
            forward_range<V> && forward_range<V const> && common_range<V const>
            && forward_range<Pattern const>
        )
            return outer_iterator<true>{*this, ranges::end(base_)};
        else
            return default_sentinel;
    }
};

template<class R, class P>
lazy_split_view(R&&, P&&) -> lazy_split_view<views::all_t<R>, views::all_t<P>>;

template<input_range R>
lazy_split_view(R&&, range_value_t<R>)
    -> lazy_split_view<views::all_t<R>, single_view<range_value_t<R>>>;

namespace views {

inline constexpr struct __lazy_split_fn {
    static constexpr auto
    operator()(auto&& e, auto&& f) _STD_RETURN(lazy_split_view(FWD(e), FWD(f)));

    static constexpr auto operator()(auto&& f) noexcept {
        return __range_adaptor_closure_fn(std::bind_back<__lazy_split_fn>(FWD(f)));
    }
} lazy_split;

}  // namespace views

}  // namespace std::ranges
