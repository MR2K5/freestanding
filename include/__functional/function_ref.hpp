#pragma once

#include <__functional/mem_ptr_traits.hpp>
#include <__memory/base.hpp>
#include <cassert>
#include <__functional/core.hpp>
#include <type_traits>
#include <utility>

namespace std {

template<class... S> class function_ref;

namespace __detail {
union fnref_BoundEntity {
    void const* obj;
    void (*func)();
};

template<bool IsConst, bool Noex, class R, class... As> class __function_ref {
    using BoundEntity = fnref_BoundEntity;
    template<class... T> static consteval bool is_invocable_using() {
        if constexpr (Noex)
            return is_nothrow_invocable_r_v<R, T..., As...>;
        else
            return is_invocable_r_v<R, T..., As...>;
    }

    using int_t  = __maybe_const<IsConst, int>&;
    using func_t = R (&)(As...) noexcept(Noex);
    using full_t =
        std::conditional_t<IsConst, R(As...) const noexcept(Noex), R(As...) noexcept(Noex)>;

    template<class F> static consteval bool is_convertible_from_specialization() {
        if constexpr (__is_specialization_of_v<function_ref, F>) {
            return is_convertible_v<typename F::func_t, func_t>
                && is_convertible_v<int_t, typename F::int_t>;
        } else {
            return false;
        }
    }

public:
    __function_ref(__function_ref const&) noexcept            = default;
    __function_ref& operator=(__function_ref const&) noexcept = default;
    template<class T> requires(!is_convertible_from_specialization<T>()) && (!is_pointer_v<T>)
                               && (!__is_constant_wrapper_v<T>)  //
    void operator=(T) = delete;

    R operator()(As... as) const noexcept(Noex) { return thunk_ptr_(bound_, FWD(as)...); }

    template<class F> requires is_function_v<F> && (is_invocable_using<F>())
    constexpr __function_ref(F* f) noexcept: bound_{.func = reinterpret_cast<void (*)()>(f)} {
        thunk_ptr_ = [](BoundEntity b, As&&... as) noexcept(Noex) -> R {
            return std::invoke_r<R>(reinterpret_cast<F*>(b.func), FWD(as)...);
        };
    }

    template<class F> requires(!is_same_v<remove_cvref_t<F>, function_ref<full_t>>)
                           && (!is_same_v<remove_cvref_t<F>, __function_ref>)
                           && (!is_member_pointer_v<remove_reference_t<F>>)
                           && (is_invocable_using<__maybe_const<IsConst, remove_reference_t<F>>&>())
    constexpr __function_ref(F&& f) noexcept {
        using RawF = remove_cvref_t<F>;
        using T    = remove_reference_t<F>;
        if constexpr (is_convertible_from_specialization<RawF>()) {
            bound_     = f.bound_;
            thunk_ptr_ = f.thunk_ptr_;
        } else {
            bound_.obj = std::addressof(f);
            thunk_ptr_ = [](BoundEntity b, As&&... as) noexcept(Noex) -> R {
                auto* src = const_cast<T*>(static_cast<T const*>(b.obj));
                using Tgt = __maybe_const<IsConst, T>&;
                return std::invoke_r<R>(static_cast<Tgt>(*src), FWD(as)...);
            };
        }
    }

    template<auto c, class F> requires(is_invocable_using<F const&>())
    constexpr __function_ref(constant_wrapper<c, F> f) noexcept {
        if constexpr (is_pointer_v<F> || is_member_pointer_v<F>)
            static_assert(f.value != nullptr, "Cannot bind function_ref to nullptr");
        if constexpr (sizeof...(As) != 0 && (__constexpr_param<remove_cvref_t<As>> && ...))
            static_assert(!requires {
                typename constant_wrapper<INVOKE(f.value, remove_cvref_t<As>::value...)>;
            });

        thunk_ptr_ = [](BoundEntity, As&&... as) noexcept(Noex) -> R {
            return std::invoke_r<R>(decltype(f)::value, FWD(as)...);
        };
    }

    template<auto c, class F, class U>
    requires(!is_rvalue_reference_v<U &&>)
         && (is_invocable_using<F const&, __maybe_const<IsConst, remove_reference_t<U>>&>())
    constexpr __function_ref(constant_wrapper<c, F> f, U&& obj) noexcept {
        if constexpr (is_pointer_v<F> || is_member_pointer_v<F>)
            static_assert(f.value != nullptr, "Cannot bind function_ref to nullptr");

        using T    = remove_reference_t<U>;
        bound_.obj = std::addressof(obj);
        thunk_ptr_ = [](BoundEntity b, As&&... as) noexcept(Noex) -> R {
            auto* p   = const_cast<T*>(static_cast<T const*>(b.obj));
            using Tgt = __maybe_const<IsConst, T>&;
            return std::invoke_r<R>(decltype(f)::value, static_cast<Tgt>(*p), FWD(as)...);
        };
    }

    template<auto c, class F, class T>
    requires IsConst && (is_invocable_using<F const&, T const*>())
    constexpr __function_ref(constant_wrapper<c, F> f, T const* obj) noexcept {
        if constexpr (is_pointer_v<F> || is_member_pointer_v<F>)
            static_assert(f.value != nullptr, "Cannot bind function_ref to nullptr");
        if constexpr (is_member_pointer_v<F>) assert(obj != nullptr);

        bound_.obj = obj;
        thunk_ptr_ = [](BoundEntity b, As&&... as) noexcept(Noex) -> R {
            return std::invoke_r<R>(decltype(f)::value, static_cast<T const*>(b.obj), FWD(as)...);
        };
    }

    template<auto c, class F, class T> requires(!IsConst) && (is_invocable_using<F const&, T*>())
    constexpr __function_ref(constant_wrapper<c, F> f, T* obj) noexcept {
        if constexpr (is_pointer_v<F> || is_member_pointer_v<F>)
            static_assert(f.value != nullptr, "Cannot bind function_ref to nullptr");
        if constexpr (is_member_pointer_v<F>) assert(obj != nullptr);

        bound_.obj = obj;
        thunk_ptr_ = [](BoundEntity b, As&&... as) noexcept(Noex) -> R {
            return std::invoke_r<R>(
                decltype(f)::value, const_cast<T*>(static_cast<T const*>(b.obj)), FWD(as)...
            );
        };
    }

private:
    BoundEntity bound_{};
    R (*thunk_ptr_)(BoundEntity b, As&&... as) noexcept(Noex){};

    template<bool, bool, class, class...> friend class __function_ref;
};
}  // namespace __detail

template<class R, class... As>
class function_ref<R(As...)>: public __detail::__function_ref<false, false, R, As...> {
    using __detail::__function_ref<false, false, R, As...>::__function_ref;
};
template<class R, class... As>
class function_ref<R(As...) const>: public __detail::__function_ref<true, false, R, As...> {
    using __detail::__function_ref<true, false, R, As...>::__function_ref;
};
template<class R, class... As>
class function_ref<R(As...) noexcept>: public __detail::__function_ref<false, true, R, As...> {
    using __detail::__function_ref<false, true, R, As...>::__function_ref;
};
template<class R, class... As>
class function_ref<R(As...) const noexcept>: public __detail::__function_ref<true, true, R, As...> {
    using __detail::__function_ref<true, true, R, As...>::__function_ref;
};

template<class F> requires is_function_v<F> function_ref(F*) -> function_ref<F>;

template<auto c, class F0> requires is_function_v<remove_pointer_t<F0>>
function_ref(constant_wrapper<c, F0>) -> function_ref<remove_pointer_t<F0>>;

template<auto c, class F, class T> requires is_member_function_pointer_v<F>
function_ref(constant_wrapper<c, F>, T&&)
    -> function_ref<typename __detail::mem_ptr_traits<F>::function_signature>;

template<auto c, class F, class T> requires is_member_object_pointer_v<F>
function_ref(constant_wrapper<c, F>, T&&) -> function_ref<std::invoke_result_t<F, T&>() noexcept>;

template<class> struct __funcref_deduction_traits: false_type {};
template<class R, class G, class... As>
struct __funcref_deduction_traits<R (*)(G, As...)>: true_type {
    using type = R(As...);
};
template<class R, class G, class... As>
struct __funcref_deduction_traits<R (*)(G, As...) noexcept>: true_type {
    using type = R(As...) noexcept;
};

template<auto c, class F, class T> requires(__funcref_deduction_traits<F>::value)
function_ref(constant_wrapper<c, F>, T&&)
    -> function_ref<typename __funcref_deduction_traits<F>::type>;

}  // namespace std
