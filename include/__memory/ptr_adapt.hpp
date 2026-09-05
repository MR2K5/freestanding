#pragma once

#include <__memory/base.hpp>
#include <tuple>
#include <type_traits>
#include <utility>

namespace std {

namespace __detail {

// Determines POINTER_OF_OR(Smart, Pointer) [memory.general]
template<class Smart, class Fallback> struct __pointer_of_or {
    using type = Fallback;
};

template<class Smart, class Fallback> requires requires { typename Smart::pointer; }
struct __pointer_of_or<Smart, Fallback> {
    using type = typename Smart::pointer;
};

template<class Smart, class Fallback> requires(!requires { typename Smart::pointer; }) && requires {
    typename Smart::element_type;
} struct __pointer_of_or<Smart, Fallback> {
    using type = typename Smart::element_type*;
};

template<class Smart, class Fallback>
using __pointer_of_or_t = typename __pointer_of_or<Smart, Fallback>::type;

// Determines POINTER_OF(Smart) [memory.general]
template<class Smart> struct __pointer_of {
    using type = void*;
};

template<class Smart> requires requires { typename Smart::pointer; } struct __pointer_of<Smart> {
    using type = typename Smart::pointer;
};

template<class Smart> requires(!requires { typename Smart::pointer; }) && requires {
    typename Smart::element_type;
} struct __pointer_of<Smart> {
    using type = typename Smart::element_type*;
};

template<class Smart> using __pointer_of_t = typename __pointer_of<Smart>::type;

}  // namespace __detail

// [out.ptr.t], class template out_ptr_t
template<class Smart, class Pointer, class... Args> class out_ptr_t {
    Smart& s;
    tuple<Args...> a;
    Pointer p;

public:
    constexpr explicit out_ptr_t(Smart& smart, Args... args): s(smart), a(FWD(args)...), p() {
        if constexpr (requires { s.reset(); }) {
            s.reset();
        } else if constexpr (is_constructible_v<Smart>) {
            s = Smart();
        } else {
            static_assert(
                sizeof(Smart) == 0,
                "Smart must be resettable via .reset() or default-constructible."
            );
        }
    }

    out_ptr_t(out_ptr_t const&)            = delete;
    out_ptr_t& operator=(out_ptr_t const&) = delete;

    constexpr ~out_ptr_t() {
        using SP = __detail::__pointer_of_or_t<Smart, Pointer>;

        if (p) {
            if constexpr (requires {
                              std::apply(
                                  [&](auto&&... args) {
                                      s.reset(static_cast<SP>(p), FWD(args)...);
                                  },
                                  std::move(a)
                              );
                          }) {
                std::apply(
                    [&](auto&&... args) { s.reset(static_cast<SP>(p), FWD(args)...); }, std::move(a)
                );
            } else if constexpr (is_constructible_v<Smart, SP, Args...>) {
                std::apply(
                    [&](auto&&... args) { s = Smart(static_cast<SP>(p), FWD(args)...); },
                    std::move(a)
                );
            } else {
                static_assert(
                    sizeof(Smart) == 0, "Smart cannot be reset or constructed with (SP, Args...)."
                );
            }
        }
    }

    constexpr operator Pointer*() const noexcept { return std::addressof(const_cast<Pointer&>(p)); }

    operator void**() const noexcept requires(!is_same_v<Pointer, void*>) {
        static_assert(
            is_pointer_v<Pointer>, "Pointer must be a raw pointer type to convert to void**."
        );
        return reinterpret_cast<void**>(const_cast<Pointer*>(std::addressof(p)));
    }
};

// [out.ptr], function template out_ptr
template<class Pointer = void, class Smart, class... Args>
constexpr auto out_ptr(Smart& s, Args&&... args) {
    using P = conditional_t<is_void_v<Pointer>, __detail::__pointer_of_t<Smart>, Pointer>;
    return out_ptr_t<Smart, P, Args&&...>(s, FWD(args)...);
}

// [inout.ptr.t], class template inout_ptr_t
template<class Smart, class Pointer, class... Args> class inout_ptr_t {
    Smart& s;
    tuple<Args...> a;
    Pointer p;

public:
    constexpr explicit inout_ptr_t(Smart& smart, Args... args)
        : s(smart), a(FWD(args)...), p([&]() -> Pointer {
              if constexpr (is_pointer_v<Smart>) {
                  return smart;
              } else {
                  return smart.get();
              }
          }()) {}

    inout_ptr_t(inout_ptr_t const&)            = delete;
    inout_ptr_t& operator=(inout_ptr_t const&) = delete;

    constexpr ~inout_ptr_t() {
        using SP = __detail::__pointer_of_or_t<Smart, Pointer>;

        if constexpr (is_pointer_v<Smart>) {
            // (11.1) Raw pointers are updated unconditionally
            std::apply(
                [&](auto&&... args) { s = Smart(static_cast<SP>(p), FWD(args)...); }, std::move(a)
            );
        } else {
            // (11.2 & 11.3) Release original ownership from the smart pointer
            if constexpr (requires { s.release(); }) { s.release(); }

            // If the C function produced a valid new pointer, reset/construct Smart
            if (p) {
                if constexpr (requires {
                                  std::apply(
                                      [&](auto&&... args) {
                                          s.reset(static_cast<SP>(p), FWD(args)...);
                                      },
                                      std::move(a)
                                  );
                              }) {
                    std::apply(
                        [&](auto&&... args) { s.reset(static_cast<SP>(p), FWD(args)...); },
                        std::move(a)
                    );
                } else if constexpr (is_constructible_v<Smart, SP, Args...>) {
                    std::apply(
                        [&](auto&&... args) { s = Smart(static_cast<SP>(p), FWD(args)...); },
                        std::move(a)
                    );
                } else {
                    static_assert(
                        sizeof(Smart) == 0,
                        "Smart cannot be reset or constructed with (SP, Args...)."
                    );
                }
            }
        }
    }

    constexpr operator Pointer*() const noexcept { return std::addressof(const_cast<Pointer&>(p)); }

    operator void**() const noexcept requires(!is_same_v<Pointer, void*>) {
        static_assert(
            is_pointer_v<Pointer>, "Pointer must be a raw pointer type to convert to void**."
        );
        return reinterpret_cast<void**>(const_cast<Pointer*>(std::addressof(p)));
    }
};

// [inout.ptr], function template inout_ptr
template<class Pointer = void, class Smart, class... Args>
constexpr auto inout_ptr(Smart& s, Args&&... args) {
    using P = conditional_t<is_void_v<Pointer>, __detail::__pointer_of_t<Smart>, Pointer>;
    return inout_ptr_t<Smart, P, Args&&...>(s, FWD(args)...);
}

}  // namespace std
