#pragma once

#include <tuple>
#include <type_traits>

#include <__memory/base.hpp>

namespace std {

template<class Smart, class Pointer, class... As> class out_ptr_t {
public:
    constexpr explicit out_ptr_t(Smart& smart, As&&... as): s(smart), a(FWD(as)...), p() {
        if constexpr (requires { s.reset(); }) {
            s.reset();
        } else if constexpr (is_constructible_v<Smart>) {
            s = Smart();
        } else {
            static_assert(false);
        }
    }

    out_ptr_t(out_ptr_t const&) = delete;

    constexpr ~out_ptr_t() {
        using SP = __pointer_of_or_t<Smart, Pointer>;
        if constexpr (requires(As&&... as) { s.reset(static_cast<SP>(p), FWD(as)...); }) {
            if (p) {
                std::apply(
                    [&](auto&&... as) { s.reset(static_cast<SP>(p), FWD(as)...); }, std::move(a)
                );
            }
        } else if constexpr (is_constructible_v<Smart, SP, As...>) {
            if (p)
                std::apply(
                    [&](auto&&... as) { s = Smart(static_cast<SP>(p), FWD(as)...); }, std::move(a)
                );
        } else {
            static_assert(false);
        }
    }

    constexpr operator Pointer*() const noexcept {
        return std::addressof(const_cast<Pointer&>(p));
    }

    // TODO prob needs some union hacks
    operator void**() const noexcept;

private:
    Smart& s;
    tuple<As...> a;
    Pointer p;
};

}  // namespace std
