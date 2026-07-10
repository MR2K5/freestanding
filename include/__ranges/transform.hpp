#pragma once
// code: language=c++
// IWYU pragma: private: include <ranges>

#include "core.hpp"

#include <optional>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace std::ranges {

template<input_range V, move_constructible F>
requires view<V> && is_object_v<F> && regular_invocable<F&, range_reference_t<V>> && requires {
    requires __detail::__referenceable<invoke_result_t<F&, range_reference_t<V>>>;
} class transform_view: public view_interface<transform_view<V, F>> {
    V base_;
    __detail::movable_box<F> fun_;

    template<bool Const> struct _iter_base {};
    template<bool Const> requires forward_range<__maybe_const<Const, V>> struct _iter_base<Const> {
        using iterator_category = decltype([] {
            using MFC  = __maybe_const<Const, F>;
            using Base = __maybe_const<Const, V>;
            if constexpr (!is_reference_v<invoke_result_t<MFC&, range_reference_t<Base>>>) {
                return input_iterator_tag();
            } else {
                using C = iterator_traits<iterator_t<Base>>::iterator_category;
                if constexpr (is_same_v<C, contiguous_iterator_tag>)
                    return random_access_iterator_tag();
                else
                    return C();
            }
        }());
    };

public:
    template<bool Const> struct iterator: _iter_base<Const> {
        using Parent = __maybe_const<Const, transform_view>;
        using Base   = __maybe_const<Const, V>;

        using iterator_concept = conditional_t<
            random_access_iterator<Base>, random_access_iterator_tag,
            conditional_t<
                bidirectional_range<Base>, bidirectional_iterator_tag,
                conditional_t<forward_range<Base>, forward_iterator_tag, input_iterator_tag>>>;

        using value_type =
            remove_cvref_t<invoke_result_t<__maybe_const<Const, F>, range_reference_t<Base>>>;
        using difference_type = range_difference_t<Base>;

        iterator() requires default_initializable<iterator_t<Base>> = default;
        constexpr iterator(Parent& p, iterator_t<Base> i): iter_(std::move(i)), parent_(&p) {}
        constexpr iterator(iterator<!Const> i)
            requires Const && convertible_to<iterator_t<V>, iterator_t<Base>>
            : iter_(std::move(i.iter_)), parent_(i.parent_) {}

        constexpr iterator_t<Base> const& base() const& noexcept { return iter_; }
        constexpr iterator_t<Base> base() && { return std::move(iter_); }
        constexpr decltype(auto) operator*() const { return std::invoke(*parent_->fun_, *iter_); }
        constexpr decltype(auto) operator[](difference_type n) const
            requires random_access_range<Base> {
            return std::invoke(*parent_->fun_, iter_[n]);
        }

        constexpr iterator& operator++() {
            ++iter_;
            return *this;
        }
        constexpr void operator++(int) { ++iter_; }
        constexpr iterator operator++(int) requires forward_range<Base> {
            auto tmp = *this;
            ++*this;
            return tmp;
        }
        constexpr iterator& operator--() requires borrowed_range<Base> {
            --iter_;
            return *this;
        }
        constexpr iterator operator--(int) requires bidirectional_range<Base> {
            auto tmp(*this);
            --*this;
            return tmp;
        }
        constexpr iterator& operator+=(difference_type n) requires random_access_range<Base> {
            iter_ += n;
            return *this;
        }
        constexpr iterator& operator-=(difference_type n) requires random_access_range<Base> {
            iter_ -= n;
            return *this;
        }

        friend constexpr bool operator==(iterator const& x, iterator const& y)
            requires equality_comparable<iterator_t<Base>> {
            return x.iter_ == y.iter_;
        }
        friend constexpr auto operator<=>(iterator const& x, iterator const& y)
            requires random_access_range<Base> && three_way_comparable<iterator_t<Base>> {
            return x.iter_ <=> y.iter_;
        }

        friend constexpr iterator operator+(iterator const& x, difference_type n)
            requires random_access_range<Base> {
            return iterator(*x.parent_, x.iter_ + n);
        }
        friend constexpr iterator operator+(difference_type n, iterator const& x)
            requires random_access_range<Base> {
            return iterator(*x.parent_, x.iter_ + n);
        }
        friend constexpr iterator operator-(iterator const& x, difference_type n)
            requires random_access_range<Base> {
            return iterator(*x.parent_, x.iter_ - n);
        }
        friend constexpr difference_type operator-(iterator const& x, iterator const& y)
            requires sized_sentinel_for<iterator_t<Base>, iterator_t<Base>> {
            return x.iter_ - y.iter_;
        }

        friend constexpr decltype(auto) iter_move(iterator const& x)
            noexcept(noexcept(std::invoke(*x.parent_->fun_, *x.iter_))) {
            if constexpr (is_lvalue_reference_v<decltype(*x)>) {
                return std::move(*x);
            } else {
                return *x;
            }
        }

    private:
        iterator_t<Base> iter_ = {};
        Parent* parent_        = {};
    };

    template<bool Const> struct sentinel {
        using Base   = __maybe_const<Const, V>;
        using Parent = __maybe_const<Const, transform_view>;

        sentinel() = default;
        constexpr explicit sentinel(sentinel_t<Base> end): end_(std::move(end)) {}
        constexpr sentinel(sentinel<!Const> const& x)
            requires Const && convertible_to<sentinel_t<V>, sentinel_t<Base>>
            : end_(std::move(x.end_)) {}

        constexpr sentinel_t<Base> base() const { return end_; }
        friend constexpr bool operator==(iterator<Const> const& x, sentinel const& y) {
            return x.base() == y.end_;
        }
        friend constexpr range_difference_t<Base>
        operator-(iterator<Const> const& x, sentinel const& y)
            requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>> {
            return x.base() - y.end_;
        }
        friend constexpr range_difference_t<Base>
        operator-(sentinel const& x, iterator<Const> const& y)
            requires sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>> {
            return x.end_ - y.base();
        }

    private:
        sentinel_t<Base> end_;
    };

    constexpr transform_view() requires default_initializable<V> && default_initializable<F>
    = default;
    constexpr explicit transform_view(V b, F f)
        : base_(std::move(b)), fun_(in_place, std::move(f)) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr iterator<false> begin() { return iterator<false>(*this, ranges::begin(base_)); }
    constexpr iterator<true> begin() const
        requires regular_invocable<F const&, range_reference_t<V const>> {
        return iterator<true>(*this, ranges::begin(base_));
    }

    constexpr sentinel<false> end() { return sentinel<false>(ranges::end(base_)); }
    constexpr iterator<false> end() requires common_range<V> {
        return iterator<false>(*this, ranges::end(base_));
    }
    constexpr sentinel<true> end() const
        requires regular_invocable<F const&, range_reference_t<V const>> {
        return sentinel<true>(ranges::end(base_));
    }
    constexpr iterator<true> end() const
        requires regular_invocable<F const&, range_reference_t<V const>> && common_range<V> {
        return iterator<true>(*this, ranges::end(base_));
    }
};

template<class R, class F> transform_view(R&& r, F) -> transform_view<views::all_t<R>, F>;

namespace views {
inline constexpr struct __transform: __range_adaptor {
    static constexpr size_t __rac_argc = 2;

    template<viewable_range R, class F>
    static constexpr view auto operator()(R&& r, F&& f)
        requires requires { transform_view(std::forward<R>(r), std::forward<F>(f)); } {
        return transform_view(std::forward<R>(r), std::forward<F>(f));
    }
    using __range_adaptor::operator();

} transform;
}  // namespace views

namespace __detail {

inline constexpr auto __unwrap =
    views::transform([]<class T>(reference_wrapper<T> x) -> auto& { return x.get(); });

}  // namespace __detail

}  // namespace std::ranges
