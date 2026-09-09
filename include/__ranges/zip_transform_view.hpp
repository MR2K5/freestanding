#pragma once

#include <__ranges/core.hpp>
#include <__ranges/zip_view.hpp>
#include <concepts>
#include <optional>
#include <tuple>
#include <type_traits>

namespace std::ranges {

// Helper to conditionally define iterator_category based on [range.zip.transform.iterator] p1
template<class Base, class F, class... Views> struct __zip_transform_iter_category {};

template<class Base, class F, class... Views> requires forward_range<Base>
struct __zip_transform_iter_category<Base, F, Views...> {
private:
    static consteval auto __determine_cat() {
        using Ret = invoke_result_t<F&, range_reference_t<Views>...>;
        if constexpr (!is_reference_v<Ret>) {
            return input_iterator_tag{};
        } else if constexpr (
            (derived_from<
                 typename iterator_traits<iterator_t<Views>>::iterator_category,
                 random_access_iterator_tag>
             && ...)
        ) {
            return random_access_iterator_tag{};
        } else if constexpr (
            (derived_from<
                 typename iterator_traits<iterator_t<Views>>::iterator_category,
                 bidirectional_iterator_tag>
             && ...)
        ) {
            return bidirectional_iterator_tag{};
        } else if constexpr (
            (derived_from<
                 typename iterator_traits<iterator_t<Views>>::iterator_category,
                 forward_iterator_tag>
             && ...)
        ) {
            return forward_iterator_tag{};
        } else {
            return input_iterator_tag{};
        }
    }

public:
    using iterator_category = decltype(__determine_cat());
};

// =============================================================================
// 25.7.26.2 Class template zip_transform_view [range.zip.transform.view]
// =============================================================================

template<move_constructible F, input_range... Views>
requires(view<Views> && ...) && (sizeof...(Views) > 0)
     && is_object_v<F> && regular_invocable<F&, range_reference_t<Views>...>
     && __detail::__referenceable<invoke_result_t<F&, range_reference_t<Views>...>>
class zip_transform_view: public view_interface<zip_transform_view<F, Views...>> {
    __detail::movable_box<F> fun_;
    zip_view<Views...> zip_;

    using InnerView = zip_view<Views...>;

    template<bool Const> using ziperator = iterator_t<__maybe_const<Const, InnerView>>;

    template<bool Const> using zentinel = sentinel_t<__maybe_const<Const, InnerView>>;

    template<bool Const> class iterator;
    template<bool Const> class sentinel;

public:
    zip_transform_view() = default;

    constexpr explicit zip_transform_view(F fun, Views... views)
        : fun_(std::move(fun)), zip_(std::move(views)...) {}

    constexpr auto begin() { return iterator<false>(*this, zip_.begin()); }

    constexpr auto begin() const
        requires range<InnerView const>
              && regular_invocable<F const&, range_reference_t<Views const>...> {
        return iterator<true>(*this, zip_.begin());
    }

    constexpr auto end() {
        if constexpr (common_range<InnerView>) {
            return iterator<false>(*this, zip_.end());
        } else {
            return sentinel<false>(zip_.end());
        }
    }

    constexpr auto end() const
        requires range<InnerView const>
              && regular_invocable<F const&, range_reference_t<Views const>...> {
        if constexpr (common_range<InnerView const>) {
            return iterator<true>(*this, zip_.end());
        } else {
            return sentinel<true>(zip_.end());
        }
    }

    constexpr auto size() requires sized_range<InnerView> { return zip_.size(); }

    constexpr auto size() const requires sized_range<InnerView const> { return zip_.size(); }
};

// Deduction guide
template<class F, class... Rs>
zip_transform_view(F, Rs&&...) -> zip_transform_view<F, views::all_t<Rs>...>;

// =============================================================================
// 25.7.26.3 Class template zip_transform_view::iterator [range.zip.transform.iterator]
// =============================================================================

template<move_constructible F, input_range... Views>
requires(view<Views> && ...) && (sizeof...(Views) > 0)
     && is_object_v<F> && regular_invocable<F&, range_reference_t<Views>...>
     && __detail::__referenceable<invoke_result_t<F&, range_reference_t<Views>...>>
template<bool Const>
class zip_transform_view<F, Views...>::iterator
    : public __zip_transform_iter_category<
          __maybe_const<Const, InnerView>, __maybe_const<Const, F>,
          __maybe_const<Const, Views>...> {

    using Parent = __maybe_const<Const, zip_transform_view>;
    using Base   = __maybe_const<Const, InnerView>;

    Parent* parent_ = nullptr;
    ziperator<Const> inner_;

    constexpr iterator(Parent& parent, ziperator<Const> inner)
        : parent_(std::addressof(parent)), inner_(std::move(inner)) {}

    template<size_t... Is> static consteval bool _noexcept_deref(index_sequence<Is...>) {
        return noexcept(std::invoke(
            *declval<Parent*>()->fun_, *std::get<Is>(declval<ziperator<Const>&>().current_)...
        ));
    }

    friend class zip_transform_view;
    template<bool> friend class iterator;
    template<bool> friend class sentinel;

public:
    using iterator_concept = typename ziperator<Const>::iterator_concept;
    using value_type       = remove_cvref_t<invoke_result_t<
        __maybe_const<Const, F>&, range_reference_t<__maybe_const<Const, Views>>...>>;
    using difference_type  = range_difference_t<Base>;

    iterator() = default;

    constexpr iterator(iterator<!Const> i)
        requires Const && convertible_to<ziperator<false>, ziperator<Const>>
        : parent_(i.parent_), inner_(std::move(i.inner_)) {}

    constexpr decltype(auto)
    operator*() const noexcept(_noexcept_deref(std::index_sequence_for<Views...>{})) {
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

    constexpr void operator++(int) { ++*this; }

    constexpr iterator operator++(int) requires forward_range<Base> {
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
            [&]<class... Is>(Is const&... iters) -> decltype(auto) {
                return std::invoke(*parent_->fun_, iters[iter_difference_t<Is>(n)]...);
            },
            inner_.current_
        );
    }

    friend constexpr bool operator==(iterator const& x, iterator const& y)
        requires equality_comparable<ziperator<Const>> {
        return x.inner_ == y.inner_;
    }

    friend constexpr auto operator<=>(iterator const& x, iterator const& y)
        requires random_access_range<Base> {
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
        requires sized_sentinel_for<ziperator<Const>, ziperator<Const>> {
        return x.inner_ - y.inner_;
    }
};

// =============================================================================
// 25.7.26.4 Class template zip_transform_view::sentinel [range.zip.transform.sentinel]
// =============================================================================

template<move_constructible F, input_range... Views>
requires(view<Views> && ...) && (sizeof...(Views) > 0)
     && is_object_v<F> && regular_invocable<F&, range_reference_t<Views>...>
     && __detail::__referenceable<invoke_result_t<F&, range_reference_t<Views>...>>
template<bool Const>
class zip_transform_view<F, Views...>::sentinel {
    zentinel<Const> inner_;

    constexpr explicit sentinel(zentinel<Const> inner): inner_(std::move(inner)) {}

    friend class zip_transform_view;
    template<bool> friend class sentinel;

public:
    sentinel() = default;

    constexpr sentinel(sentinel<!Const> i)
        requires Const && convertible_to<zentinel<false>, zentinel<Const>>
        : inner_(std::move(i.inner_)) {}

    template<bool OtherConst> requires sentinel_for<zentinel<Const>, ziperator<OtherConst>>
    friend constexpr bool operator==(iterator<OtherConst> const& x, sentinel const& y) {
        return x.inner_ == y.inner_;
    }

    template<bool OtherConst> requires sized_sentinel_for<zentinel<Const>, ziperator<OtherConst>>
    friend constexpr range_difference_t<__maybe_const<OtherConst, InnerView>>
    operator-(iterator<OtherConst> const& x, sentinel const& y) {
        return x.inner_ - y.inner_;
    }

    template<bool OtherConst> requires sized_sentinel_for<zentinel<Const>, ziperator<OtherConst>>
    friend constexpr range_difference_t<__maybe_const<OtherConst, InnerView>>
    operator-(sentinel const& x, iterator<OtherConst> const& y) {
        return x.inner_ - y.inner_;
    }
};

namespace views {

// =============================================================================
// 25.7.26.1 Overview [range.zip.transform.overview] - Customization Point Object
// =============================================================================

struct __zip_transform_fn {
    template<class F, class FD = decay_t<F>> requires move_constructible<FD>
                                                   && regular_invocable<FD&>
                                                   && is_object_v<decay_t<invoke_result_t<FD&>>>
    static constexpr auto operator()(F&&) noexcept {
        return empty<decay_t<invoke_result_t<FD&>>>;
    }

    template<class F, class... Rs>
    static constexpr auto
    operator()(F&& f, Rs&&... rs) _STD_RETURN(zip_transform_view(FWD(f), FWD(rs)...));
};

inline constexpr __zip_transform_fn zip_transform{};

}  // namespace views
}  // namespace std::ranges
