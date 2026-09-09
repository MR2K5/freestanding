#pragma once

#include <__iterator/concepts.hpp>
#include <__ranges/core.hpp>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace std::ranges {

template<size_t I = 0, size_t N, class F> constexpr void __dispatch_index(size_t idx, F&& f) {
    if constexpr (I < N) {
        if (idx == I) {
            f.template operator()<I>();
            return;
        }
        __dispatch_index<I + 1, N>(idx, std::forward<F>(f));
    }
}

template<size_t I, class... Ts> using __type_at = Ts...[I];

// Concat type traits
template<class... Rs> using __concat_reference_t = common_reference_t<range_reference_t<Rs>...>;

template<class... Rs> using __concat_value_t = common_type_t<range_value_t<Rs>...>;

template<class... Rs>
using __concat_rvalue_reference_t = common_reference_t<range_rvalue_reference_t<Rs>...>;

template<class Ref, class RRef, class It>
concept __concat_indirectly_readable_impl = requires(It const it) {
    { *it } -> convertible_to<Ref>;
    { ranges::iter_move(it) } -> convertible_to<RRef>;
};

template<class... Rs>
concept __concat_indirectly_readable =
    common_reference_with<__concat_reference_t<Rs...>&&, __concat_value_t<Rs...>&>
    && common_reference_with<__concat_reference_t<Rs...>&&, __concat_rvalue_reference_t<Rs...>&&>
    && common_reference_with<__concat_rvalue_reference_t<Rs...>&&, __concat_value_t<Rs...> const&>
    && (__concat_indirectly_readable_impl<
            __concat_reference_t<Rs...>, __concat_rvalue_reference_t<Rs...>, iterator_t<Rs>>
        && ...);

template<class... Rs>
concept __concatable = requires {
    typename __concat_reference_t<Rs...>;
    typename __concat_value_t<Rs...>;
    typename __concat_rvalue_reference_t<Rs...>;
} && __concat_indirectly_readable<Rs...>;

template<bool Const, class... Rs> struct __concat_is_random_access_helper {
    static constexpr bool value = []<size_t... Is>(std::index_sequence<Is...>) {
        return (random_access_range<__maybe_const<Const, Rs>> && ...)
            && (common_range<__maybe_const<Const, __type_at<Is, Rs...>>> && ...);
    }(std::make_index_sequence<sizeof...(Rs) - 1>{});
};

template<bool Const, class... Rs>
concept concat_is_random_access = __concat_is_random_access_helper<Const, Rs...>::value;

template<bool Const, class... Rs> struct __concat_is_bidirectional_helper {
    static constexpr bool value = []<size_t... Is>(std::index_sequence<Is...>) {
        return (bidirectional_range<__maybe_const<Const, Rs>> && ...)
            && (common_range<__maybe_const<Const, __type_at<Is, Rs...>>> && ...);
    }(std::make_index_sequence<sizeof...(Rs) - 1>{});
};

template<bool Const, class... Rs>
concept concat_is_bidirectional = __concat_is_bidirectional_helper<Const, Rs...>::value;

template<bool Const, class... Views> struct __concat_sized_sentinel_helper {
    static constexpr bool value = []<size_t... Is>(std::index_sequence<Is...>) {
        return (sized_sentinel_for<
                    sentinel_t<__maybe_const<Const, Views>>,
                    iterator_t<__maybe_const<Const, Views>>>
                && ...)
            && (sized_range<__maybe_const<Const, __type_at<Is + 1, Views...>>> && ...);
    }(std::make_index_sequence<sizeof...(Views) - 1>{});
};

template<bool Const, class... Views>
concept __concat_sized_sentinel_clause = __concat_sized_sentinel_helper<Const, Views...>::value;

// Helper to determine iterator_category
template<bool Const, class... Views> struct __concat_view_iter_cat {};

template<bool Const, class... Views> requires __detail::__all_forward<Const, Views...>
struct __concat_view_iter_cat<Const, Views...> {
private:
    static consteval auto __cat() {
        if constexpr (!is_reference_v<__concat_reference_t<__maybe_const<Const, Views>...>>) {
            return input_iterator_tag{};
        } else {
            using Cs = std::tuple<typename std::iterator_traits<
                iterator_t<__maybe_const<Const, Views>>>::iterator_category...>;

            auto all_derived_from = []<class Tag>() {
                return []<size_t... Is>(std::index_sequence<Is...>) {
                    return (derived_from<std::tuple_element_t<Is, Cs>, Tag> && ...);
                }(std::make_index_sequence<sizeof...(Views)>{});
            };

            if constexpr (
                all_derived_from.template operator()<random_access_iterator_tag>()
                && concat_is_random_access<Const, Views...>
            ) {
                return random_access_iterator_tag{};
            } else if constexpr (
                all_derived_from.template operator()<bidirectional_iterator_tag>()
                && concat_is_bidirectional<Const, Views...>
            ) {
                return bidirectional_iterator_tag{};
            } else if constexpr (all_derived_from.template operator()<forward_iterator_tag>()) {
                return forward_iterator_tag{};
            } else {
                return input_iterator_tag{};
            }
        }
    }

public:
    using iterator_category = decltype(__cat());
};

// =============================================================================
// 25.7.18.2 Class template concat_view [range.concat.view]
// =============================================================================

template<input_range... Views>
requires(view<Views> && ...) && (sizeof...(Views) > 0) && __concatable<Views...>
class concat_view: public view_interface<concat_view<Views...>> {
    std::tuple<Views...> views_;

    template<bool> class iterator;

    template<bool Const> friend class iterator;

public:
    constexpr concat_view() = default;

    constexpr explicit concat_view(Views... views): views_(std::move(views)...) {}

    constexpr iterator<false> begin() requires(!(__detail::__simple_view<Views> && ...)) {
        iterator<false> it(this, in_place_index<0>, ranges::begin(std::get<0>(views_)));
        it.template satisfy<0>();
        return it;
    }

    constexpr iterator<true> begin() const
        requires(range<Views const> && ...) && __concatable<Views const...> {
        iterator<true> it(this, in_place_index<0>, ranges::begin(std::get<0>(views_)));
        it.template satisfy<0>();
        return it;
    }

    constexpr auto end() requires(!(__detail::__simple_view<Views> && ...)) {
        constexpr auto N = sizeof...(Views);
        if constexpr (
            __detail::__all_forward<false, Views...>
            && common_range<std::tuple_element_t<N - 1, std::tuple<Views...>>>
        ) {
            return iterator<false>(
                this, in_place_index<N - 1>, ranges::end(std::get<N - 1>(views_))
            );
        } else {
            return default_sentinel;
        }
    }

    constexpr auto end() const requires(range<Views const> && ...) && __concatable<Views const...> {
        constexpr auto N = sizeof...(Views);
        if constexpr (
            __detail::__all_forward<true, Views...>
            && common_range<std::tuple_element_t<N - 1, std::tuple<Views...>> const>
        ) {
            return iterator<true>(
                this, in_place_index<N - 1>, ranges::end(std::get<N - 1>(views_))
            );
        } else {
            return default_sentinel;
        }
    }

    constexpr auto size() requires(sized_range<Views> && ...) {
        return std::apply(
            [](auto... sizes) {
                using CT = __detail::__make_unsigned_like<common_type_t<decltype(sizes)...>>;
                return (CT(sizes) + ...);
            },
            __detail::__tuple_transform(ranges::size, views_)
        );
    }

    constexpr auto size() const requires(sized_range<Views const> && ...) {
        return std::apply(
            [](auto... sizes) {
                using CT = __detail::__make_unsigned_like<common_type_t<decltype(sizes)...>>;
                return (CT(sizes) + ...);
            },
            __detail::__tuple_transform(ranges::size, views_)
        );
    }

    constexpr auto reserve_hint() requires(approximately_sized_range<Views> && ...) {
        return std::apply(
            [](auto... sizes) {
                using CT = __detail::__make_unsigned_like<common_type_t<decltype(sizes)...>>;
                return (CT(sizes) + ...);
            },
            __detail::__tuple_transform(ranges::reserve_hint, views_)
        );
    }

    constexpr auto reserve_hint() const requires(approximately_sized_range<Views const> && ...) {
        return std::apply(
            [](auto... sizes) {
                using CT = __detail::__make_unsigned_like<common_type_t<decltype(sizes)...>>;
                return (CT(sizes) + ...);
            },
            __detail::__tuple_transform(ranges::reserve_hint, views_)
        );
    }
};

template<class... R> concat_view(R&&...) -> concat_view<views::all_t<R>...>;

// =============================================================================
// 25.7.18.3 Class concat_view::iterator [range.concat.iterator]
// =============================================================================

template<input_range... Views> requires(view<Views> && ...) && (sizeof...(Views) > 0)
                                    && __concatable<Views...> template<bool Const>
class concat_view<Views...>::iterator: public __concat_view_iter_cat<Const, Views...> {
public:
    using iterator_concept = conditional_t<
        concat_is_random_access<Const, Views...>, random_access_iterator_tag,
        conditional_t<
            concat_is_bidirectional<Const, Views...>, bidirectional_iterator_tag,
            conditional_t<
                __detail::__all_forward<Const, Views...>, forward_iterator_tag,
                input_iterator_tag>>>;

    using value_type      = __concat_value_t<__maybe_const<Const, Views>...>;
    using difference_type = common_type_t<range_difference_t<__maybe_const<Const, Views>>...>;

private:
    using base_iter = std::variant<iterator_t<__maybe_const<Const, Views>>...>;

    __maybe_const<Const, concat_view>* parent_ = nullptr;
    base_iter it_;

    template<size_t N> constexpr void satisfy() {
        if constexpr (N < (sizeof...(Views) - 1)) {
            if (std::get<N>(it_) == ranges::end(std::get<N>(parent_->views_))) {
                it_.template emplace<N + 1>(ranges::begin(std::get<N + 1>(parent_->views_)));
                satisfy<N + 1>();
            }
        }
    }

    template<size_t N> constexpr void prev() {
        if constexpr (N == 0) {
            --std::get<0>(it_);
        } else {
            if (std::get<N>(it_) == ranges::begin(std::get<N>(parent_->views_))) {
                it_.template emplace<N - 1>(ranges::end(std::get<N - 1>(parent_->views_)));
                prev<N - 1>();
            } else {
                --std::get<N>(it_);
            }
        }
    }

    template<size_t N> constexpr void advance_fwd(difference_type offset, difference_type steps) {
        using underlying_diff_type = iter_difference_t<std::variant_alternative_t<N, base_iter>>;
        if constexpr (N == sizeof...(Views) - 1) {
            std::get<N>(it_) += static_cast<underlying_diff_type>(steps);
        } else {
            auto n_size = ranges::distance(std::get<N>(parent_->views_));
            if (offset + steps < n_size) {
                std::get<N>(it_) += static_cast<underlying_diff_type>(steps);
            } else {
                it_.template emplace<N + 1>(ranges::begin(std::get<N + 1>(parent_->views_)));
                advance_fwd<N + 1>(0, offset + steps - n_size);
            }
        }
    }

    template<size_t N> constexpr void advance_bwd(difference_type offset, difference_type steps) {
        using underlying_diff_type = iter_difference_t<std::variant_alternative_t<N, base_iter>>;
        if constexpr (N == 0) {
            std::get<N>(it_) -= static_cast<underlying_diff_type>(steps);
        } else {
            if (offset >= steps) {
                std::get<N>(it_) -= static_cast<underlying_diff_type>(steps);
            } else {
                auto prev_size = ranges::distance(std::get<N - 1>(parent_->views_));
                it_.template emplace<N - 1>(ranges::end(std::get<N - 1>(parent_->views_)));
                advance_bwd<N - 1>(prev_size, steps - offset);
            }
        }
    }

    template<class... Args>
    constexpr explicit iterator(__maybe_const<Const, concat_view>* parent, Args&&... args)
        requires constructible_from<base_iter, Args&&...>
        : parent_(parent), it_(std::forward<Args>(args)...) {}

    template<size_t I = 0> static constexpr base_iter __convert_variant(auto&& var) {
        if constexpr (I < sizeof...(Views)) {
            if (var.index() == I) {
                return base_iter(in_place_index<I>, std::get<I>(std::move(var)));
            }
            return __convert_variant<I + 1>(std::move(var));
        }
        return base_iter();
    }

    friend class concat_view;
    template<bool> friend class iterator;

public:
    iterator() = default;

    constexpr iterator(iterator<!Const> it)
        requires Const && (convertible_to<iterator_t<Views>, iterator_t<Views const>> && ...)
        : parent_(it.parent_), it_(__convert_variant(std::move(it.it_))) {}

    constexpr decltype(auto) operator*() const {
        using reference = __concat_reference_t<__maybe_const<Const, Views>...>;
        return std::visit([](auto&& it) -> reference { return *it; }, it_);
    }

    constexpr iterator& operator++() {
        __dispatch_index<0, sizeof...(Views)>(it_.index(), [this]<size_t I>() {
            ++std::get<I>(it_);
            satisfy<I>();
        });
        return *this;
    }

    constexpr void operator++(int) { ++*this; }

    constexpr iterator operator++(int) requires __detail::__all_forward<Const, Views...> {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr iterator& operator--() requires concat_is_bidirectional<Const, Views...> {
        __dispatch_index<0, sizeof...(Views)>(it_.index(), [this]<size_t I>() { prev<I>(); });
        return *this;
    }

    constexpr iterator operator--(int) requires concat_is_bidirectional<Const, Views...> {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    constexpr iterator& operator+=(difference_type n)
        requires concat_is_random_access<Const, Views...> {
        __dispatch_index<0, sizeof...(Views)>(it_.index(), [this, n]<size_t I>() {
            if (n > 0) {
                advance_fwd<I>(std::get<I>(it_) - ranges::begin(std::get<I>(parent_->views_)), n);
            } else if (n < 0) {
                advance_bwd<I>(std::get<I>(it_) - ranges::begin(std::get<I>(parent_->views_)), -n);
            }
        });
        return *this;
    }

    constexpr iterator& operator-=(difference_type n)
        requires concat_is_random_access<Const, Views...> {
        *this += -n;
        return *this;
    }

    constexpr decltype(auto) operator[](difference_type n) const
        requires concat_is_random_access<Const, Views...> {
        return *((*this) + n);
    }

    friend constexpr bool operator==(iterator const& x, iterator const& y)
        requires(equality_comparable<iterator_t<__maybe_const<Const, Views>>> && ...) {
        return x.it_ == y.it_;
    }

    friend constexpr bool operator==(iterator const& it, default_sentinel_t) {
        constexpr auto last_idx = sizeof...(Views) - 1;
        return it.it_.index() == last_idx
            && std::get<last_idx>(it.it_) == ranges::end(std::get<last_idx>(it.parent_->views_));
    }

    friend constexpr bool operator<(iterator const& x, iterator const& y)
        requires __detail::__all_random_access<Const, Views...> {
        return x.it_ < y.it_;
    }

    friend constexpr bool operator>(iterator const& x, iterator const& y)
        requires __detail::__all_random_access<Const, Views...> {
        return x.it_ > y.it_;
    }

    friend constexpr bool operator<=(iterator const& x, iterator const& y)
        requires __detail::__all_random_access<Const, Views...> {
        return x.it_ <= y.it_;
    }

    friend constexpr bool operator>=(iterator const& x, iterator const& y)
        requires __detail::__all_random_access<Const, Views...> {
        return x.it_ >= y.it_;
    }

    friend constexpr auto operator<=>(iterator const& x, iterator const& y) requires(
        __detail::__all_random_access<Const, Views...>
        && (three_way_comparable<iterator_t<__maybe_const<Const, Views>>> && ...)
    ) {
        return x.it_ <=> y.it_;
    }

    friend constexpr iterator operator+(iterator const& it, difference_type n)
        requires concat_is_random_access<Const, Views...> {
        auto temp  = it;
        temp      += n;
        return temp;
    }

    friend constexpr iterator operator+(difference_type n, iterator const& it)
        requires concat_is_random_access<Const, Views...> {
        return it + n;
    }

    friend constexpr iterator operator-(iterator const& it, difference_type n)
        requires concat_is_random_access<Const, Views...> {
        auto temp  = it;
        temp      -= n;
        return temp;
    }

    friend constexpr difference_type operator-(iterator const& x, iterator const& y)
        requires concat_is_random_access<Const, Views...> {
        auto const ix = x.it_.index();
        auto const iy = y.it_.index();
        if (ix > iy) {
            difference_type dy = 0;
            __dispatch_index<0, sizeof...(Views)>(iy, [&]<size_t Iy>() {
                dy = ranges::distance(
                    std::get<Iy>(y.it_), ranges::end(std::get<Iy>(y.parent_->views_))
                );
            });

            difference_type dx = 0;
            __dispatch_index<0, sizeof...(Views)>(ix, [&]<size_t Ix>() {
                dx = ranges::distance(
                    ranges::begin(std::get<Ix>(x.parent_->views_)), std::get<Ix>(x.it_)
                );
            });

            difference_type s = 0;
            auto sum_ranges   = [&]<size_t... Is>(std::index_sequence<Is...>) {
                ((void)((Is > iy && Is < ix)
                            ? (s += ranges::distance(std::get<Is>(x.parent_->views_)))
                            : 0),
                 ...);
            };
            sum_ranges(std::make_index_sequence<sizeof...(Views)>{});

            return dy + s + dx;
        } else if (ix < iy) {
            return -(y - x);
        } else {
            difference_type diff = 0;
            __dispatch_index<0, sizeof...(Views)>(ix, [&]<size_t I>() {
                diff = std::get<I>(x.it_) - std::get<I>(y.it_);
            });
            return diff;
        }
    }

    friend constexpr difference_type operator-(iterator const& x, default_sentinel_t)
        requires __concat_sized_sentinel_clause<Const, Views...> {
        auto const ix      = x.it_.index();
        difference_type dx = 0;
        __dispatch_index<0, sizeof...(Views)>(ix, [&]<size_t Ix>() {
            dx =
                ranges::distance(std::get<Ix>(x.it_), ranges::end(std::get<Ix>(x.parent_->views_)));
        });

        difference_type s = 0;
        auto sum_ranges   = [&]<size_t... Is>(std::index_sequence<Is...>) {
            ((void)((Is > ix) ? (s += ranges::size(std::get<Is>(x.parent_->views_))) : 0), ...);
        };
        sum_ranges(std::make_index_sequence<sizeof...(Views)>{});

        return -(dx + s);
    }

    friend constexpr difference_type operator-(default_sentinel_t, iterator const& x)
        requires __concat_sized_sentinel_clause<Const, Views...> {
        return -(x - default_sentinel);
    }

    friend constexpr decltype(auto) iter_move(iterator const& it) noexcept(
        ((is_nothrow_invocable_v<
              decltype(ranges::iter_move), iterator_t<__maybe_const<Const, Views>> const&>
          && is_nothrow_convertible_v<
              range_rvalue_reference_t<__maybe_const<Const, Views>>,
              __concat_rvalue_reference_t<__maybe_const<Const, Views>...>>)
         && ...)
    ) {
        return std::visit(
            [](auto const& i) -> __concat_rvalue_reference_t<__maybe_const<Const, Views>...> {
                return ranges::iter_move(i);
            },
            it.it_
        );
    }

    friend constexpr void iter_swap(iterator const& x, iterator const& y) noexcept(
        noexcept(ranges::swap(*x, *y))
        && (noexcept(ranges::iter_swap(
                std::declval<iterator_t<__maybe_const<Const, Views>> const&>(),
                std::declval<iterator_t<__maybe_const<Const, Views>> const&>()
            ))
            && ...)
    ) requires swappable_with<iter_reference_t<iterator>, iter_reference_t<iterator>>
            && (... && indirectly_swappable<iterator_t<__maybe_const<Const, Views>>>) {
        std::visit(
            [&](auto const& it1, auto const& it2) {
                if constexpr (is_same_v<decltype(it1), decltype(it2)>) {
                    ranges::iter_swap(it1, it2);
                } else {
                    ranges::swap(*x, *y);
                }
            },
            x.it_, y.it_
        );
    }
};

// =============================================================================
// 25.7.18.1 Overview [range.concat.overview] - Customization Point Object
// =============================================================================

namespace views {

struct __concat_fn {
    static constexpr auto operator()(auto&&... Es) _STD_RETURN_REQ(
        (sizeof...(Es) == 1 && input_range<decltype(Es...[0])>), all(FWD(Es)...)
    );
    static constexpr auto operator()(auto&&... Es) _STD_RETURN(concat_view(FWD(Es)...));
};

inline constexpr __concat_fn concat{};

}  // namespace views

}  // namespace std::ranges
