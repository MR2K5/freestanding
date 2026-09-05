#pragma once

#include <__iterator/adaptors.hpp>
#include <__ranges/core.hpp>
#include <__ranges/subrange.hpp>
#include <concepts>
#include <optional>
#include <type_traits>

namespace std::ranges {

template<view V> requires bidirectional_range<V>
class reverse_view: public view_interface<reverse_view<V>> {
    V base_ = V();
    [[no_unique_address]] conditional_t<
        !common_range<V>, __detail::non_propagating_cache<iterator_t<V>>, __empty> cache_;

public:
    reverse_view() requires default_initializable<V> = default;
    constexpr explicit reverse_view(V v): base_(std::move(v)) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr reverse_iterator<iterator_t<V>> begin() {
        if constexpr (!common_range<V>) {
            if (!cache_.has_value())
                cache_ = ranges::next(ranges::begin(base_), ranges::end(base_));
            return std::make_reverse_iterator(*cache_);
        } else {
            return std::make_reverse_iterator(
                ranges::next(ranges::begin(base_), ranges::end(base_))
            );
        }
    }
    constexpr reverse_iterator<iterator_t<V>> begin() requires common_range<V> {
        return std::make_reverse_iterator(ranges::end(base_));
    }
    constexpr auto begin() const requires common_range<V const> {
        return std::make_reverse_iterator(ranges::end(base_));
    }

    constexpr reverse_iterator<iterator_t<V>> end() {
        return std::make_reverse_iterator(ranges::begin(base_));
    }
    constexpr auto end() const requires common_range<V const> {
        return std::make_reverse_iterator(ranges::begin(base_));
    }

    constexpr auto size() requires sized_range<V> { return ranges::size(base_); }

    constexpr auto size() const requires sized_range<V const> { return ranges::size(base_); }

    constexpr auto reserve_hint() requires approximately_sized_range<V> {
        return ranges::reserve_hint(base_);
    }
    constexpr auto reserve_hint() const requires approximately_sized_range<V const> {
        return ranges::reserve_hint(base_);
    }
};

template<class T> constexpr bool enable_borrowed_range<reverse_view<T>> = enable_borrowed_range<T>;

template<class R> reverse_view(R&&) -> reverse_view<views::all_t<R>>;

namespace views {

template<class> inline constexpr bool __is_reverse_subrange = false;
template<class I, class S, subrange_kind K>
inline constexpr bool __is_reverse_subrange<subrange<I, S, K>> = true;

template<class I, class S, subrange_kind K>
constexpr auto __make_unreversed_subrange(subrange<I, S, K> const& s) {
    if constexpr (K == subrange_kind::sized) {
        return subrange<I, I, K>(s.end().base(), s.begin().base(), s.size());
    } else {
        return subrange<I, I, K>(s.end().base(), s.begin().base());
    }
}

inline constexpr struct __reverse_fn: range_adaptor_closure<__reverse_fn> {
private:
    // (2.1) Specialization of reverse_view -> E.base()
    template<class R> requires __is_specialization_of_v<reverse_view, remove_cvref_t<R>>
    static constexpr auto
    __impl(R&& r, __detail::__priority_tag<3>) noexcept(noexcept(FWD(r).base()))
        -> decltype(FWD(r).base()) {
        return FWD(r).base();
    }

    // (2.2) Specialization of optional view -> decay-copy(E)
    template<class R>
    requires(__is_specialization_of_v<optional, remove_cvref_t<R>> && view<remove_cvref_t<R>>)
    static constexpr auto
    __impl(R&& r, __detail::__priority_tag<2>) noexcept(noexcept(auto(FWD(r))))
        -> decltype(auto(FWD(r))) {
        return auto(FWD(r));
    }

    // (2.3) subrange<reverse_iterator<I>, reverse_iterator<I>, K>
    template<class R> requires __is_reverse_subrange<remove_cvref_t<R>>
    static constexpr auto __impl(R&& r, __detail::__priority_tag<1>) noexcept(
        noexcept(__make_unreversed_subrange(FWD(r)))
    ) -> decltype(__make_unreversed_subrange(FWD(r))) {
        return __make_unreversed_subrange(FWD(r));
    }

    // (2.4) Default fallback -> reverse_view{E}
    template<class R> requires requires { reverse_view{std::declval<R>()}; } static constexpr auto
    __impl(R&& r, __detail::__priority_tag<0>) noexcept(noexcept(reverse_view{FWD(r)}))
        -> decltype(reverse_view{FWD(r)}) {
        return reverse_view{FWD(r)};
    }

public:
    template<viewable_range R>
    requires requires { __impl(std::declval<R>(), __detail::__priority_tag<3>{}); } constexpr auto
    operator()(R&& r) const noexcept(noexcept(__impl(FWD(r), __detail::__priority_tag<3>{})))
        -> decltype(__impl(FWD(r), __detail::__priority_tag<3>{})) {
        return __impl(FWD(r), __detail::__priority_tag<3>{});
    }
} reverse;

}  // namespace views
}  // namespace std::ranges
