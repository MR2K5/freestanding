#pragma once

#include <__ranges/adjacent_view.hpp>
#include <__ranges/core.hpp>
#include <__ranges/zip_transform_view.hpp>
#include <concepts>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>

namespace std::ranges {

// REPEAT helper to unpack N copies of type T into regular_invocable / invoke_result_t
template<class F, class T, size_t N, class = std::make_index_sequence<N>> struct __repeat_invoker;

template<class F, class T, size_t N, size_t... Is>
struct __repeat_invoker<F, T, N, std::index_sequence<Is...>> {
    template<size_t> using __elem           = T;
    static constexpr bool regular_invocable = std::regular_invocable<F, __elem<Is>...>;
    using invoke_result                     = std::invoke_result_t<F, __elem<Is>...>;
};

template<class F, class T, size_t N>
concept __repeat_regular_invocable = __repeat_invoker<F, T, N>::regular_invocable;

template<class F, class T, size_t N>
using __repeat_invoke_result_t = typename __repeat_invoker<F, T, N>::invoke_result;

// Helper to determine iterator_category based on [range.adjacent.transform.iterator] p1
template<class Base, class F, size_t N> struct __adjacent_transform_iter_category {
private:
    static consteval auto __determine_cat() {
        using Ret = __repeat_invoke_result_t<F&, range_reference_t<Base>, N>;
        if constexpr (!std::is_reference_v<Ret>) {
            return input_iterator_tag{};
        } else {
            using C = typename std::iterator_traits<iterator_t<Base>>::iterator_category;
            if constexpr (derived_from<C, random_access_iterator_tag>) {
                return random_access_iterator_tag{};
            } else if constexpr (derived_from<C, bidirectional_iterator_tag>) {
                return bidirectional_iterator_tag{};
            } else if constexpr (derived_from<C, forward_iterator_tag>) {
                return forward_iterator_tag{};
            } else {
                return input_iterator_tag{};
            }
        }
    }

public:
    using iterator_category = decltype(__determine_cat());
};

// =============================================================================
// 25.7.28.2 Class template adjacent_transform_view [range.adjacent.transform.view]
// =============================================================================

template<forward_range V, move_constructible F, size_t N>
requires view<V> && (N > 0)
      && is_object_v<F> && __repeat_regular_invocable<F&, range_reference_t<V>, N>
      && __detail::__referenceable<__repeat_invoke_result_t<F&, range_reference_t<V>, N>>
class adjacent_transform_view: public view_interface<adjacent_transform_view<V, F, N>> {
    __detail::movable_box<F> fun_;
    adjacent_view<V, N> inner_;

    using InnerView = adjacent_view<V, N>;

    template<bool Const> using inner_iterator = iterator_t<__maybe_const<Const, InnerView>>;

    template<bool Const> using inner_sentinel = sentinel_t<__maybe_const<Const, InnerView>>;

    template<bool Const> class iterator;
    template<bool Const> class sentinel;

public:
    adjacent_transform_view() = default;

    constexpr explicit adjacent_transform_view(V base, F fun)
        : fun_(std::move(fun)), inner_(std::move(base)) {}

    constexpr V base() const& requires copy_constructible<V> { return inner_.base(); }
    constexpr V base() && { return std::move(inner_).base(); }

    constexpr auto begin() { return iterator<false>(*this, inner_.begin()); }

    constexpr auto begin() const
        requires range<InnerView const>
              && __repeat_regular_invocable<F const&, range_reference_t<V const>, N> {
        return iterator<true>(*this, inner_.begin());
    }

    constexpr auto end() {
        if constexpr (common_range<InnerView>) {
            return iterator<false>(*this, inner_.end());
        } else {
            return sentinel<false>(inner_.end());
        }
    }

    constexpr auto end() const
        requires range<InnerView const>
              && __repeat_regular_invocable<F const&, range_reference_t<V const>, N> {
        if constexpr (common_range<InnerView const>) {
            return iterator<true>(*this, inner_.end());
        } else {
            return sentinel<true>(inner_.end());
        }
    }

    constexpr auto size() requires sized_range<InnerView> { return inner_.size(); }

    constexpr auto size() const requires sized_range<InnerView const> { return inner_.size(); }

    constexpr auto reserve_hint() requires approximately_sized_range<InnerView> {
        return inner_.reserve_hint();
    }

    constexpr auto reserve_hint() const requires approximately_sized_range<InnerView const> {
        return inner_.reserve_hint();
    }
};

// =============================================================================
// 25.7.28.3 Class template adjacent_transform_view::iterator [range.adjacent.transform.iterator]
// =============================================================================

template<forward_range V, move_constructible F, size_t N>
requires view<V> && (N > 0)
      && is_object_v<F> && __repeat_regular_invocable<F&, range_reference_t<V>, N>
      && __detail::__referenceable<__repeat_invoke_result_t<F&, range_reference_t<V>, N>>
template<bool Const>
class adjacent_transform_view<V, F, N>::iterator {
    using Parent = __maybe_const<Const, adjacent_transform_view>;
    using Base   = __maybe_const<Const, V>;

    Parent* parent_ = nullptr;
    inner_iterator<Const> inner_;

    constexpr iterator(Parent& parent, inner_iterator<Const> inner)
        : parent_(std::addressof(parent)), inner_(std::move(inner)) {}

    template<size_t... Is> static consteval bool __noexcept_deref(std::index_sequence<Is...>) {
        return noexcept(std::invoke(
            *std::declval<Parent*>()->fun_,
            *std::get<Is>(std::declval<inner_iterator<Const>&>().current_)...
        ));
    }

    friend class adjacent_transform_view;
    template<bool> friend class iterator;
    template<bool> friend class sentinel;

public:
    using iterator_category = typename __adjacent_transform_iter_category<
        Base, __maybe_const<Const, F>, N>::iterator_category;
    using iterator_concept = typename inner_iterator<Const>::iterator_concept;
    using value_type       = remove_cvref_t<
        __repeat_invoke_result_t<__maybe_const<Const, F>&, range_reference_t<Base>, N>>;
    using difference_type = range_difference_t<Base>;

    iterator() = default;

    constexpr iterator(iterator<!Const> i)
        requires Const && convertible_to<inner_iterator<false>, inner_iterator<Const>>
        : parent_(i.parent_), inner_(std::move(i.inner_)) {}

    constexpr decltype(auto)
    operator*() const noexcept(__noexcept_deref(std::make_index_sequence<N>{})) {
        return std::apply(
            [&](auto const&... iters) -> decltype(auto) {
                return std::invoke(*parent_->fun_, *iters...);
            },
            inner_.current_
        );
    }

    constexpr iterator& operator++() {
        ++inner_;
        return *this;
    }

    constexpr iterator operator++(int) {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr iterator& operator--() requires bidirectional_range<Base> {
        --inner_;
        return *this;
    }

    constexpr iterator operator--(int) requires bidirectional_range<Base> {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    constexpr iterator& operator+=(difference_type x) requires random_access_range<Base> {
        inner_ += x;
        return *this;
    }

    constexpr iterator& operator-=(difference_type x) requires random_access_range<Base> {
        inner_ -= x;
        return *this;
    }

    constexpr decltype(auto) operator[](difference_type n) const requires random_access_range<Base>
    {
        return std::apply(
            [&](auto const&... iters) -> decltype(auto) {
                return std::invoke(*parent_->fun_, iters[n]...);
            },
            inner_.current_
        );
    }

    friend constexpr bool operator==(iterator const& x, iterator const& y) {
        return x.inner_ == y.inner_;
    }

    friend constexpr bool operator<(iterator const& x, iterator const& y)
        requires random_access_range<Base> {
        return x.inner_ < y.inner_;
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
        requires random_access_range<Base> && three_way_comparable<inner_iterator<Const>> {
        return x.inner_ <=> y.inner_;
    }

    friend constexpr iterator operator+(iterator const& i, difference_type n)
        requires random_access_range<Base> {
        return iterator(*i.parent_, i.inner_ + n);
    }

    friend constexpr iterator operator+(difference_type n, iterator const& i)
        requires random_access_range<Base> {
        return iterator(*i.parent_, i.inner_ + n);
    }

    friend constexpr iterator operator-(iterator const& i, difference_type n)
        requires random_access_range<Base> {
        return iterator(*i.parent_, i.inner_ - n);
    }

    friend constexpr difference_type operator-(iterator const& x, iterator const& y)
        requires sized_sentinel_for<inner_iterator<Const>, inner_iterator<Const>> {
        return x.inner_ - y.inner_;
    }
};

// =============================================================================
// 25.7.28.4 Class template adjacent_transform_view::sentinel [range.adjacent.transform.sentinel]
// =============================================================================

template<forward_range V, move_constructible F, size_t N>
requires view<V> && (N > 0)
      && is_object_v<F> && __repeat_regular_invocable<F&, range_reference_t<V>, N>
      && __detail::__referenceable<__repeat_invoke_result_t<F&, range_reference_t<V>, N>>
template<bool Const>
class adjacent_transform_view<V, F, N>::sentinel {
    inner_sentinel<Const> inner_;

    constexpr explicit sentinel(inner_sentinel<Const> inner): inner_(std::move(inner)) {}

    friend class adjacent_transform_view;
    template<bool> friend class sentinel;

public:
    sentinel() = default;

    constexpr sentinel(sentinel<!Const> i)
        requires Const && convertible_to<inner_sentinel<false>, inner_sentinel<Const>>
        : inner_(std::move(i.inner_)) {}

    template<bool OtherConst>
    requires sentinel_for<inner_sentinel<Const>, inner_iterator<OtherConst>>
    friend constexpr bool operator==(iterator<OtherConst> const& x, sentinel const& y) {
        return x.inner_ == y.inner_;
    }

    template<bool OtherConst>
    requires sized_sentinel_for<inner_sentinel<Const>, inner_iterator<OtherConst>>
    friend constexpr range_difference_t<__maybe_const<OtherConst, InnerView>>
    operator-(iterator<OtherConst> const& x, sentinel const& y) {
        return x.inner_ - y.inner_;
    }

    template<bool OtherConst>
    requires sized_sentinel_for<inner_sentinel<Const>, inner_iterator<OtherConst>>
    friend constexpr range_difference_t<__maybe_const<OtherConst, InnerView>>
    operator-(sentinel const& x, iterator<OtherConst> const& y) {
        return x.inner_ - y.inner_;
    }
};

// =============================================================================
// 25.7.28.1 Overview [range.adjacent.transform.overview] - Range Adaptor Object
// =============================================================================

namespace views {

template<size_t N, class F>
struct __adjacent_transform_closure: range_adaptor_closure<__adjacent_transform_closure<N, F>> {
    __detail::movable_box<F> fun_;

    constexpr explicit __adjacent_transform_closure(F f): fun_(std::move(f)) {}

    template<forward_range R> constexpr auto operator()(R&& r) const& {
        if constexpr (N == 0) {
            return ((void)r, views::zip_transform(*fun_));
        } else {
            return adjacent_transform_view<views::all_t<R>, F, N>(std::forward<R>(r), *fun_);
        }
    }

    template<forward_range R> constexpr auto operator()(R&& r) && {
        if constexpr (N == 0) {
            return ((void)r, views::zip_transform(std::move(*fun_)));
        } else {
            return adjacent_transform_view<views::all_t<R>, F, N>(
                std::forward<R>(r), std::move(*fun_)
            );
        }
    }
};

template<size_t N> struct __adjacent_transform_fn {
    static constexpr auto operator()(auto&& E, auto&& F) _STD_RETURN_REQ(
        (N == 0 && forward_range<decltype(E)>), zip_transform(FWD(F))
    );
    static constexpr auto operator()(auto&& E, auto&& F) _STD_RETURN(
        adjacent_transform_view<all_t<decltype(E)>, decltype(F), N>(FWD(E), FWD(F))
    );

    static constexpr auto operator()(auto&& F) noexcept(is_nothrow_move_constructible_v<decltype(F)>) {
        return __range_adaptor_closure_fn(bind_back<__adjacent_transform_fn>(FWD(F)));
    }
};

template<size_t N> inline constexpr __adjacent_transform_fn<N> adjacent_transform{};

}  // namespace views

}  // namespace std::ranges
