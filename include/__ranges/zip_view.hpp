#pragma once

#include <__algorithm/minmax.hpp>
#include <__iterator/concepts.hpp>
#include <__ranges/core.hpp>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

namespace std::ranges {
template<class... Rs>
concept __zip_is_common = (sizeof...(Rs) == 1 && (common_range<Rs> && ...))
                       || (!(bidirectional_range<Rs> && ...) && (common_range<Rs> && ...))
                       || ((random_access_range<Rs> && ...) && (sized_range<Rs> && ...));

template<input_range... Views> requires(view<Views> && ...) && (sizeof...(Views) > 0)
class zip_view: public view_interface<zip_view<Views...>> {
    tuple<Views...> views_;

    template<bool C> struct __iter_base {};
    template<bool C> requires __detail::__all_forward<C, Views...> struct __iter_base<C> {
        using iterator_category = input_iterator_tag;
    };

    // [range.zip.iterator], class template zip_view​::​iterator
    template<bool Const> class iterator: public __iter_base<Const> {
        using Cur = tuple<iterator_t<__maybe_const<Const, Views>>...>;
        Cur current_;

        constexpr explicit iterator(tuple<iterator_t<__maybe_const<Const, Views>>...> cur)
            : current_(std::move(cur)) {}

        friend zip_view;
        template<bool> friend class iterator;
        template<bool> friend class sentinel;

    public:
        using iterator_concept = conditional_t<
            __detail::__all_random_access<Const, Views...>, random_access_iterator_tag,
            conditional_t<
                __detail::__all_bidirectional<Const, Views...>, bidirectional_iterator_tag,
                conditional_t<
                    __detail::__all_forward<Const, Views...>, forward_iterator_tag,
                    input_iterator_tag>>>;
        using value_type      = tuple<range_value_t<__maybe_const<Const, Views>>...>;
        using difference_type = common_type_t<range_difference_t<__maybe_const<Const, Views>>...>;

        iterator() = default;
        constexpr iterator(iterator<!Const> i)
            requires Const && (convertible_to<iterator_t<Views>, iterator_t<Views const>> && ...)
            : current_(std::move(i.current_)) {}

        constexpr auto operator*() const {
            return __detail::__tuple_transform(
                [](auto& i) -> decltype(auto) { return *i; }, current_
            );
        }

        constexpr iterator& operator++() {
            __detail::__tuple_for_each([](auto& i) { ++i; }, current_);
            return *this;
        }
        constexpr void operator++(int) { ++*this; }
        iterator operator++(int) requires __detail::__all_forward<Const, Views...> {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        constexpr iterator& operator--() requires __detail::__all_bidirectional<Const, Views...> {
            __detail::__tuple_for_each([](auto& i) { --i; }, current_);
            return *this;
        }
        iterator operator--(int) requires __detail::__all_bidirectional<Const, Views...> {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type x)
            requires __detail::__all_random_access<Const, Views...> {
            __detail::__tuple_for_each(
                [&]<class I>(I& i) { i += iter_difference_t<I>(x); }, current_
            );
            return *this;
        }
        constexpr iterator& operator-=(difference_type x)
            requires __detail::__all_random_access<Const, Views...> {
            __detail::__tuple_for_each(
                [&]<class I>(I& i) { i -= iter_difference_t<I>(x); }, current_
            );
            return *this;
        }

        constexpr auto operator[](difference_type n) const
            requires __detail::__all_random_access<Const, Views...> {
            return __detail::__tuple_transform(
                [&]<class I>(I& i) -> decltype(auto) { return i[iter_difference_t<I>(n)]; },
                current_
            );
        }

        friend constexpr bool operator==(iterator const& x, iterator const& y)
            requires(equality_comparable<iterator_t<__maybe_const<Const, Views>>> && ...) {
            if constexpr (__detail::__all_bidirectional<Const, Views...>) {
                return x.current_ == y.current_;
            } else {
                bool found = false;
                [&]<size_t... Is>(index_sequence<Is...>) {
                    auto inner = [&]<size_t I> {
                        if (std::get<I>(x.current_) == std::get<I>(y.current_)) found = true;
                    };
                    (inner.template operator()<Is>(), ...);
                }(make_index_sequence<sizeof...(Views)>());
                return found;
            }
        }

        friend constexpr auto operator<=>(iterator const& x, iterator const& y)
            requires __detail::__all_random_access<Const, Views...> {
            return x.current_ <=> y.current_;
        }

        friend constexpr iterator operator+(iterator const& i, difference_type n)
            requires __detail::__all_random_access<Const, Views...> {
            return auto(i) += n;
        }
        friend constexpr iterator operator+(difference_type n, iterator const& i)
            requires __detail::__all_random_access<Const, Views...> {
            return i + n;
        }
        friend constexpr iterator operator-(iterator const& i, difference_type n)
            requires __detail::__all_random_access<Const, Views...> {
            return auto(i) -= n;
        }
        friend constexpr difference_type operator-(iterator const& x, iterator const& y) requires(
            sized_sentinel_for<
                iterator_t<__maybe_const<Const, Views>>, iterator_t<__maybe_const<Const, Views>>>
            && ...
        ) {
            auto dist = [&]<size_t I>() {
                return difference_type(std::get<I>(x.current_), std::get<I>(y.current_));
            };
            auto impl = [&]<size_t... Is>(index_sequence<Is...>) {
                return ranges::min({dist.template operator()<Is>()...});
            };
            return impl(make_index_sequence<sizeof...(Views)>());
        }

        friend constexpr auto iter_move(iterator const& i) noexcept(
            (noexcept(ranges::iter_move(declval<iterator_t<__maybe_const<Const, Views>> const&>()))
             && ...)
            && (is_nothrow_move_constructible_v<
                    range_rvalue_reference_t<__maybe_const<Const, Views>>>
                && ...)
        ) {
            return __detail::__tuple_transform(ranges::iter_move, i.current_);
        }

        friend constexpr void iter_swap(iterator const& l, iterator const& r) noexcept(
            []<size_t... Is>(index_sequence<Is...>) {
                return (
                    noexcept(ranges::iter_swap(
                        std::get<Is>(declval<Cur const&>()), std::get<Is>(declval<Cur const&>())
                    ))
                    && ...
                );
            }(make_index_sequence<sizeof...(Views)>())
        ) requires(indirectly_swappable<iterator_t<__maybe_const<Const, Views>>> && ...) {
            auto impl = [&]<size_t... Is>(index_sequence<Is...>) {
                ((ranges::iter_swap(std::get<Is>(l.current_), std::get<Is>(r.current_))), ...);
            };
            impl(make_index_sequence<sizeof...(Views)>());
        }
    };

    // [range.zip.sentinel], class template zip_view​::​sentinel
    template<bool Const> class sentinel {
        tuple<sentinel_t<__maybe_const<Const, Views>>...> end_;
        constexpr explicit sentinel(tuple<sentinel_t<__maybe_const<Const, Views>>...> end)
            : end_(std::move(end)) {}

        friend zip_view;
        template<bool> friend class sentinel;

    public:
        sentinel() = default;
        constexpr sentinel(sentinel<!Const> i)
            requires Const && (convertible_to<sentinel_t<Views>, sentinel_t<Views const>> && ...)
            : end_(std::move(i.end_)) {}

        template<bool OtherConst> requires(
            sentinel_for<
                sentinel_t<__maybe_const<Const, Views>>,
                iterator_t<__maybe_const<OtherConst, Views>>>
            && ...
        )
        friend constexpr bool operator==(iterator<OtherConst> const& x, sentinel const& y) {
            return [&]<size_t... Is>(index_sequence<Is...>) {
                return ((std::get<Is>(x.current_) == std::get<Is>(y.end_)) || ...);
            }(make_index_sequence<sizeof...(Views)>());
        }

        template<bool OtherConst> requires(
            sized_sentinel_for<
                sentinel_t<__maybe_const<Const, Views>>,
                iterator_t<__maybe_const<OtherConst, Views>>>
            && ...
        ) friend constexpr common_type_t<range_difference_t<__maybe_const<OtherConst, Views>>...>
        operator-(iterator<OtherConst> const& x, sentinel const& y) {
            auto dist = [&]<size_t I>() {
                return difference_type(std::get<I>(x.current_), std::get<I>(y.end_));
            };
            auto impl = [&]<size_t... Is>(index_sequence<Is...>) {
                return ranges::min({dist.template operator()<Is>()...});
            };
            return impl(make_index_sequence<sizeof...(Views)>());
        }

        template<bool OtherConst> requires(
            sized_sentinel_for<
                sentinel_t<__maybe_const<Const, Views>>,
                iterator_t<__maybe_const<OtherConst, Views>>>
            && ...
        ) friend constexpr common_type_t<range_difference_t<__maybe_const<OtherConst, Views>>...>
        operator-(sentinel const& y, iterator<OtherConst> const& x) {
            return -(x - y);
        }
    };

public:
    zip_view() = default;
    constexpr explicit zip_view(Views... views): views_(std::move(views)...) {}

    constexpr auto begin() requires(!(__detail::__simple_view<Views> && ...)) {
        return iterator<false>(__detail::__tuple_transform(ranges::begin, views_));
    }
    constexpr auto begin() const requires(range<Views const> && ...) {
        return iterator<true>(__detail::__tuple_transform(ranges::begin, views_));
    }

    constexpr auto end() requires(!(__detail::__simple_view<Views> && ...)) {
        if constexpr (!__zip_is_common<Views...>) {
            return sentinel<false>(__detail::__tuple_transform(ranges::end, views_));
        } else if constexpr ((random_access_range<Views> && ...)) {
            return begin() + iter_difference_t<iterator<false>>(size());
        } else {
            return iterator<false>(__detail::__tuple_transform(ranges::end, views_));
        }
    }

    constexpr auto end() const requires(range<Views const> && ...) {
        if constexpr (!__zip_is_common<Views const...>) {
            return sentinel<true>(__detail::__tuple_transform(ranges::end, views_));
        } else if constexpr ((random_access_range<Views const> && ...)) {
            return begin() + iter_difference_t<iterator<true>>(size());
        } else {
            return iterator<true>(__detail::__tuple_transform(ranges::end, views_));
        }
    }

    constexpr auto size() requires(sized_range<Views> && ...) {
        return std::apply(
            [](auto... sizes) {
                using CT = __detail::__make_unsigned_like<common_type_t<decltype(sizes)...>>;
                return ranges::min({CT(sizes)...});
            },
            __detail::__tuple_transform(ranges::size, views_)
        );
    }
    constexpr auto size() const requires(sized_range<Views const> && ...) {
        return std::apply(
            [](auto... sizes) {
                using CT = __detail::__make_unsigned_like<common_type_t<decltype(sizes)...>>;
                return ranges::min({CT(sizes)...});
            },
            __detail::__tuple_transform(ranges::size, views_)
        );
    }
};

template<class... Rs> zip_view(Rs&&...) -> zip_view<views::all_t<Rs>...>;

template<class... Views>
constexpr bool enable_borrowed_range<zip_view<Views...>> = (enable_borrowed_range<Views> && ...);

namespace views {
inline constexpr struct __zip_fn {
    static constexpr empty_view<tuple<>> operator()() noexcept { return empty<tuple<>>; }

    static constexpr auto
    operator()(auto&&... rs) _STD_RETURN(zip_view<all_t<decltype(rs)>...>(FWD(rs)...));
} zip;
}  // namespace views

}  // namespace std::ranges
