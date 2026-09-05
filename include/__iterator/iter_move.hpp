#pragma once
// code: language=c++
// IWYU pragma: private: include <ranges>

#include <type_traits>
#include <utility>

namespace std::ranges {

namespace __detail {

using namespace ::std::__detail;

template<class T> concept __is_lvalue = is_lvalue_reference_v<T>;
template<class T> concept __is_rvalue = is_rvalue_reference_v<T>;

struct __iter_move {
    template<class T, class U = remove_cvref_t<T>>
    requires(is_class_v<U> || is_enum_v<U>) && requires(T&& t) { iter_move(FWD(t)); }
    static constexpr auto __impl(T&& t, __priority_tag<2>) _STD_RETURN(iter_move(FWD(t)));

    template<class T> requires requires(T&& t) {
        { *FWD(t) } -> __is_lvalue;
    } static constexpr auto __impl(T&& t, __priority_tag<1>) _STD_RETURN(std::move(*t));

    template<class T> requires requires(T&& t) {
        { *FWD(t) } -> __is_rvalue;
    } static constexpr auto __impl(T&& t, __priority_tag<0>) _STD_RETURN(*t);

    template<class T> requires requires(T&& t) { __impl(FWD(t), __priority_tag<2>()); }
    static constexpr auto operator()(T&& t) _STD_RETURN(__impl(FWD(t), __priority_tag<2>()));
};

}  // namespace __detail

inline namespace __cpo {
inline constexpr __detail::__iter_move iter_move;
}

}  // namespace std::ranges
