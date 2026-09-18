#pragma once

#include <__memory/base.hpp>
#include <cstddef>
#include <limits>
#include <new>
#include <utility>

namespace std {

// [allocator.traits.other], class template allocation_result (C++23)
template<class Pointer, class SizeType = size_t> struct allocation_result {
    Pointer ptr;
    SizeType count;

    friend bool operator==(allocation_result const&, allocation_result const&) = default;
};

namespace __alloc_detail {

// 1. pointer detection
template<class Alloc, class Val> struct __pointer_type {
    using type = Val*;
};

template<class Alloc, class Val> requires requires { typename Alloc::pointer; }
struct __pointer_type<Alloc, Val> {
    using type = typename Alloc::pointer;
};

// 2. const_pointer detection
template<class Alloc, class Ptr, class Val> struct __const_pointer_type {
    using type = typename pointer_traits<Ptr>::template rebind<Val const>;
};

template<class Alloc, class Ptr, class Val> requires requires { typename Alloc::const_pointer; }
struct __const_pointer_type<Alloc, Ptr, Val> {
    using type = typename Alloc::const_pointer;
};

// 3. void_pointer detection
template<class Alloc, class Ptr> struct __void_pointer_type {
    using type = typename pointer_traits<Ptr>::template rebind<void>;
};

template<class Alloc, class Ptr> requires requires { typename Alloc::void_pointer; }
struct __void_pointer_type<Alloc, Ptr> {
    using type = typename Alloc::void_pointer;
};

// 4. const_void_pointer detection
template<class Alloc, class Ptr> struct __const_void_pointer_type {
    using type = typename pointer_traits<Ptr>::template rebind<void const>;
};

template<class Alloc, class Ptr> requires requires { typename Alloc::const_void_pointer; }
struct __const_void_pointer_type<Alloc, Ptr> {
    using type = typename Alloc::const_void_pointer;
};

// 5. difference_type detection
template<class Alloc, class Ptr> struct __diff_type {
    using type = typename pointer_traits<Ptr>::difference_type;
};

template<class Alloc, class Ptr> requires requires { typename Alloc::difference_type; }
struct __diff_type<Alloc, Ptr> {
    using type = typename Alloc::difference_type;
};

// 6. size_type detection
template<class Alloc, class Diff> struct __size_type {
    using type = make_unsigned_t<Diff>;
};

template<class Alloc, class Diff> requires requires { typename Alloc::size_type; }
struct __size_type<Alloc, Diff> {
    using type = typename Alloc::size_type;
};

// 7. propagate_on_container_copy_assignment detection
template<class Alloc> struct __pocca_type {
    using type = false_type;
};

template<class Alloc> requires requires { typename Alloc::propagate_on_container_copy_assignment; }
struct __pocca_type<Alloc> {
    using type = typename Alloc::propagate_on_container_copy_assignment;
};

// 8. propagate_on_container_move_assignment detection
template<class Alloc> struct __pocma_type {
    using type = false_type;
};

template<class Alloc> requires requires { typename Alloc::propagate_on_container_move_assignment; }
struct __pocma_type<Alloc> {
    using type = typename Alloc::propagate_on_container_move_assignment;
};

// 9. propagate_on_container_swap detection
template<class Alloc> struct __pocs_type {
    using type = false_type;
};

template<class Alloc> requires requires { typename Alloc::propagate_on_container_swap; }
struct __pocs_type<Alloc> {
    using type = typename Alloc::propagate_on_container_swap;
};

// 10. is_always_equal detection
template<class Alloc> struct __is_always_equal_type {
    using type = typename is_empty<Alloc>::type;
};

template<class Alloc> requires requires { typename Alloc::is_always_equal; }
struct __is_always_equal_type<Alloc> {
    using type = typename Alloc::is_always_equal;
};

// 11. rebind_alloc detection
template<class Alloc, class T>
concept __has_rebind_other = requires { typename Alloc::template rebind<T>::other; };

template<class Alloc, class T> struct __alloc_rebind {};

template<class Alloc, class T> requires __has_rebind_other<Alloc, T>
struct __alloc_rebind<Alloc, T> {
    using type = typename Alloc::template rebind<T>::other;
};

template<template<class, class...> class AllocTemplate, class U, class... Args, class T>
requires(!__has_rebind_other<AllocTemplate<U, Args...>, T>)
struct __alloc_rebind<AllocTemplate<U, Args...>, T> {
    using type = AllocTemplate<T, Args...>;
};

}  // namespace __alloc_detail

// [allocator.traits], class template allocator_traits
template<class Alloc> struct allocator_traits {
    using allocator_type = Alloc;
    using value_type     = typename Alloc::value_type;

    using pointer = typename __alloc_detail::__pointer_type<Alloc, value_type>::type;
    using const_pointer =
        typename __alloc_detail::__const_pointer_type<Alloc, pointer, value_type>::type;
    using void_pointer = typename __alloc_detail::__void_pointer_type<Alloc, pointer>::type;
    using const_void_pointer =
        typename __alloc_detail::__const_void_pointer_type<Alloc, pointer>::type;

    using difference_type = typename __alloc_detail::__diff_type<Alloc, pointer>::type;
    using size_type       = typename __alloc_detail::__size_type<Alloc, difference_type>::type;

    using propagate_on_container_copy_assignment =
        typename __alloc_detail::__pocca_type<Alloc>::type;
    using propagate_on_container_move_assignment =
        typename __alloc_detail::__pocma_type<Alloc>::type;
    using propagate_on_container_swap = typename __alloc_detail::__pocs_type<Alloc>::type;
    using is_always_equal = typename __alloc_detail::__is_always_equal_type<Alloc>::type;

    template<class T> using rebind_alloc = typename __alloc_detail::__alloc_rebind<Alloc, T>::type;

    template<class T> using rebind_traits = allocator_traits<rebind_alloc<T>>;

    // Static Member Functions

    [[nodiscard]] static constexpr pointer allocate(Alloc& a, size_type n) { return a.allocate(n); }

    [[nodiscard]] static constexpr pointer
    allocate(Alloc& a, size_type n, const_void_pointer hint) {
        if constexpr (requires { a.allocate(n, hint); }) {
            return a.allocate(n, hint);
        } else {
            return a.allocate(n);
        }
    }

    [[nodiscard]] static constexpr allocation_result<pointer, size_type>
    allocate_at_least(Alloc& a, size_type n) {
        if constexpr (requires { a.allocate_at_least(n); }) {
            return a.allocate_at_least(n);
        } else {
            return {a.allocate(n), n};
        }
    }

    static constexpr void deallocate(Alloc& a, pointer p, size_type n) noexcept {
        a.deallocate(p, n);
    }

    template<class T, class... Args>
    static constexpr void construct(Alloc& a, T* p, Args&&... args) {
        if constexpr (requires { a.construct(p, FWD(args)...); }) {
            a.construct(p, FWD(args)...);
        } else {
            std::construct_at(p, FWD(args)...);
        }
    }

    template<class T> static constexpr void destroy(Alloc& a, T* p) {
        if constexpr (requires { a.destroy(p); }) {
            a.destroy(p);
        } else {
            std::destroy_at(p);
        }
    }

    static constexpr size_type max_size(Alloc const& a) noexcept {
        if constexpr (requires { a.max_size(); }) {
            return a.max_size();
        } else {
            return (numeric_limits<size_type>::max)() / sizeof(value_type);
        }
    }

    static constexpr Alloc select_on_container_copy_construction(Alloc const& rhs) {
        if constexpr (requires { rhs.select_on_container_copy_construction(); }) {
            return rhs.select_on_container_copy_construction();
        } else {
            return rhs;
        }
    }
};

namespace __detail {

template<class T> struct __non_allocating_allocator {
    using value_type                             = T;
    using propagate_on_container_move_assignment = true_type;

    [[noreturn]] constexpr T* allocate(size_t) { std::unreachable(); }
    constexpr void deallocate(T*, size_t) noexcept {}

    constexpr __non_allocating_allocator() noexcept                                  = default;
    constexpr __non_allocating_allocator(__non_allocating_allocator const&) noexcept = default;
    template<class U>
    constexpr __non_allocating_allocator(__non_allocating_allocator<U> const&) noexcept {}
};

}  // namespace __detail

template<class Alloc>
concept __simple_allocator = requires(Alloc alloc, size_t n) {
    { *alloc.allocate(n) } -> same_as<typename Alloc::value_type&>;
    { alloc.deallocate(alloc.allocate(n), n) };
} && copy_constructible<Alloc> && equality_comparable<Alloc>;

#ifdef _STD_HAS_HEAP

template<class T> class allocator {
public:
    using value_type                             = T;
    using size_type                              = size_t;
    using difference_type                        = ptrdiff_t;
    using propagate_on_container_move_assignment = true_type;

    constexpr allocator() noexcept                 = default;
    constexpr allocator(allocator const&) noexcept = default;
    template<class U> constexpr allocator(allocator<U> const&) noexcept {}
    constexpr ~allocator()                           = default;
    constexpr allocator& operator=(allocator const&) = default;

    constexpr T* allocate(size_t n) {
        if (numeric_limits<size_t>::max() / sizeof(T) < n) _THROW(std::bad_alloc());
        if constexpr (alignof(T) <= alignof(std::max_align_t))
            return __builtin_operator_new(n);
        else
            return __builtin_operator_new(n, std::align_val_t(alignof(T)));
    }
    constexpr allocation_result<T*> allocate_at_least(size_t n) { return allocate(n); }
    constexpr void deallocate(T* p, size_t n) {
        if constexpr (alignof(T) <= alignof(std::max_align_t))
            __builtin_operator_delete(p, n);
        else
            __builtin_operator_delete(p, n, std::align_val_t(alignof(T)));
    }
};

template<class T, class U>
constexpr bool operator==(allocator<T> const&, allocator<U> const&) noexcept {
    return true;
};

#else

template<class T> class allocator {
public:
    static_assert(false, "Allocator is not enabled when the heap is not in use");
};

#endif

}  // namespace std
