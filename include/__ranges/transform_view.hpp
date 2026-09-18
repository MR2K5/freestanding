#pragma once

#include <__ranges/core.hpp>
#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>

namespace std::ranges {

template<input_range V, move_constructible F>
requires view<V> && is_object_v<F> && regular_invocable<F&, range_reference_t<V>>
      && __detail::__referenceable<invoke_result_t<F&, range_reference_t<V>>>
class transform_view: public view_interface<transform_view<V, F>> {
    [[no_unique_address]] V base_ = V();
    [[no_unique_address]] __detail::movable_box<F> fun_;

    template<bool B> struct __iter_category_base {};

    template<bool B> requires forward_range<__maybe_const<B, V>> struct __iter_category_base<B> {
        using __Base = __maybe_const<B, V>;
        using __Fn   = __maybe_const<B, F>;
        using __Res  = invoke_result_t<__Fn&, range_reference_t<__Base>>;
        using __C    = typename iterator_traits<iterator_t<__Base>>::iterator_category;

        using iterator_category = conditional_t<
            is_reference_v<__Res>,
            conditional_t<
                derived_from<__C, contiguous_iterator_tag>, random_access_iterator_tag, __C>,
            input_iterator_tag>;
    };

    // [range.transform.iterator], class template transform_view::iterator
    template<bool Const> class iterator: public __iter_category_base<Const> {
        using Parent          = __maybe_const<Const, transform_view>;
        using Base            = __maybe_const<Const, V>;
        iterator_t<Base> cur_ = iterator_t<Base>();
        Parent* parent_       = nullptr;

        friend class transform_view;
        template<bool> friend class iterator;
        // template<bool> friend class sentinel;

        constexpr iterator(Parent& parent, iterator_t<Base> current)
            : cur_(std::move(current)), parent_(std::addressof(parent)) {}

    public:
        using iterator_concept = conditional_t<
            random_access_range<Base>, random_access_iterator_tag,
            conditional_t<
                bidirectional_range<Base>, bidirectional_iterator_tag,
                conditional_t<forward_range<Base>, forward_iterator_tag, input_iterator_tag>>>;
        using value_type =
            remove_cvref_t<invoke_result_t<__maybe_const<Const, F>&, range_reference_t<Base>>>;
        using difference_type = range_difference_t<Base>;

        iterator() requires default_initializable<iterator_t<Base>> = default;

        constexpr iterator(iterator<!Const> i)
            requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
            : cur_(std::move(i.cur_)), parent_(i.parent_) {}

        constexpr iterator_t<Base> const& base() const& noexcept { return cur_; }
        constexpr iterator_t<Base> base() && { return std::move(cur_); }

        constexpr decltype(auto)
        operator*() const noexcept(noexcept(std::invoke(*parent_->fun_, *cur_))) {
            return std::invoke(*parent_->fun_, *cur_);
        }

        constexpr iterator& operator++() {
            ++cur_;
            return *this;
        }

        constexpr void operator++(int) { ++cur_; }

        constexpr iterator operator++(int) requires forward_range<Base> {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        constexpr iterator& operator--() requires bidirectional_range<Base> {
            --cur_;
            return *this;
        }

        constexpr iterator operator--(int) requires bidirectional_range<Base> {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type n) requires random_access_range<Base> {
            cur_ += n;
            return *this;
        }

        constexpr iterator& operator-=(difference_type n) requires random_access_range<Base> {
            cur_ -= n;
            return *this;
        }

        constexpr decltype(auto) operator[](difference_type n) const
            requires random_access_range<Base> {
            return std::invoke(*parent_->fun_, cur_[n]);
        }

        friend constexpr bool operator==(iterator const& x, iterator const& y)
            requires equality_comparable<iterator_t<Base>> {
            return x.cur_ == y.cur_;
        }

        friend constexpr bool operator<(iterator const& x, iterator const& y)
            requires random_access_range<Base> {
            return x.cur_ < y.cur_;
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
            return x.cur_ <=> y.cur_;
        }

        friend constexpr iterator operator+(iterator i, difference_type n)
            requires random_access_range<Base> {
            return iterator{*i.parent_, i.cur_ + n};
        }

        friend constexpr iterator operator+(difference_type n, iterator i)
            requires random_access_range<Base> {
            return iterator{*i.parent_, i.cur_ + n};
        }

        friend constexpr iterator operator-(iterator i, difference_type n)
            requires random_access_range<Base> {
            return iterator{*i.parent_, i.cur_ - n};
        }

        friend constexpr difference_type operator-(iterator const& x, iterator const& y)
            requires sized_sentinel_for<iterator_t<Base>, iterator_t<Base>> {
            return x.cur_ - y.cur_;
        }

        friend constexpr void iter_swap(iterator const& x, iterator const& y) noexcept(
            noexcept(ranges::iter_swap(x.cur_, y.cur_))
        ) requires indirectly_swappable<iterator_t<Base>> {
            ranges::iter_swap(x.cur_, y.cur_);
        }
    };

    // [range.transform.sentinel], class template transform_view::sentinel
    template<bool Const> class sentinel {
        using Parent          = __maybe_const<Const, transform_view>;
        using Base            = __maybe_const<Const, V>;
        sentinel_t<Base> end_ = sentinel_t<Base>();

        friend class transform_view;
        template<bool> friend class sentinel;

        constexpr explicit sentinel(sentinel_t<Base> end): end_(std::move(end)) {}

    public:
        sentinel() = default;

        constexpr sentinel(sentinel<!Const> i)
            requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
            : end_(std::move(i.end_)) {}

        constexpr sentinel_t<Base> base() const { return end_; }

        template<bool OtherConst>
        requires sentinel_for<sentinel_t<Base>, iterator_t<__maybe_const<OtherConst, V>>>
        friend constexpr bool operator==(iterator<OtherConst> const& x, sentinel const& y) {
            return x.cur_ == y.end_;
        }

        template<bool OtherConst>
        requires sized_sentinel_for<sentinel_t<Base>, iterator_t<__maybe_const<OtherConst, V>>>
        friend constexpr range_difference_t<__maybe_const<OtherConst, V>>
        operator-(iterator<OtherConst> const& x, sentinel const& y) {
            return x.cur_ - y.end_;
        }

        template<bool OtherConst>
        requires sized_sentinel_for<sentinel_t<Base>, iterator_t<__maybe_const<OtherConst, V>>>
        friend constexpr range_difference_t<__maybe_const<OtherConst, V>>
        operator-(sentinel const& y, iterator<OtherConst> const& x) {
            return y.end_ - x.cur_;
        }
    };

public:
    transform_view() requires default_initializable<V> && default_initializable<F> = default;

    constexpr explicit transform_view(V base, F fun)
        : base_(std::move(base)), fun_(std::move(fun)) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr iterator<false> begin() { return iterator<false>{*this, ranges::begin(base_)}; }

    constexpr iterator<true> begin() const
        requires range<V const> && regular_invocable<F const&, range_reference_t<V const>> {
        return iterator<true>{*this, ranges::begin(base_)};
    }

    constexpr sentinel<false> end() { return sentinel<false>{ranges::end(base_)}; }

    constexpr iterator<false> end() requires common_range<V> {
        return iterator<false>{*this, ranges::end(base_)};
    }

    constexpr sentinel<true> end() const
        requires range<V const> && regular_invocable<F const&, range_reference_t<V const>> {
        return sentinel<true>{ranges::end(base_)};
    }

    constexpr iterator<true> end() const
        requires common_range<V const> && regular_invocable<F const&, range_reference_t<V const>> {
        return iterator<true>{*this, ranges::end(base_)};
    }

    constexpr auto size() requires sized_range<V> { return ranges::size(base_); }

    constexpr auto size() const requires sized_range<V const> { return ranges::size(base_); }
};

// Deduction Guide
template<class R, class F> transform_view(R&&, F) -> transform_view<views::all_t<R>, F>;

namespace views {

inline constexpr struct __transform_fn {
    template<typename E, typename F>
    static constexpr auto operator()(E&& e, F&& f)
        requires requires { transform_view(std::forward<E>(e), std::forward<F>(f)); } {
        return transform_view(std::forward<E>(e), std::forward<F>(f));
    }

    template<typename F> static constexpr auto operator()(F&& f) {
        return __range_adaptor_closure_fn(std::bind_back<__transform_fn>(std::forward<F>(f)));
    }
} transform;

}  // namespace views

}  // namespace std::ranges
