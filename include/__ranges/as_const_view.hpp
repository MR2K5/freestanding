#pragma once

#include <__functional/mem_ptr_traits.hpp>
#include <__ranges/core.hpp>
#include <optional>
#include <span>
#include <type_traits>

namespace std::ranges {
template<view V> requires input_range<V>
class as_const_view: public view_interface<as_const_view<V>> {
    V base_ = V();  // exposition only

public:
    as_const_view() requires default_initializable<V> = default;
    constexpr explicit as_const_view(V base): base_(std::move(base)) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin() requires(!__detail::__simple_view<V>) { return ranges::cbegin(base_); }
    constexpr auto begin() const requires range<V const> { return ranges::cbegin(base_); }

    constexpr auto end() requires(!__detail::__simple_view<V>) { return ranges::cend(base_); }
    constexpr auto end() const requires range<V const> { return ranges::cend(base_); }

    constexpr auto size() requires sized_range<V> { return ranges::size(base_); }
    constexpr auto size() const requires sized_range<V const> { return ranges::size(base_); }

    constexpr auto reserve_hint() requires approximately_sized_range<V> {
        return ranges::reserve_hint(base_);
    }
    constexpr auto reserve_hint() const requires approximately_sized_range<V const> {
        return ranges::reserve_hint(base_);
    }
};

template<class T> constexpr bool enable_borrowed_range<as_const_view<T>> = enable_borrowed_range<T>;

template<class R> as_const_view(R&&) -> as_const_view<views::all_t<R>>;

namespace views {

inline constexpr struct __as_const_fn {
    template<class T> requires constant_range<all_t<T>>
    static constexpr auto __impl(T&& e, __detail::__priority_tag<6>) _STD_RETURN(all(FWD(e)));

    template<class T> requires __is_specialization_of_v<empty_view, remove_cvref_t<T>>
    static constexpr auto __impl(T&&, __detail::__priority_tag<5>) _STD_RETURN(
        auto(empty<typename remove_cvref_t<T>::__value_type>)
    );

    template<class T> struct __opt_ref {};
    template<class T> struct __opt_ref<optional<T&>> {
        using type = T;
    };

    template<class T> requires requires { typename __opt_ref<remove_cvref_t<T>>::type; }
    static constexpr auto __impl(T&& e, __detail::__priority_tag<4>) _STD_RETURN(
        optional<typename __opt_ref<remove_cvref_t<T>>::type>(FWD(e))
    );

    template<class T> requires __is_span_v<remove_cvref_t<T>>
    static constexpr auto __impl(T&& e, __detail::__priority_tag<3>) _STD_RETURN(
        span<typename remove_cvref_t<T>::element_type const, remove_cvref_t<T>::extent>(FWD(e))
    );

    template<class T> struct __ref_view {};
    template<class T> struct __ref_view<ref_view<T>> {
        using type = T;
    };

    template<class T, class U = remove_cvref_t<T>> requires requires {
        typename __ref_view<U>::type;
        requires constant_range<typename __ref_view<U>::type const>;
    }
    static constexpr auto __impl(T&& e, __detail::__priority_tag<2>) _STD_RETURN(
        ref_view(static_cast<typename __ref_view<U>::type const&>(FWD(e).base()))
    );

    template<class T, class U = remove_cvref_t<T>>
    requires is_lvalue_reference_v<T> && constant_range<U const> && (!view<U>)static constexpr auto
    __impl(T&& e, __detail::__priority_tag<1>) _STD_RETURN(ref_view(static_cast<U const&>(FWD(e))));

    template<class T>
    static constexpr auto
    __impl(T&& e, __detail::__priority_tag<0>) _STD_RETURN(as_const_view(FWD(e)));

} as_const;

}  // namespace views

}  // namespace std::ranges
