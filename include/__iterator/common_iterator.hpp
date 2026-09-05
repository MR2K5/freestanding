#pragma once

#include <__iterator/concepts.hpp>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>

namespace std {

template<input_or_output_iterator I, sentinel_for<I> S> requires(!same_as<I, S> && copyable<I>)
class common_iterator {
public:
    constexpr common_iterator() requires default_initializable<I> = default;
    constexpr common_iterator(I i): var_(std::in_place_type<I>, std::move(i)) {}
    constexpr common_iterator(S s): var_(std::in_place_type<S>, std::move(s)) {}
    template<class I2, class S2>
    requires convertible_to<const I2&, I> && convertible_to<const S2&, S>
    constexpr common_iterator(common_iterator<I2, S2> const& x)
        : var_(x.var_.visit([](auto& x) {
              return variant<I, S>(
                  in_place_index<__index_of_type_v<remove_cvref_t<decltype(x)>, I2, S2>>, x
              );
          })) {}

    template<class I2, class S2>
    requires convertible_to<const I2&, I> && convertible_to<const S2&, S>
          && assignable_from<I&, const I2&> && assignable_from<S&, const S2&>
    constexpr common_iterator& operator=(common_iterator<I2, S2> const& x) {
        __detail::__indexed_visit([&]<size_t N1, size_t N2>(index_sequence<N1, N2>) {
            if constexpr (N1 == N2) {
                *std::get_if<N1>(&var_) = *std::get_if<N2>(&x.var_);
            } else {
                var_.emplace<N2>(*std::get_if<N2>(&x.var_));
            }
        });
        return *this;
    }

    constexpr decltype(auto) operator*() { return **std::get_if<I>(&var_); }
    constexpr decltype(auto) operator*() const requires __detail::dereferenceable<I const> {
        return **std::get_if<I>(&var_);
    }
    constexpr auto operator->() const
        requires indirectly_readable<I const>
              && (requires(I const& i) { i.operator->(); } || is_reference_v<iter_reference_t<I>>
                  || constructible_from<iter_value_t<I>, iter_reference_t<I>>) {
        if constexpr (is_pointer_v<I> || requires(I& i) { i.operator->(); }) {
            return *std::get_if<I>(&var_);
        } else if constexpr (is_reference_v<iter_reference_t<I>>) {
            auto&& tmp = **std::get_if<I>(&var_);
            return std::addressof(tmp);
        } else {
            class proxy {
                iter_value_t<I> keep_;
                constexpr proxy(iter_reference_t<I>&& x): keep_(std::move(x)) {}

            public:
                constexpr iter_value_t<I> const* operator->() const noexcept {
                    return std::addressof(keep_);
                }
            };
            return proxy(**std::get_if<I>(&var_));
        }
    }

    constexpr common_iterator& operator++() {
        ++*std::get_if<I>(&var_);
        return *this;
    }
    constexpr decltype(auto) operator++(int) {
        if constexpr (
            requires(I& i) {
                { *i++ } -> __detail::__referenceable;
            }
            || !(
                indirectly_readable<I> && constructible_from<iter_value_t<I>, iter_reference_t<I>>
                && move_constructible<iter_value_t<I>>
            )
        ) {
            return *std::get_if<I>(&var_)++;
        } else {
            class postfix_proxy {
                iter_value_t<I> keep_;
                constexpr postfix_proxy(iter_reference_t<I>&& x)
                    : keep_(std::forward<iter_reference_t<I>>(x)) {}

            public:
                constexpr iter_value_t<I> const& operator*() const noexcept { return keep_; }
            };
            postfix_proxy p(**this);
            ++*this;
            return p;
        }
    }
    common_iterator operator++(int) requires forward_iterator<I> {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    template<class I2, sentinel_for<I> S2> requires sentinel_for<S, I2>
    friend constexpr bool operator==(common_iterator const& x, common_iterator<I2, S2> const& y) {
        if (x.var_.index() == y.var_.index()) return true;
        if (x.var_.index() == 0) return *std::get_if<0>(&x.var_) == *std::get_if<1>(&y.var_);
        if (x.var_.index() == 1) return *std::get_if<1>(&x.var_) == *std::get_if<0>(&y.var_);
        std::unreachable();
    }
    template<class I2, sentinel_for<I> S2>
    requires sentinel_for<S, I2> && equality_comparable_with<I, I2>
    friend constexpr bool operator==(common_iterator const& x, common_iterator<I2, S2> const& y) {
        return __detail::__indexed_visit(
            [&]<size_t N1, size_t N2>(index_sequence<N1, N2>) {
                if constexpr (N1 == 1 && N2 == 1)
                    return true;
                else { return *std::get_if<N1>(&x.var_) == *std::get_if<N2>(&y.var_); }
            },
            x.var_, y.var_
        );
    }

    template<sized_sentinel_for<I> I2, sized_sentinel_for<I> S2> requires sized_sentinel_for<S, I2>
    friend constexpr iter_difference_t<I2>
    operator-(common_iterator const& x, common_iterator<I2, S2> const& y) {
        return __detail::__indexed_visit(
            [&]<size_t N1, size_t N2>(index_sequence<N1, N2>) {
                if constexpr (N1 == 1 && N2 == 1)
                    return iter_difference_t<I2>(0);
                else { return *std::get_if<N1>(&x.var_) - *std::get_if<N2>(&y.var_); }
            },
            x.var_, y.var_
        );
    }

    friend constexpr decltype(auto)
    iter_move(common_iterator const& i) noexcept(noexcept(ranges::iter_move(declval<I const&>())))
        requires input_iterator<I> {
        return ranges::iter_move(*std::get_if<I>(&i.var_));
    }
    template<indirectly_swappable<I> I2, class S2>
    friend constexpr void iter_swap(
        common_iterator const& x, common_iterator<I2, S2> const& y
    ) noexcept(noexcept(ranges::iter_swap(declval<I const&>(), declval<const I2&>()))) {
        ranges::iter_swap(*std::get_if<I>(&x.var_), *std::get_if<I2>(&y.var_));
    }

private:
    variant<I, S> var_;

    template<class, class> friend class common_iterator;
};

template<class I, class S> struct incrementable_traits<common_iterator<I, S>> {
    using difference_type = iter_difference_t<I>;
};

template<class I> struct __common_it_iter_traits {};

template<class I> requires integral<iter_difference_t<I>> struct __common_it_iter_traits<I> {
    using iterator_category = conditional_t<
        derived_from<typename iterator_traits<I>::iterator_category, forward_iterator_tag>,
        forward_iterator_tag, input_iterator_tag>;
};

template<input_iterator I, class S>
struct iterator_traits<common_iterator<I, S>>: __common_it_iter_traits<I> {
    using iterator_concept =
        conditional_t<forward_iterator<I>, forward_iterator_tag, input_iterator_tag>;
    using value_type      = iter_value_t<I>;
    using difference_type = iter_difference_t<I>;
    using pointer         = decltype([] {
        if constexpr (requires(common_iterator<I, S> const& a) { a.operator->(); }) {
            return type_identity<decltype(declval<common_iterator<I, S> const&>.operator->())>();
        } else {
            return type_identity<void>();
        }
    })::type;
    using reference       = iter_reference_t<I>;
};

}  // namespace std
