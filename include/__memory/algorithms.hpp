#pragma once

#include <__iterator/concepts.hpp>
#include <__memory/allocator.hpp>
#include <__ranges/core.hpp>
#include <__ranges/result_types.hpp>
#include <concepts>
#include <iterator>
#include <type_traits>

namespace std {

namespace __detail {

// 1. nothrow-input-iterator
template<class _Ip>
concept nothrow_input_iterator = input_iterator<_Ip> && is_lvalue_reference_v<iter_reference_t<_Ip>>
                              && same_as<remove_cvref_t<iter_reference_t<_Ip>>, iter_value_t<_Ip>>;

// 2. nothrow-sentinel-for
template<class _Sp, class _Ip> concept nothrow_sentinel_for = sentinel_for<_Sp, _Ip>;

// 3. nothrow-sized-sentinel-for
template<class _Sp, class _Ip>
concept nothrow_sized_sentinel_for = nothrow_sentinel_for<_Sp, _Ip> && sized_sentinel_for<_Sp, _Ip>;

// 4. nothrow-input-range
template<class _Rp>
concept nothrow_input_range =
    ranges::range<_Rp> && nothrow_input_iterator<ranges::iterator_t<_Rp>>
    && nothrow_sentinel_for<ranges::sentinel_t<_Rp>, ranges::iterator_t<_Rp>>;

// 5. nothrow-forward-iterator
template<class _Ip>
concept nothrow_forward_iterator =
    nothrow_input_iterator<_Ip> && forward_iterator<_Ip> && nothrow_sentinel_for<_Ip, _Ip>;

// 6. nothrow-forward-range
template<class _Rp>
concept nothrow_forward_range =
    nothrow_input_range<_Rp> && nothrow_forward_iterator<ranges::iterator_t<_Rp>>;

// 7. nothrow-bidirectional-iterator
template<class _Ip>
concept nothrow_bidirectional_iterator =
    nothrow_forward_iterator<_Ip> && bidirectional_iterator<_Ip>;

// 8. nothrow-bidirectional-range
template<class _Rp>
concept nothrow_bidirectional_range =
    nothrow_forward_range<_Rp> && nothrow_bidirectional_iterator<ranges::iterator_t<_Rp>>;

// 9. nothrow-random-access-iterator
template<class _Ip>
concept nothrow_random_access_iterator =
    nothrow_bidirectional_iterator<_Ip> && random_access_iterator<_Ip>
    && nothrow_sized_sentinel_for<_Ip, _Ip>;

// 10. nothrow-random-access-range
template<class _Rp>
concept nothrow_random_access_range =
    nothrow_bidirectional_range<_Rp> && nothrow_random_access_iterator<ranges::iterator_t<_Rp>>;

// 11. nothrow-sized-random-access-range
template<class _Rp>
concept nothrow_sized_random_access_range =
    nothrow_random_access_range<_Rp> && ranges::sized_range<_Rp>;

}  // namespace __detail

template<class T> constexpr void* __voidify(T& obj) noexcept {
    return std::addressof(obj);
}

template<class I> constexpr decltype(auto) __deref_move(I& it) {
    if constexpr (is_lvalue_reference_v<decltype(*it)>)
        return std::move(*it);
    else
        return *it;
}

namespace ranges {

inline constexpr struct __construct_at_fn {
    template<class _Tp, class... _Args>
    requires(!is_unbounded_array_v<_Tp>) && requires(void* __p, _Args&&... __args) {
        ::new (__p) _Tp(FWD(__args)...);
    } static constexpr _Tp* operator()(_Tp* __loc, _Args&&... __args) {
        if constexpr (is_array_v<_Tp>) {
            static_assert(
                sizeof...(_Args) == 0, "Cannot provide arguments when constructing array types"
            );
            return ::new (__voidify(*__loc)) _Tp[1]();
        } else {
            return ::new (__voidify(*__loc)) _Tp(FWD(__args)...);
        }
    }
} construct_at;

// =============================================================================
// 26.11.9 destroy [specialized.destroy]
// =============================================================================

inline constexpr struct __destroy_at_fn {
    template<
        destructible _Tp, __simple_allocator _Alloc = __detail::__non_allocating_allocator<_Tp>>
    static constexpr void operator()(_Tp* __loc, _Alloc&& __a = _Alloc()) noexcept {
        if constexpr (is_array_v<_Tp>) {
            for (auto& __x: *__loc) { operator()(std::addressof(__x), __a); }
        } else {
            allocator_traits<remove_cvref_t<_Alloc>>::destroy(__a, __loc);
        }
    }
} destroy_at;

inline constexpr struct __destroy_fn {
    template<
        __detail::nothrow_input_iterator _Ip, __detail::nothrow_sentinel_for<_Ip> _Sp,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<iter_value_t<_Ip>>>
    requires destructible<iter_value_t<_Ip>>
    static constexpr _Ip operator()(_Ip __first, _Sp __last, _Alloc&& __a = _Alloc()) noexcept {
        for (; __first != __last; ++__first) { destroy_at(std::addressof(*__first), __a); }
        return __first;
    }

    template<
        __detail::nothrow_input_range _Rp,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<range_value_t<_Rp>>>
    requires destructible<range_value_t<_Rp>> static constexpr borrowed_iterator_t<_Rp>
    operator()(_Rp&& __r, _Alloc&& __a = _Alloc()) noexcept {
        return operator()(ranges::begin(__r), ranges::end(__r), FWD(__a));
    }
} destroy;

inline constexpr struct __destroy_n_fn {
    template<
        __detail::nothrow_input_iterator _Ip,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<iter_value_t<_Ip>>>
    requires destructible<iter_value_t<_Ip>> static constexpr _Ip
    operator()(_Ip __first, iter_difference_t<_Ip> __n, _Alloc&& __a = _Alloc()) noexcept {
        for (; __n > 0; (void)++__first, --__n) { destroy_at(std::addressof(*__first), __a); }
        return __first;
    }
} destroy_n;

// =============================================================================
// 26.11.3 uninitialized_default_construct [uninitialized.construct.default]
// =============================================================================

inline constexpr struct __uninit_default_construct_fn {
    template<
        __detail::nothrow_forward_iterator _Ip, __detail::nothrow_sentinel_for<_Ip> _Sp,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<iter_value_t<_Ip>>>
    requires default_initializable<iter_value_t<_Ip>>
    static constexpr _Ip operator()(_Ip __first, _Sp __last, _Alloc&& __a = _Alloc()) {
        using _ValueType = remove_reference_t<iter_reference_t<_Ip>>;
        auto __rollback  = __first;
        try {
            for (; __first != __last; ++__first) { ::new (__voidify(*__first)) _ValueType; }
            return __first;
        } catch (...) {
            for (; __rollback != __first; ++__rollback) {
                destroy_at(std::addressof(*__rollback), __a);
            }
            throw;
        }
    }

    template<
        __detail::nothrow_forward_range _Rp,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<range_value_t<_Rp>>>
    requires default_initializable<range_value_t<_Rp>>
    static constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& __r, _Alloc&& __a = _Alloc()) {
        return operator()(ranges::begin(__r), ranges::end(__r), FWD(__a));
    }
} uninitialized_default_construct;

inline constexpr struct __uninit_default_construct_n_fn {
    template<
        __detail::nothrow_forward_iterator _Ip,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<iter_value_t<_Ip>>>
    requires default_initializable<iter_value_t<_Ip>> static constexpr _Ip
    operator()(_Ip __first, iter_difference_t<_Ip> __n, _Alloc&& __a = _Alloc()) {
        using _ValueType = remove_reference_t<iter_reference_t<_Ip>>;
        auto __rollback  = __first;
        try {
            for (; __n > 0; (void)++__first, --__n) { ::new (__voidify(*__first)) _ValueType; }
            return __first;
        } catch (...) {
            for (; __rollback != __first; ++__rollback) {
                destroy_at(std::addressof(*__rollback), __a);
            }
            throw;
        }
    }
} uninitialized_default_construct_n;

// =============================================================================
// 26.11.4 uninitialized_value_construct [uninitialized.construct.value]
// =============================================================================

inline constexpr struct __uninit_value_construct_fn {
    template<
        __detail::nothrow_forward_iterator _Ip, __detail::nothrow_sentinel_for<_Ip> _Sp,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<iter_value_t<_Ip>>>
    requires default_initializable<iter_value_t<_Ip>>
    static constexpr _Ip operator()(_Ip __first, _Sp __last, _Alloc&& __a = _Alloc()) {
        auto __rollback = __first;
        try {
            for (; __first != __last; ++__first) {
                allocator_traits<remove_cvref_t<_Alloc>>::construct(__a, std::addressof(*__first));
            }
            return __first;
        } catch (...) {
            for (; __rollback != __first; ++__rollback) {
                destroy_at(std::addressof(*__rollback), __a);
            }
            throw;
        }
    }

    template<
        __detail::nothrow_forward_range _Rp,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<range_value_t<_Rp>>>
    requires default_initializable<range_value_t<_Rp>>
    static constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& __r, _Alloc&& __a = _Alloc()) {
        return operator()(ranges::begin(__r), ranges::end(__r), FWD(__a));
    }
} uninitialized_value_construct;

inline constexpr struct __uninit_value_construct_n_fn {
    template<
        __detail::nothrow_forward_iterator _Ip,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<iter_value_t<_Ip>>>
    requires default_initializable<iter_value_t<_Ip>> static constexpr _Ip
    operator()(_Ip __first, iter_difference_t<_Ip> __n, _Alloc&& __a = _Alloc()) {
        auto __rollback = __first;
        try {
            for (; __n > 0; (void)++__first, --__n) {
                allocator_traits<remove_cvref_t<_Alloc>>::construct(__a, std::addressof(*__first));
            }
            return __first;
        } catch (...) {
            for (; __rollback != __first; ++__rollback) {
                destroy_at(std::addressof(*__rollback), __a);
            }
            throw;
        }
    }
} uninitialized_value_construct_n;

// =============================================================================
// 26.11.5 uninitialized_copy [uninitialized.copy]
// =============================================================================

inline constexpr struct __uninit_copy_fn {
    template<
        input_iterator _Ip, sentinel_for<_Ip> _Sp1, __detail::nothrow_forward_iterator _Op,
        __detail::nothrow_sentinel_for<_Op> _Sp2,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<iter_value_t<_Op>>>
    requires constructible_from<iter_value_t<_Op>, iter_reference_t<_Ip>>
    static constexpr uninitialized_copy_result<_Ip, _Op>
    operator()(_Ip __ifirst, _Sp1 __ilast, _Op __ofirst, _Sp2 __olast, _Alloc&& __a = _Alloc()) {
        auto __rollback = __ofirst;
        try {
            for (; __ifirst != __ilast && __ofirst != __olast; ++__ofirst, (void)++__ifirst) {
                allocator_traits<remove_cvref_t<_Alloc>>::construct(
                    __a, std::addressof(*__ofirst), *__ifirst
                );
            }
            return {std::move(__ifirst), __ofirst};
        } catch (...) {
            for (; __rollback != __ofirst; ++__rollback) {
                destroy_at(std::addressof(*__rollback), __a);
            }
            throw;
        }
    }

    template<
        input_range _IR, __detail::nothrow_forward_range _OR,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<range_value_t<_OR>>>
    requires constructible_from<range_value_t<_OR>, range_reference_t<_IR>>
    static constexpr uninitialized_copy_result<borrowed_iterator_t<_IR>, borrowed_iterator_t<_OR>>
    operator()(_IR&& __in_range, _OR&& __out_range, _Alloc&& __a = _Alloc()) {
        return operator()(
            ranges::begin(__in_range), ranges::end(__in_range), ranges::begin(__out_range),
            ranges::end(__out_range), FWD(__a)
        );
    }
} uninitialized_copy;

inline constexpr struct __uninit_copy_n_fn {
    template<
        input_iterator _Ip, __detail::nothrow_forward_iterator _Op,
        __detail::nothrow_sentinel_for<_Op> _Sp,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<iter_value_t<_Op>>>
    requires constructible_from<iter_value_t<_Op>, iter_reference_t<_Ip>>
    static constexpr uninitialized_copy_n_result<_Ip, _Op> operator()(
        _Ip __ifirst, iter_difference_t<_Ip> __n, _Op __ofirst, _Sp __olast, _Alloc&& __a = _Alloc()
    ) {
        auto __rollback = __ofirst;
        try {
            for (; __n > 0 && __ofirst != __olast; ++__ofirst, (void)++__ifirst, --__n) {
                allocator_traits<remove_cvref_t<_Alloc>>::construct(
                    __a, std::addressof(*__ofirst), *__ifirst
                );
            }
            return {std::move(__ifirst), __ofirst};
        } catch (...) {
            for (; __rollback != __ofirst; ++__rollback) {
                destroy_at(std::addressof(*__rollback), __a);
            }
            throw;
        }
    }
} uninitialized_copy_n;

// =============================================================================
// 26.11.6 uninitialized_move [uninitialized.move]
// =============================================================================

inline constexpr struct __uninit_move_fn {
    template<
        input_iterator _Ip, sentinel_for<_Ip> _Sp1, __detail::nothrow_forward_iterator _Op,
        __detail::nothrow_sentinel_for<_Op> _Sp2,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<iter_value_t<_Op>>>
    requires constructible_from<iter_value_t<_Op>, iter_rvalue_reference_t<_Ip>>
    static constexpr uninitialized_move_result<_Ip, _Op>
    operator()(_Ip __ifirst, _Sp1 __ilast, _Op __ofirst, _Sp2 __olast, _Alloc&& __a = _Alloc()) {
        auto __rollback = __ofirst;
        try {
            for (; __ifirst != __ilast && __ofirst != __olast; ++__ofirst, (void)++__ifirst) {
                allocator_traits<remove_cvref_t<_Alloc>>::construct(
                    __a, std::addressof(*__ofirst), ranges::iter_move(__ifirst)
                );
            }
            return {std::move(__ifirst), __ofirst};
        } catch (...) {
            for (; __rollback != __ofirst; ++__rollback) {
                destroy_at(std::addressof(*__rollback), __a);
            }
            throw;
        }
    }

    template<
        input_range _IR, __detail::nothrow_forward_range _OR,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<range_value_t<_OR>>>
    requires constructible_from<range_value_t<_OR>, range_rvalue_reference_t<_IR>>
    static constexpr uninitialized_move_result<borrowed_iterator_t<_IR>, borrowed_iterator_t<_OR>>
    operator()(_IR&& __in_range, _OR&& __out_range, _Alloc&& __a = _Alloc()) {
        return operator()(
            ranges::begin(__in_range), ranges::end(__in_range), ranges::begin(__out_range),
            ranges::end(__out_range), FWD(__a)
        );
    }
} uninitialized_move;

inline constexpr struct __uninit_move_n_fn {
    template<
        input_iterator _Ip, __detail::nothrow_forward_iterator _Op,
        __detail::nothrow_sentinel_for<_Op> _Sp,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<iter_value_t<_Op>>>
    requires constructible_from<iter_value_t<_Op>, iter_rvalue_reference_t<_Ip>>
    static constexpr uninitialized_move_n_result<_Ip, _Op> operator()(
        _Ip __ifirst, iter_difference_t<_Ip> __n, _Op __ofirst, _Sp __olast, _Alloc&& __a = _Alloc()
    ) {
        auto __rollback = __ofirst;
        try {
            for (; __n > 0 && __ofirst != __olast; ++__ofirst, (void)++__ifirst, --__n) {
                allocator_traits<remove_cvref_t<_Alloc>>::construct(
                    __a, std::addressof(*__ofirst), ranges::iter_move(__ifirst)
                );
            }
            return {std::move(__ifirst), __ofirst};
        } catch (...) {
            for (; __rollback != __ofirst; ++__rollback) {
                destroy_at(std::addressof(*__rollback), __a);
            }
            throw;
        }
    }
} uninitialized_move_n;

// =============================================================================
// 26.11.7 uninitialized_fill [uninitialized.fill]
// =============================================================================

inline constexpr struct __uninit_fill_fn {
    template<
        __detail::nothrow_forward_iterator _Ip, __detail::nothrow_sentinel_for<_Ip> _Sp,
        class _Tp                 = iter_value_t<_Ip>,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<iter_value_t<_Ip>>>
    requires constructible_from<iter_value_t<_Ip>, _Tp const&> static constexpr _Ip
    operator()(_Ip __first, _Sp __last, _Tp const& __x, _Alloc&& __a = _Alloc()) {
        auto __rollback = __first;
        try {
            for (; __first != __last; ++__first) {
                allocator_traits<remove_cvref_t<_Alloc>>::construct(
                    __a, std::addressof(*__first), __x
                );
            }
            return __first;
        } catch (...) {
            for (; __rollback != __first; ++__rollback) {
                destroy_at(std::addressof(*__rollback), __a);
            }
            throw;
        }
    }

    template<
        __detail::nothrow_forward_range _Rp, class _Tp = range_value_t<_Rp>,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<range_value_t<_Rp>>>
    requires constructible_from<range_value_t<_Rp>, _Tp const&>
    static constexpr borrowed_iterator_t<_Rp>
    operator()(_Rp&& __r, _Tp const& __x, _Alloc&& __a = _Alloc()) {
        return operator()(ranges::begin(__r), ranges::end(__r), __x, FWD(__a));
    }
} uninitialized_fill;

inline constexpr struct __uninit_fill_n_fn {
    template<
        __detail::nothrow_forward_iterator _Ip, class _Tp = iter_value_t<_Ip>,
        __simple_allocator _Alloc = __detail::__non_allocating_allocator<iter_value_t<_Ip>>>
    requires constructible_from<iter_value_t<_Ip>, _Tp const&> static constexpr _Ip
    operator()(_Ip __first, iter_difference_t<_Ip> __n, _Tp const& __x, _Alloc&& __a = _Alloc()) {
        auto __rollback = __first;
        try {
            for (; __n > 0; (void)++__first, --__n) {
                allocator_traits<remove_cvref_t<_Alloc>>::construct(
                    __a, std::addressof(*__first), __x
                );
            }
            return __first;
        } catch (...) {
            for (; __rollback != __first; ++__rollback) {
                destroy_at(std::addressof(*__rollback), __a);
            }
            throw;
        }
    }
} uninitialized_fill_n;
}  // namespace ranges

}  // namespace std
