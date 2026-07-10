#pragma once

#include <cstddef>
#include <cstdlib>
#include <new>
#include <type_traits>

namespace std {

namespace detail {
template<class T> struct ptr_traits_elem {};

template<class T> requires requires { typename T::element_type; } struct ptr_traits_elem<T> {
    using type = T::element_type;
};

template<template<class...> class SomePointer, class T, class... Args>
requires(!requires { typename SomePointer<T, Args...>::element_type; })
struct ptr_traits_elem<SomePointer<T, Args...>> {
    using type = T;
};

template<class Ptr> concept has_elem_type = requires { typename ptr_traits_elem<Ptr>::type; };
}  // namespace detail

template<class Ptr> struct pointer_traits {};
template<class Ptr> requires detail::has_elem_type<Ptr> struct pointer_traits<Ptr> {
    using pointer         = Ptr;
    using element_type    = detail::ptr_traits_elem<Ptr>::type;
    using difference_type = decltype([] {
        if constexpr (requires { typename Ptr::difference_type; }) {
            return type_identity<typename Ptr::difference_type>{};
        } else {
            return type_identity<ptrdiff_t>{};
        }
    }())::type;

    template<class P2, class U> struct __rebind;

    template<class P2, class U> requires requires { typename Ptr::template rebind<U>; }
    struct __rebind<P2, U> {
        using type = Ptr::template rebind<U>;
    };

    template<template<class, class...> class Tmpl, class T, class... As, class U>
    requires(!requires { typename Ptr::template rebind<U>; }) struct __rebind<Tmpl<T, As...>, U> {
        using type = Tmpl<U, As...>;
    };

    template<class U> using rebind = __rebind<Ptr, U>::type;

    using __type = decltype([] -> decltype(auto) {
        if constexpr (is_void_v<element_type>) {
            return __empty();
        } else {
            return declval<element_type&>();
        }
    }());

    static constexpr pointer pointer_to(__type r) {
        static_assert(requires { Ptr::pointer_to(r); });
        return Ptr::pointer_to(r);
    }
};

template<class T> struct pointer_traits<T*> {
    using pointer                  = T*;
    using element_type             = T;
    using difference_type          = ptrdiff_t;
    template<class U> using rebind = U*;

    using __type = decltype([] -> decltype(auto) {
        if constexpr (is_void_v<element_type>) {
            return __empty();
        } else {
            return declval<element_type&>();
        }
    }());

    static constexpr pointer pointer_to(__type r) { return __builtin_addressof(r); }
};

template<class T> constexpr T* to_address(T* p) noexcept {
    static_assert(!is_function_v<T>);
    return p;
}
template<class Ptr> constexpr auto to_address(Ptr p) noexcept {
    if constexpr (requires { pointer_traits<Ptr>::to_address(p); }) {
        return pointer_traits<Ptr>::to_address(p);
    } else {
        return to_address(p.operator->());
    }
}

template<class T> constexpr T* addressof(T& arg) noexcept {
    return __builtin_addressof(arg);
}

template<class T> void addressof(T const&&) = delete;

template<class T>
constexpr T* construct_at(T* loc, auto&&... as)
    requires(!is_unbounded_array_v<T>) && requires(void* p) { ::new (p) T(FWD(as)...); } {
    static_assert(!is_array_v<T> || sizeof...(as) == 0);
    if constexpr (is_array_v<T>) { return ::new (static_cast<void*>(loc)) T(FWD(as)...); }
}

template<class T> constexpr void destroy_at(T* loc) noexcept {
    if constexpr (is_array_v<T>) {
        for (auto&& val: *loc) std::destroy_at(std::addressof(val));
    } else {
        loc->~T();
    }
}

template<class T>
using __pointer_of_t = decltype([] {
    if constexpr (requires { typename T::pointer; }) {
        return type_identity<typename T::pointer>{};
    } else if constexpr (requires { typename T::element_type; }) {
        return type_identity<typename T::element_type*>{};
    } else {
        return type_identity<typename pointer_traits<T>::element_type*>{};
    }
}())::type;

template<class T, class U>
using __pointer_of_or_t = decltype([] {
    if constexpr (requires { typename __pointer_of_t<T>; }) {
        return type_identity<__pointer_of_t<T>>{};
    } else {
        return type_identity<U>{};
    }
}())::type;

template<class To, class From>
requires(sizeof(To) == sizeof(From)) && is_trivially_copyable_v<To> && is_trivially_copyable_v<From>
[[nodiscard]] constexpr To bit_cast(From const& from) noexcept {
    return __builtin_bit_cast(To, from);
}

}  // namespace std
