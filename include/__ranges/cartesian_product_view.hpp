#pragma once

#include <__ranges/core.hpp>
#include <__ranges/single.hpp>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace std::ranges {

// TODO size() and difference_type could be smaller

template<bool Const, class First, class... Vs>
concept __cartesian_product_is_random_access =
    (random_access_range<__maybe_const<Const, First>> && ...
     && (random_access_range<__maybe_const<Const, Vs>> && sized_range<__maybe_const<Const, Vs>>));

template<class R>
concept __cartesian_product_common_arg =
    common_range<R> || (sized_range<R> && random_access_range<R>);

template<bool Const, class First, class... Vs>
concept __cartesian_product_is_bidirectional =
    (bidirectional_range<__maybe_const<Const, First>> && ...
     && (bidirectional_range<__maybe_const<Const, Vs>>
         && __cartesian_product_common_arg<__maybe_const<Const, Vs>>));

template<class First, class...>
concept __cartesian_product_is_common = __cartesian_product_common_arg<First>;

template<class... Vs> concept __cartesian_product_is_sized = (sized_range<Vs> && ...);

template<bool Const, template<class> class FirstSent, class First, class... Vs>
concept __cartesian_is_sized_sentinel =
    (sized_sentinel_for<
         FirstSent<__maybe_const<Const, First>>, iterator_t<__maybe_const<Const, First>>>
     && ...
     && (sized_range<__maybe_const<Const, Vs>>
         && sized_sentinel_for<
             iterator_t<__maybe_const<Const, Vs>>, iterator_t<__maybe_const<Const, Vs>>>));

template<__cartesian_product_common_arg R> constexpr auto __cartesian_common_arg_end(R& r) {
    if constexpr (common_range<R>) {
        return ranges::end(r);
    } else {
        return ranges::begin(r) + ranges::distance(r);
    }
}

template<input_range First, forward_range... Vs> requires(view<First> && ... && view<Vs>)
class cartesian_product_view: public view_interface<cartesian_product_view<First, Vs...>> {
private:
    tuple<First, Vs...> bases_;
    template<bool Const> class iterator;

public:
    constexpr cartesian_product_view() = default;
    constexpr explicit cartesian_product_view(First first_base, Vs... bases)
        : bases_(std::move(first_base), std::move(bases)...) {}

    constexpr iterator<false> begin()
        requires(!__detail::__simple_view<First> || ... || !__detail::__simple_view<Vs>) {
        return iterator<false>(*this, __detail::__tuple_transform(ranges::begin, bases_));
    }

    constexpr iterator<true> begin() const requires(range<First const> && ... && range<Vs const>) {
        return iterator<true>(*this, __detail::__tuple_transform(ranges::begin, bases_));
    }

    constexpr iterator<false> end() requires(
        (!__detail::__simple_view<First> || ... || !__detail::__simple_view<Vs>)
        && __cartesian_product_is_common<First, Vs...>
    ) {
        return __end_impl<false>();
    }
    constexpr iterator<true> end() const
        requires __cartesian_product_is_common<First const, Vs const...> {
        return __end_impl<true>();
    }

    constexpr default_sentinel_t end() const noexcept { return {}; }

    constexpr unsigned long long size() requires __cartesian_product_is_sized<First, Vs...> {
        return std::apply(
            [](auto&... rs) { return (static_cast<unsigned long long>(ranges::size(rs)) * ...); },
            bases_
        );
    }
    constexpr unsigned long long size() const
        requires __cartesian_product_is_sized<First const, Vs const...> {
        return std::apply(
            [](auto&... rs) { return (static_cast<unsigned long long>(ranges::size(rs)) * ...); },
            bases_
        );
    }

    template<bool Const> constexpr iterator<Const> __end_impl(this auto& self) {
        auto is_empty = [&]<size_t... Is>(index_sequence<Is...>) {
            return (ranges::empty(std::get<Is + 1>(self.bases_)) || ...);
        };

        auto begin_or_first_end = [&]<size_t I>(auto& r) {
            if constexpr (I == 0) {
                return is_empty(make_index_sequence<sizeof...(Vs)>())
                         ? ranges::begin(r)
                         : __cartesian_common_arg_end(r);
            } else {
                return ranges::begin(r);
            }
        };

        auto it_tuple = [&]<size_t... Is>(index_sequence<Is...>) {
            return std::tuple{
                begin_or_first_end.template operator()<Is>(std::get<Is>(self.bases_))...
            };
        }(make_index_sequence<1 + sizeof...(Vs)>{});

        return iterator<Const>(self, std::move(it_tuple));
    }

private:
    template<bool Const> class iterator {
    public:
        using iterator_category = input_iterator_tag;
        using iterator_concept  = conditional_t<
            __cartesian_product_is_random_access<Const, First, Vs...>, random_access_iterator_tag,
            conditional_t<
                __cartesian_product_is_bidirectional<Const, First, Vs...>,
                bidirectional_iterator_tag,
                conditional_t<
                    forward_range<__maybe_const<Const, First>>, forward_iterator_tag,
                    input_iterator_tag>>>;
        using value_type = tuple<
            range_value_t<__maybe_const<Const, First>>, range_value_t<__maybe_const<Const, Vs>>...>;
        using reference = tuple<
            range_reference_t<__maybe_const<Const, First>>,
            range_reference_t<__maybe_const<Const, Vs>>...>;
        using difference_type = long long;

        iterator() = default;

        constexpr iterator(iterator<!Const> i)
            requires Const
                      && (convertible_to<iterator_t<First>, iterator_t<First const>> && ...
                          && convertible_to<iterator_t<Vs>, iterator_t<Vs const>>)
            : parent_(i.parent_), current_(std::move(i.current_)) {}

        constexpr auto operator*() const {
            return __detail::__tuple_transform(
                [&](auto& i) -> decltype(auto) { return *i; }, current_
            );
        }

        constexpr iterator& operator++() {
            next();
            return *this;
        }
        constexpr void operator++(int) { ++*this; }
        iterator operator++(int) requires forward_range<__maybe_const<Const, First>> {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        constexpr iterator& operator--()
            requires __cartesian_product_is_bidirectional<Const, First, Vs...> {
            prev();
            return *this;
        }
        iterator operator--(int) requires __cartesian_product_is_bidirectional<Const, First, Vs...>
        {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type x)
            requires __cartesian_product_is_random_access<Const, First, Vs...> {
            if (x == 0) return *this;

            auto advance_dim = [&]<size_t N>(this auto& self, difference_type offset) -> void {
                auto& current_it = std::get<N>(current_);
                auto& base       = std::get<N>(parent_->bases_);

                if constexpr (N == 0) {
                    current_it += offset;
                } else {
                    auto size = static_cast<difference_type>(ranges::size(base));
                    auto current_idx =
                        static_cast<difference_type>(current_it - ranges::begin(base));
                    auto total_idx = current_idx + offset;

                    auto div = total_idx / size;
                    auto rem = total_idx % size;

                    if (rem < 0) {
                        rem += size;
                        div -= 1;
                    }

                    current_it = ranges::begin(base) + rem;
                    if (div != 0) { self.template operator()<N - 1>(div); }
                }
            };

            advance_dim.template operator()<sizeof...(Vs)>(x);
            return *this;
        }

        constexpr iterator& operator-=(difference_type x)
            requires __cartesian_product_is_random_access<Const, First, Vs...> {
            return *this += -x;
        }

        constexpr reference operator[](difference_type n) const
            requires __cartesian_product_is_random_access<Const, First, Vs...> {
            return *((*this) + n);
        }

        friend constexpr bool operator==(iterator const& x, iterator const& y)
            requires equality_comparable<iterator_t<__maybe_const<Const, First>>> {
            return x.current_ == y.current_;
        }

        friend constexpr bool operator==(iterator const& x, default_sentinel_t) {
            auto impl = [&]<size_t... Is>(index_sequence<Is...>) {
                return (
                    (std::get<Is>(x.current_) == ranges::end(std::get<Is>(x.parent_->bases_)))
                    || ...
                );
            };
            return impl(make_index_sequence<1 + sizeof...(Vs)>());
        }

        friend constexpr auto operator<=>(iterator const& x, iterator const& y)
            requires __detail::__all_random_access<Const, First, Vs...> {
            return x.current_ <=> y.current_;
        }

        friend constexpr iterator operator+(iterator const& x, difference_type y)
            requires __cartesian_product_is_random_access<Const, First, Vs...> {
            return auto(x) += y;
        }
        friend constexpr iterator operator+(difference_type x, iterator const& y)
            requires __cartesian_product_is_random_access<Const, First, Vs...> {
            return y + x;
        }
        friend constexpr iterator operator-(iterator const& x, difference_type y)
            requires __cartesian_product_is_random_access<Const, First, Vs...> {
            return auto(x) -= y;
        }
        friend constexpr difference_type operator-(iterator const& x, iterator const& y)
            requires __cartesian_is_sized_sentinel<Const, iterator_t, First, Vs...> {
            return x.distance_from(y.current_);
        }

        friend constexpr difference_type operator-(iterator const& i, default_sentinel_t)
            requires __cartesian_is_sized_sentinel<Const, sentinel_t, First, Vs...> {
            auto impl = [&]<size_t... Is>(index_sequence<Is...>) {
                return std::tuple{
                    ranges::end(std::get<0>(i.parent_->bases_)),
                    ranges::begin(std::get<Is + 1>(i.parent_->bases_))...
                };
            };
            return i.distance_from(impl(make_index_sequence<sizeof...(Vs)>()));
        }
        friend constexpr difference_type operator-(default_sentinel_t, iterator const& i)
            requires __cartesian_is_sized_sentinel<Const, sentinel_t, First, Vs...> {
            return -(i - default_sentinel);
        }

        friend constexpr auto iter_move(iterator const& i) noexcept(
            is_nothrow_move_constructible_v<range_rvalue_reference_t<__maybe_const<Const, First>>>
            && (is_nothrow_move_constructible_v<range_rvalue_reference_t<__maybe_const<Const, Vs>>>
                && ...)
            && noexcept(ranges::iter_move(declval<iterator_t<__maybe_const<Const, First>>&>()))
            && (noexcept(ranges::iter_move(declval<iterator_t<__maybe_const<Const, Vs>>&>()))
                && ...)
        ) {
            return __detail::__tuple_transform(ranges::iter_move, i.current_);
        }

        friend constexpr void iter_swap(iterator const& l, iterator const& r) noexcept(
            noexcept(ranges::iter_swap(
                declval<iterator_t<__maybe_const<Const, First>>&>(),
                declval<iterator_t<__maybe_const<Const, First>>&>()
            ))
            && (noexcept(ranges::iter_swap(
                    declval<iterator_t<__maybe_const<Const, Vs>>&>(),
                    declval<iterator_t<__maybe_const<Const, Vs>>&>()
                ))
                && ...)
        )
            requires(
                indirectly_swappable<iterator_t<__maybe_const<Const, First>>> && ...
                && indirectly_swappable<iterator_t<__maybe_const<Const, Vs>>>
            ) {
            auto impl = [&]<size_t... Is>(index_sequence<Is...>) {
                (ranges::iter_swap(std::get<Is>(l.current_), std::get<Is>(r.current_)), ...);
            };
            return impl(make_index_sequence<1 + sizeof...(Vs)>());
        }

    private:
        using Parent    = __maybe_const<Const, cartesian_product_view>;
        Parent* parent_ = nullptr;
        tuple<iterator_t<__maybe_const<Const, First>>, iterator_t<__maybe_const<Const, Vs>>...>
            current_;

        friend cartesian_product_view;
        template<bool> friend class iterator;

        template<size_t N = sizeof...(Vs)> constexpr void next() {
            auto& it = std::get<N>(current_);
            ++it;
            if constexpr (N > 0) {
                if (it == ranges::end(std::get<N>(parent_->bases_))) {
                    it = ranges::begin(std::get<N>(parent_->bases_));
                    next<N - 1>();
                }
            }
        }

        template<size_t N = sizeof...(Vs)> constexpr void prev() {
            auto& it = std::get<N>(current_);
            if constexpr (N > 0) {
                if (it == ranges::begin(std::get<N>(parent_->bases_))) {
                    it = __cartesian_common_arg_end(std::get<N>(parent_->bases_));
                    prev<N - 1>();
                }
            }
            --it;
        }

        template<class Tuple> constexpr difference_type distance_from(Tuple const& t) const {
            auto scaled_size = [this]<size_t N>(this auto& self) -> difference_type {
                if constexpr (N <= sizeof...(Vs)) {
                    return static_cast<difference_type>(ranges::size(std::get<N>(parent_->bases_)))
                         * self.template operator()<N + 1>();
                } else {
                    return difference_type(1);
                }
            };

            auto scaled_distance = [&]<size_t N>() -> difference_type {
                return static_cast<difference_type>(std::get<N>(current_) - std::get<N>(t))
                     * scaled_size.template operator()<N + 1>();
            };

            auto scaled_sum = [&]<size_t... Ns>(index_sequence<Ns...>) -> difference_type {
                return (scaled_distance.template operator()<Ns>() + ...);
            };
            return scaled_sum(make_index_sequence<sizeof...(Vs) + 1>());
        }

        constexpr iterator(
            Parent& parent,
            tuple<iterator_t<__maybe_const<Const, First>>, iterator_t<__maybe_const<Const, Vs>>...>
                current
        )
            : parent_(&parent), current_(std::move(current)) {}
    };
};

template<class... Vs>
cartesian_product_view(Vs&&...) -> cartesian_product_view<views::all_t<Vs>...>;

namespace views {
inline constexpr struct __cartesian_product_fn {
    static constexpr auto operator()() noexcept { return views::single(tuple()); }

    static constexpr auto operator()(auto&&... Es) _STD_RETURN(
        cartesian_product_view<all_t<decltype(Es)>...>(FWD(Es)...)
    );
} cartesian_product;
}  // namespace views

}  // namespace std::ranges
