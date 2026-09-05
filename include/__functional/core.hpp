#pragma once

#include <__memory/base.hpp>
#include <compare>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

#include <__functional/hash.hpp>
#include <__functional/mem_ptr_traits.hpp>

namespace std {

namespace __refwrap {
template<class T> struct impl {
    static void FUN(T&) noexcept;
    static void FUN(T&&) = delete;
};
}  // namespace __refwrap

template<class T> class reference_wrapper {
public:
    // types
    using type = T;

    // [refwrap.const], constructors
    template<class U>
    constexpr reference_wrapper(U&& u) noexcept(noexcept(__refwrap::impl<T>::FUN(declval<U>())))
        requires(!is_same_v<remove_cvref_t<U>, reference_wrapper>)
             && requires { __refwrap::impl<T>::FUN(declval<U>()); } {
        T& r = FWD(u);
        ptr  = std::addressof(r);
    }

    constexpr reference_wrapper(reference_wrapper const& x) noexcept = default;

    // [refwrap.assign], assignment
    constexpr reference_wrapper& operator=(reference_wrapper const& x) noexcept = default;

    // [refwrap.access], access
    constexpr operator T&() const noexcept { return *ptr; }
    constexpr T& get() const noexcept { return *ptr; }

    // [refwrap.invoke], invocation
    template<class... ArgTypes>
    constexpr invoke_result_t<T&, ArgTypes...>
    operator()(ArgTypes&&... as) const noexcept(is_nothrow_invocable_v<T&, ArgTypes...>) {
        static_assert(sizeof(T) != 0, "T must be a complete type");
        return std::invoke(get(), FWD(as)...);
    }

    // [refwrap.comparisons], comparisons
    friend constexpr bool operator==(reference_wrapper x, reference_wrapper y) requires requires {
        { x.get() == y.get() } -> boolean_testable;
    } {
        return x.get() == y.get();
    }
    friend constexpr bool operator==(reference_wrapper x, T const& y) requires requires {
        { x.get() == y } -> boolean_testable;
    } {
        return x.get() == y;
    }
    friend constexpr bool operator==(reference_wrapper x, reference_wrapper<T const> y)
        requires requires {
            { x.get() == y.get() } -> boolean_testable;
        } {
        return x.get() == y.get();
    }

    friend constexpr auto operator<=>(reference_wrapper x, reference_wrapper y)
        requires requires { __synth_three_way(x.get(), y.get()); } {
        return __synth_three_way(x.get(), y.get());
    }
    friend constexpr auto operator<=>(reference_wrapper x, T const& y)
        requires requires { __synth_three_way(x.get(), y); } {
        return __synth_three_way(x.get(), y);
    }
    friend constexpr auto operator<=>(reference_wrapper x, reference_wrapper<T const> y)
        requires requires { __synth_three_way(x.get(), y.get()); } {
        return __synth_three_way(x.get(), y.get());
    }

private:
    T* ptr;
};

template<class T> reference_wrapper(T&) -> reference_wrapper<T>;

template<class T> constexpr reference_wrapper<T> ref(T& t) noexcept {
    return reference_wrapper<T>(t);
}
template<class T> constexpr reference_wrapper<T> ref(reference_wrapper<T> t) noexcept {
    return t;
}
template<class T> constexpr reference_wrapper<T const> cref(T const& t) noexcept {
    return reference_wrapper<T const>(t);
}
template<class T> constexpr reference_wrapper<T const> cref(reference_wrapper<T> t) noexcept {
    return t;
}

template<class T> void ref(T const&&)  = delete;
template<class T> void cref(T const&&) = delete;

template<class T> constexpr bool __is_ref_wrapper = false;  // exposition only

template<class T> constexpr bool __is_ref_wrapper<reference_wrapper<T>> = true;

template<class R, class T, class RQ, class TQ>
concept __ref_wrap_common_reference_exists_with =  // exposition only
    __is_ref_wrapper<R> && requires { typename common_reference_t<typename R::type&, TQ>; }
    && convertible_to<RQ, common_reference_t<typename R::type&, TQ>>;

template<class R, class T, template<class> class RQual, template<class> class TQual> requires(
    __ref_wrap_common_reference_exists_with<R, T, RQual<R>, TQual<T>>
    && !__ref_wrap_common_reference_exists_with<T, R, TQual<T>, RQual<R>>
) struct basic_common_reference<R, T, RQual, TQual> {
    using type = common_reference_t<typename R::type&, TQual<T>>;
};

template<class T, class R, template<class> class TQual, template<class> class RQual> requires(
    __ref_wrap_common_reference_exists_with<R, T, RQual<R>, TQual<T>>
    && !__ref_wrap_common_reference_exists_with<T, R, TQual<T>, RQual<R>>
) struct basic_common_reference<T, R, TQual, RQual> {
    using type = common_reference_t<typename R::type&, TQual<T>>;
};

#define _DECL_OP_IMPL(R, name, op)                                                                 \
    template<class T = void> struct name {                                                         \
        constexpr R operator()(T const& x, T const& y) const { return x op y; }                    \
    };                                                                                             \
                                                                                                   \
    template<> struct name<void> {                                                                 \
        template<class T, class U>                                                                 \
        constexpr auto operator()(T&& t, U&& u) const -> decltype(FWD(t) op FWD(u)) {              \
            return FWD(t) op FWD(u);                                                               \
        }                                                                                          \
                                                                                                   \
        using is_transparent = void;                                                               \
    };

#define _DECL_OP(name, op)   _DECL_OP_IMPL(T, name, op)
#define _DECL_OP_B(name, op) _DECL_OP_IMPL(bool, name, op)

_DECL_OP(plus, +)
_DECL_OP(minus, -)
_DECL_OP(multiplies, *)
_DECL_OP(divides, /)
_DECL_OP(modulus, %)

_DECL_OP(bit_and, &)
_DECL_OP(bit_or, |)
_DECL_OP(bit_xor, ^)

template<class T = void> struct negate {
    constexpr T operator()(T const& x) const { return -x; }
};
template<> struct negate<void> {
    using is_transparent = void;
    constexpr auto operator()(auto&& x) const -> decltype(-FWD(x)) { return -FWD(x); }
};

template<class T = void> struct bit_not {
    constexpr T operator()(T const& x) const { return ~x; }
};
template<> struct bit_not<void> {
    using is_transparent = void;
    constexpr auto operator()(auto&& x) const -> decltype(~FWD(x)) { return ~FWD(x); }
};

template<class T = void> struct logical_not {
    constexpr bool operator()(T const& x) const { return !x; }
};

template<> struct logical_not<void> {
    template<class T> constexpr auto operator()(T&& t) const -> decltype(!FWD(t)) {
        return !FWD(t);
    }

    using is_transparent = void;
};

_DECL_OP_B(equal_to, ==)
_DECL_OP_B(not_equal_to, !=)
_DECL_OP_B(greater, >)
_DECL_OP_B(less, <)
_DECL_OP_B(greater_equal, >=)
_DECL_OP_B(less_equal, <=)
_DECL_OP_B(logical_and, &&)
_DECL_OP_B(logical_or, ||)

#undef _DECL_OP
#undef _DECL_OP_IMPL
#undef _DECL_OP_B

struct identity {
    template<class T> constexpr T&& operator()(T&& t) const noexcept { return FWD(t); }

    using is_transparent = void;
};

template<class F>
constexpr auto not_fn(F&& f)
    requires is_constructible_v<decay_t<F>, F> && is_move_constructible_v<decay_t<F>> {
    return [f2 = FWD(f)](this auto&& self, auto&&... as) noexcept(
               noexcept(!std::invoke(std::forward_like<decltype(self)>(f2), FWD(as)...))
           ) -> decltype(!std::invoke(std::forward_like<decltype(self)>(f2), FWD(as)...)) {
        return !std::invoke(std::forward_like<decltype(self)>(f2), FWD(as)...);
    };
}

template<auto F> constexpr auto not_fn() noexcept {
    using T = decltype(F);
    if constexpr (is_pointer_v<T> || is_member_pointer_v<T>) { static_assert(F != nullptr); }
    return [](auto&&... as) noexcept(noexcept(!std::invoke(F, FWD(as)...)))
               requires requires { !std::invoke(F, FWD(as)...); } {
                   return !std::invoke(F, FWD(as)...);
               };
}

template<class F, class... As> constexpr auto bind_front(F&& f, As&&... as) {
    using FD = decay_t<F>;
    static_assert(
        is_constructible_v<FD, F> && is_move_constructible_v<FD>
        && (is_constructible_v<decay_t<As>, As> && ...)
        && (is_move_constructible_v<decay_t<As>> && ...)
    );

    return [f2 = FWD(f), ... bound = FWD(as)]<class Self>(this Self&&, auto&&... as2) noexcept(
               noexcept(std::invoke(
                   std::forward_like<Self>(f2), std::forward_like<Self>(bound)..., FWD(as2)...
               ))
           )
               -> decltype(std::invoke(
                   std::forward_like<Self>(f2), std::forward_like<Self>(bound)..., FWD(as2)...
               )) {
        return std::invoke(
            std::forward_like<Self>(f2), std::forward_like<Self>(bound)..., FWD(as2)...
        );
    };
}

template<class F, class... As> constexpr auto bind_back(F&& f, As&&... as) {
    using FD = decay_t<F>;
    static_assert(
        is_constructible_v<FD, F> && is_move_constructible_v<FD>
        && (is_constructible_v<decay_t<As>, As> && ...)
        && (is_move_constructible_v<decay_t<As>> && ...)
    );

    return [f2 = FWD(f), ... bound = FWD(as)]<class Self>(this Self&&, auto&&... as2) noexcept(
               noexcept(std::invoke(
                   std::forward_like<Self>(f2), FWD(as2)..., std::forward_like<Self>(bound)...
               ))
           )
               -> decltype(std::invoke(
                   std::forward_like<Self>(f2), FWD(as2)..., std::forward_like<Self>(bound)...
               )) {
        return std::invoke(
            std::forward_like<Self>(f2), FWD(as2)..., std::forward_like<Self>(bound)...
        );
    };
}

template<auto F, class... As> constexpr auto bind_front(As&&... as) {
    static_assert(
        (is_constructible_v<decay_t<As>, As> && ...)
        && (is_move_constructible_v<decay_t<As>> && ...)
    );

    return [... bound = FWD(as)]<class Self>(this Self&&, auto&&... as2) noexcept(
               noexcept(std::invoke(F, std::forward_like<Self>(bound)..., FWD(as2)...))
           ) -> decltype(std::invoke(F, std::forward_like<Self>(bound)..., FWD(as2)...)) {
        return std::invoke(F, std::forward_like<Self>(bound)..., FWD(as2)...);
    };
}

template<auto F, class... As> constexpr auto bind_back(As&&... as) {
    static_assert(
        (is_constructible_v<decay_t<As>, As> && ...)
        && (is_move_constructible_v<decay_t<As>> && ...)
    );

    return [... bound = FWD(as)]<class Self>(this Self&&, auto&&... as2) noexcept(
               noexcept(std::invoke(F, FWD(as2)..., std::forward_like<Self>(bound)...))
           ) -> decltype(std::invoke(F, FWD(as2)..., std::forward_like<Self>(bound)...)) {
        return std::invoke(F, FWD(as2)..., std::forward_like<Self>(bound)...);
    };
}

template<class R, class T> constexpr auto mem_fn(R T::* pm) noexcept {
    return [=](auto&&... as) noexcept(
               noexcept(std::invoke(pm, FWD(as)...))
           ) -> decltype(std::invoke(pm, FWD(as)...)) {
        return std::invoke(pm, FWD(as)...);
    };
}

namespace ranges {

struct equal_to {
    using is_transparent = void;

    template<class T, class U>
    static constexpr bool operator()(T&& t, U&& u) noexcept(noexcept(FWD(t) == FWD(u)))
        requires equality_comparable_with<T, U> {
        return FWD(t) == FWD(u);
    }
};

struct not_equal_to {
    using is_transparent = void;

    template<class T, class U>
    static constexpr bool operator()(T&& t, U&& u) noexcept(noexcept(!equal_to{}(FWD(t), FWD(u))))
        requires equality_comparable_with<T, U> {
        return !equal_to{}(FWD(t), FWD(u));
    }
};

struct less {
    template<class T, class U>
    static constexpr bool operator()(T&& t, U&& u) noexcept(noexcept(FWD(t) < FWD(u)))
        requires totally_ordered_with<T, U> {
        return FWD(t) < FWD(u);
    }

    using is_transparent = void;
};

struct greater {
    template<class T, class U>
    static constexpr bool
    operator()(T&& t, U&& u) noexcept(noexcept(ranges::less{}(FWD(u), FWD(t))))
        requires totally_ordered_with<T, U> {
        return ranges::less{}(FWD(u), FWD(t));
    }

    using is_transparent = void;
};

struct less_equal {
    template<class T, class U>
    static constexpr bool
    operator()(T&& t, U&& u) noexcept(noexcept(!ranges::less{}(FWD(u), FWD(t))))
        requires totally_ordered_with<T, U> {
        return !ranges::less{}(FWD(u), FWD(t));
    }

    using is_transparent = void;
};

struct greater_equal {
    template<class T, class U>
    static constexpr bool
    operator()(T&& t, U&& u) noexcept(noexcept(!ranges::less{}(FWD(t), FWD(u))))
        requires totally_ordered_with<T, U> {
        return !ranges::less{}(FWD(t), FWD(u));
    }

    using is_transparent = void;
};

}  // namespace ranges
}  // namespace std
