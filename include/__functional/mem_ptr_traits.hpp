#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>

namespace std::__detail {

    // TODO fix volatile as in __maybe_const

enum class ref_qualifier { none, lvalue, rvalue };

// -----------------------------------------------------------------------------
// 1. Primary Template (Non-member-pointer fallback)
// -----------------------------------------------------------------------------
template<typename T> struct mem_ptr_traits {
    static constexpr bool is_member_pointer          = false;
    static constexpr bool is_member_function_pointer = false;
    static constexpr bool is_member_object_pointer   = false;
};

// -----------------------------------------------------------------------------
// 2. Member Object Pointer Specialization (M G::*)
// -----------------------------------------------------------------------------
template<typename M, typename G> requires(!std::is_function_v<M>) struct mem_ptr_traits<M G::*> {
    static constexpr bool is_member_pointer          = true;
    static constexpr bool is_member_function_pointer = false;
    static constexpr bool is_member_object_pointer   = true;

    using class_type  = G;
    using member_type = M;
};

// -----------------------------------------------------------------------------
// 3. Member Function Pointer Specializations
// -----------------------------------------------------------------------------

template<
    typename Class, typename Ret, bool Const, bool Volatile, ref_qualifier Ref, bool Noex,
    typename... Args>
struct __mem_fn_traits_base {
    static constexpr bool is_member_pointer          = true;
    static constexpr bool is_member_function_pointer = true;
    static constexpr bool is_member_object_pointer   = false;

    using class_type  = Class;
    using return_type = Ret;
    using args_tuple  = std::tuple<Args...>;

    template<std::size_t I> using arg_t = Args...[I];

    static constexpr std::size_t arity      = sizeof...(Args);
    static constexpr bool is_const          = Const;
    static constexpr bool is_volatile       = Volatile;
    static constexpr ref_qualifier ref_qual = Ref;
    static constexpr bool is_noexcept       = Noex;

    // Stripped signature without class: Ret(Args...) noexcept(Noex)
    using function_signature = Ret(Args...) noexcept(Noex);

    // Object type with cv/ref qualifications applied to Class
    using qualified_class_type = std::conditional_t<
        Ref == ref_qualifier::rvalue, __maybe_const<Const, Class>&&, __maybe_const<Const, Class>&>;

    // Invokable signature with object as 1st parameter: Ret(ClassQualifier, Args...)
    using full_signature = Ret(qualified_class_type, Args...) noexcept(Noex);
};

#define __EXPAND_MEM_FN_TRAITS(CV, CONST, VOL, REF_Q, REF_ENUM, NOEX)                              \
    template<typename R, typename G, typename... Args>                                             \
    struct mem_ptr_traits<R (G::*)(Args...) CV REF_Q noexcept(NOEX)>                               \
        : __mem_fn_traits_base<                                                          \
              G, R, CONST, VOL, ref_qualifier::REF_ENUM, NOEX, Args...> {};

// Const combinations
__EXPAND_MEM_FN_TRAITS(, false, false, , none, false)
__EXPAND_MEM_FN_TRAITS(, false, false, , none, true)
__EXPAND_MEM_FN_TRAITS(, false, false, &, lvalue, false)
__EXPAND_MEM_FN_TRAITS(, false, false, &, lvalue, true)
__EXPAND_MEM_FN_TRAITS(, false, false, &&, rvalue, false)
__EXPAND_MEM_FN_TRAITS(, false, false, &&, rvalue, true)

__EXPAND_MEM_FN_TRAITS(volatile, false, true, , none, false)
__EXPAND_MEM_FN_TRAITS(volatile, false, true, , none, true)
__EXPAND_MEM_FN_TRAITS(volatile, false, true, &, lvalue, false)
__EXPAND_MEM_FN_TRAITS(volatile, false, true, &, lvalue, true)
__EXPAND_MEM_FN_TRAITS(volatile, false, true, &&, rvalue, false)
__EXPAND_MEM_FN_TRAITS(volatile, false, true, &&, rvalue, true)

__EXPAND_MEM_FN_TRAITS(const, true, false, , none, false)
__EXPAND_MEM_FN_TRAITS(const, true, false, , none, true)
__EXPAND_MEM_FN_TRAITS(const, true, false, &, lvalue, false)
__EXPAND_MEM_FN_TRAITS(const, true, false, &, lvalue, true)
__EXPAND_MEM_FN_TRAITS(const, true, false, &&, rvalue, false)
__EXPAND_MEM_FN_TRAITS(const, true, false, &&, rvalue, true)

__EXPAND_MEM_FN_TRAITS(const volatile, true, true, , none, false)
__EXPAND_MEM_FN_TRAITS(const volatile, true, true, , none, true)
__EXPAND_MEM_FN_TRAITS(const volatile, true, true, &, lvalue, false)
__EXPAND_MEM_FN_TRAITS(const volatile, true, true, &, lvalue, true)
__EXPAND_MEM_FN_TRAITS(const volatile, true, true, &&, rvalue, false)
__EXPAND_MEM_FN_TRAITS(const volatile, true, true, &&, rvalue, true)

#undef __EXPAND_MEM_FN_TRAITS

// -----------------------------------------------------------------------------
// 4. Convenience Value & Type Helpers
// -----------------------------------------------------------------------------

template<typename T> using mem_ptr_class_t = typename mem_ptr_traits<T>::class_type;

template<typename T> using mem_ptr_return_t = typename mem_ptr_traits<T>::return_type;

template<typename T> using mem_ptr_signature_t = typename mem_ptr_traits<T>::function_signature;

template<typename T> using mem_ptr_full_signature_t = typename mem_ptr_traits<T>::full_signature;

}  // namespace std::__detail
