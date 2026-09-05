#pragma once
// code: language=c++

#include <__config.hpp>
#include <__functional/core.hpp>
#include <__iterator/adaptors.hpp>
#include <__iterator/concepts.hpp>
#include <__iterator/const_iterator.hpp>
#include <__memory/base.hpp>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace std::ranges {
namespace views {}
}  // namespace std::ranges

namespace std {
namespace views = ranges::views;

struct from_range_t {
    explicit from_range_t() = default;
};
inline constexpr from_range_t from_range{};

}  // namespace std

namespace std::ranges {

template<class D> requires is_class_v<D> && same_as<D, remove_cv_t<D>> class view_interface;

namespace __detail {
template<class D> consteval auto __test_view_interface(view_interface<D> const*) -> true_type;
consteval false_type __test_view_interface(...);

template<class T>
inline constexpr bool __is_derived_from_view_interface =
    decltype(__test_view_interface(declval<T*>()))::value;

}  // namespace __detail

template<class R> constexpr bool enable_borrowed_range = false;

struct view_base {};

template<class T>
inline constexpr bool enable_view =
    derived_from<T, view_base> || __detail::__is_derived_from_view_interface<T>;

namespace __detail {

void begin() = delete;

struct __begin_fn {
    template<class T> requires is_array_v<remove_cvref_t<T>>
    static constexpr input_or_output_iterator auto __impl(T&& t, __priority_tag<2>) noexcept {
        static_assert(sizeof(T) > 0, "ranges::begin() passed array of incomplete type");
        return t + 0;
    }

    template<class T> requires requires {
        { auto(declval<T>().begin()) } -> input_or_output_iterator;
    } static constexpr input_or_output_iterator auto
    __impl(T&& t, __priority_tag<1>) noexcept(noexcept(auto(declval<T>().begin()))) {
        return auto(std::forward<T>(t).begin());
    }
    template<class T> requires requires {
        { auto(begin(declval<T>())) } -> input_or_output_iterator;
    } static constexpr input_or_output_iterator auto
    __impl(T&& t, __priority_tag<0>) noexcept(noexcept(auto(begin(declval<T>())))) {
        return auto(begin(std::forward<T>(t)));
    }

    template<class T> requires(is_lvalue_reference_v<T> || enable_borrowed_range<remove_cv_t<T>>)
                           && requires { __impl(declval<T>(), __priority_tag<2>()); }
    static constexpr input_or_output_iterator auto
    operator()(T&& t) noexcept(noexcept(__impl(declval<T>(), __priority_tag<2>()))) {
        return __impl(std::forward<T>(t), __priority_tag<2>());
    }
};

}  // namespace __detail

inline namespace __cpo {
inline constexpr __detail::__begin_fn begin;
}

template<class T> using iterator_t = decltype(begin(declval<T&>()));

namespace __detail {

void end() = delete;

struct __end_fn {
    template<class T> requires is_bounded_array_v<remove_cvref_t<T>>
    static constexpr sentinel_for<iterator_t<T>> auto __impl(T&& t, __priority_tag<2>) noexcept {
        static_assert(sizeof(T) > 0, "ranges::end() passed array of incomplete type");
        return t + extent_v<remove_cvref_t<T>>;
    }

    template<class T> requires requires {
        { auto(declval<T>().end()) } -> sentinel_for<iterator_t<T>>;
    } static constexpr sentinel_for<iterator_t<T>> auto
    __impl(T&& t, __priority_tag<1>) noexcept(noexcept(auto(declval<T>().end()))) {
        return auto(std::forward<T>(t).end());
    }
    template<class T> requires requires {
        { auto(end(declval<T>())) } -> sentinel_for<iterator_t<T>>;
    } static constexpr sentinel_for<iterator_t<T>> auto
    __impl(T&& t, __priority_tag<0>) noexcept(noexcept(auto(end(declval<T>())))) {
        return auto(end(std::forward<T>(t)));
    }

    template<class T> requires(is_lvalue_reference_v<T> || enable_borrowed_range<remove_cv_t<T>>)
                           && requires { __impl(declval<T>(), __priority_tag<2>()); }
    static constexpr sentinel_for<iterator_t<T>> auto
    operator()(T&& t) noexcept(noexcept(__impl(declval<T>(), __priority_tag<2>()))) {
        return __impl(std::forward<T>(t), __priority_tag<2>());
    }
};
}  // namespace __detail

inline namespace __cpo {
inline constexpr __detail::__end_fn end;
}

template<class T>
concept range = requires(T& t) {
    ranges::begin(t);
    ranges::end(t);
};

template<ranges::range R> using sentinel_t = decltype(ranges::end(declval<R&>()));

template<class T> concept input_range   = range<T> && input_iterator<iterator_t<T>>;
template<class T> concept forward_range = input_range<T> && forward_iterator<iterator_t<T>>;
template<class T>
concept bidirectional_range = forward_range<T> && bidirectional_iterator<iterator_t<T>>;
template<class T>
concept random_access_range = bidirectional_range<T> && random_access_iterator<iterator_t<T>>;

template<class T>
concept constant_range = input_range<T> && __detail::constant_iterator<iterator_t<T>>;

}  // namespace std::ranges

namespace std {
namespace __detail {

template<ranges::input_range R> constexpr auto& possibly_const_range(R& r) {  // exposition only
    if constexpr (ranges::constant_range<R const> && !ranges::constant_range<R>) {
        return const_cast<R const&>(r);
    } else {
        return r;
    }
}
template<class T> constexpr auto as_const_pointer(T const* p) noexcept {
    return p;
}

}  // namespace __detail

template<input_iterator I>
using const_iterator = conditional_t<__detail::constant_iterator<I>, I, basic_const_iterator<I>>;

template<semiregular S>
using const_sentinel = conditional_t<input_iterator<S>, const_iterator<S>, S>;

template<input_iterator I> constexpr const_iterator<I> make_const_iterator(I it) {
    return it;
}
template<semiregular S> constexpr const_sentinel<S> make_const_sentinel(S s) {
    return s;
}
}  // namespace std

namespace std::ranges {

template<class> constexpr bool disable_sized_range = false;

namespace __detail {
void size() = delete;

struct __size_fn {
    template<class T> requires is_bounded_array_v<remove_cvref_t<T>>
    static constexpr auto __impl(T&&, __priority_tag<3>) noexcept {
        return auto(extent_v<remove_cvref_t<T>>);
    }

    template<class T> requires(!disable_sized_range<remove_cv_t<T>>) && requires {
        { auto(declval<T>().size()) } -> __integer_like;
    } static constexpr auto
    __impl(T&& t, __priority_tag<2>) noexcept(noexcept(auto(declval<T>().size()))) {
        return auto(std::forward<T>(t).size());
    }

    template<class T>
    requires(!disable_sized_range<remove_cv_t<T>>) && (is_class_v<T> || is_enum_v<T>) && requires {
        { auto(size(declval<T>())) } -> __integer_like;
    } static constexpr auto
    __impl(T&& t, __priority_tag<1>) noexcept(noexcept(auto(size(declval<T>)))) {
        return auto(size(std::forward<T>(t)));
    }

    template<forward_range T> requires requires {
        { ranges::begin(declval<T>()) } -> forward_iterator;
        { ranges::end(declval<T>()) } -> sized_sentinel_for<decltype(ranges::begin(declval<T>()))>;
        __to_unsigned_like(ranges::end(declval<T>()) - ranges::begin(declval<T>()));
    }
    static constexpr auto __impl(T&& t, __priority_tag<0>) noexcept(
        noexcept(__to_unsigned_like(ranges::end(declval<T>()) - ranges::begin(declval<T>())))
    ) {
        return __to_unsigned_like(
            ranges::end(std::forward<T>(t)) - ranges::begin(std::forward<T>(t))
        );
    }

    template<class T> requires requires { __impl(declval<T>(), __priority_tag<3>()); }
    static constexpr auto
    operator()(T&& t) noexcept(noexcept(__impl(declval<T>(), __priority_tag<3>()))) {
        return __impl(std::forward<T>(t), __priority_tag<3>());
    }
};

}  // namespace __detail

inline namespace __cpo {
inline constexpr auto cbegin =  //
    []<class T, class U = decltype(begin(__detail::possibly_const_range(declval<T>())))>(T&& t) static noexcept(
        noexcept(const_iterator<U>(begin(__detail::possibly_const_range(std::forward<T>(t)))))
    ) requires is_lvalue_reference_v<T> || enable_borrowed_range<remove_cv_t<T>> {
        return const_iterator<U>(begin(__detail::possibly_const_range(std::forward<T>(t))));
    };

inline constexpr auto cend =
    []<class T, class U = decltype(end(__detail::possibly_const_range(declval<T>())))>
    requires is_lvalue_reference_v<T> || enable_borrowed_range<remove_cv_t<T>>
    (T&& t) static noexcept(
        noexcept(const_sentinel<U>(end(__detail::possibly_const_range(std::forward<T>(t)))))
    ) -> decltype(const_sentinel<U>(end(__detail::possibly_const_range(std::forward<T>(t))))) {
        return const_sentinel<U>(end(__detail::possibly_const_range(std::forward<T>(t))));
    };

inline constexpr __detail::__size_fn size;

inline constexpr auto ssize = []<class T> requires requires {
    size(declval<T>());
}(T && t) static noexcept(noexcept(size(declval<T>()))) {
    using S   = decltype(__detail::__to_signed_like(size(declval<T>())));
    using tgt = conditional_t<sizeof(S) >= sizeof(ptrdiff_t), S, ptrdiff_t>;
    return static_cast<tgt>(size(std::forward<T>(t)));
};

}  // namespace __cpo

namespace __reserve_hint_adl {
void reserve_hint() = delete;
template<class T>
concept __has_adl =
    (is_class_v<remove_cvref_t<T>> || is_enum_v<remove_cvref_t<T>>) && requires(T& t) {
        // ADL-only lookup because reserve_hint() poison pill blocks ordinary lookup
        { auto(reserve_hint(t)) } -> __detail::__integer_like;
    };
}  // namespace __reserve_hint_adl

inline constexpr struct __reserve_hint_fn {
    template<class T>
    static constexpr decltype(auto)
    __impl(T&& t, __detail::__priority_tag<2>) noexcept(noexcept(ranges::size(FWD(t))))
        requires requires { ranges::size(FWD(t)); } {
        return ranges::size(FWD(t));
    }

    template<class T>
    static constexpr decltype(auto)
    __impl(T&& t, __detail::__priority_tag<1>) noexcept(noexcept(auto(t.reserve_hint())))
        requires requires {
            { auto(t.reserve_hint()) } -> __detail::__integer_like;
        } {
        return auto(t.reserve_hint());
    }

    template<class T>
    static constexpr decltype(auto)
    __impl(T&& t, __detail::__priority_tag<0>) noexcept(noexcept(auto(reserve_hint(t)))) {
        return auto(reserve_hint(t));
    }

    static constexpr decltype(auto)
    operator()(auto&& t) noexcept(noexcept(__impl(FWD(t), __detail::__priority_tag<2>())))
        requires requires { __impl(FWD(t), __detail::__priority_tag<2>()); } {
        return __impl(FWD(t), __detail::__priority_tag<2>());
    }
} reserve_hint;

template<range R> using range_reference_t        = iter_reference_t<iterator_t<R>>;
template<range R> using range_const_reference_t  = iter_const_reference_t<iterator_t<R>>;
template<range R> using range_rvalue_reference_t = iter_rvalue_reference_t<iterator_t<R>>;
template<range R> using range_common_reference_t = iter_common_reference_t<iterator_t<R>>;

namespace __detail {
struct __empty_fn {
    template<class T> requires requires { bool(declval<T>().empty()); } static constexpr bool
    __impl(T&& t, __priority_tag<2>) noexcept(noexcept(bool(declval<T>().empty()))) {
        return bool(std::forward<T>(t).empty());
    }

    template<class T> requires requires { size(declval<T>() == 0); } static constexpr bool
    __impl(T&& t, __priority_tag<1>) noexcept(noexcept(size(declval<T>() == 0))) {
        return size(std::forward<T>(t) == 0);
    }

    template<class T> requires requires {
        { ranges::begin(declval<T>()) } -> forward_iterator;
        bool(ranges::begin(declval<T>()) == ranges::end(declval<T>()));
    }
    static constexpr bool __impl(T&& t, __priority_tag<0>) noexcept(
        noexcept(bool(ranges::begin(declval<T>()) == ranges::end(declval<T>())))
    ) {
        return bool(ranges::begin(std::forward<T>(t)) == ranges::end(std::forward<T>(t)));
    }

    template<class T> requires requires { __impl(declval<T>(), __priority_tag<2>()); }
    [[nodiscard("empty() doesn't clear the range. It only checks")]] static constexpr bool
    operator()(T&& t) noexcept(noexcept(__impl(declval<T>(), __priority_tag<2>()))) {
        return __impl(std::forward<T>(t), __priority_tag<2>());
    }
};

struct __data_fn {
    template<class T> requires requires { auto(declval<T>().data()); } static constexpr auto
    __impl(T&& t, __priority_tag<1>) noexcept(noexcept(auto(declval<T>().data()))) {
        return auto(std::forward<T>(t).data());
    }

    template<class T> requires requires {
        { ranges::begin(declval<T>()) } -> contiguous_iterator;
    }
    static constexpr auto __impl(T&& t, __priority_tag<0>) noexcept(
        noexcept(std::to_address(ranges::begin(std::forward<T>(t))))
    ) {
        return std::to_address(ranges::begin(std::forward<T>(t)));
    }

    template<class T> requires(is_lvalue_reference_v<T> || enable_borrowed_range<remove_cv_t<T>>)
                           && requires { __impl(declval<T>(), __priority_tag<1>()); }
    static constexpr remove_reference_t<range_reference_t<T>>*
    operator()(T&& t) noexcept(noexcept(__impl(declval<T>(), __priority_tag<1>()))) {
        return __impl(std::forward<T>(t), __priority_tag<1>());
    }
};
}  // namespace __detail

inline namespace __cpo {
inline constexpr __detail::__empty_fn empty;
inline constexpr __detail::__data_fn data;

inline constexpr auto cdata = []<class T>
    requires(is_lvalue_reference_v<T> || enable_borrowed_range<remove_cv_t<T>>)
             && requires { data(declval<T>()); }(T && t) static noexcept(noexcept(
        __detail::as_const_pointer(data(__detail::possibly_const_range(std::forward<T>(t))))
    )) -> remove_reference_t<range_const_reference_t<T>>* {
        return __detail::as_const_pointer(data(__detail::possibly_const_range(std::forward<T>(t))));
    };
}  // namespace __cpo

template<class T> concept sized_range = ranges::range<T> && requires(T& t) { ranges::size(t); };
template<class T>
concept approximately_sized_range = ranges::range<T> && requires(T& t) { ranges::reserve_hint(t); };
template<sized_range R> using range_size_t = decltype(ranges::size(std::declval<R&>()));
template<range R> using range_difference_t = iter_difference_t<iterator_t<R>>;
template<range R> using range_value_t      = iter_value_t<iterator_t<R>>;

template<class T> concept view = range<T> && movable<T> && enable_view<T>;
template<class R>
concept borrowed_range =
    range<R> && (is_lvalue_reference_v<R> || enable_borrowed_range<remove_cvref_t<R>>);

template<class T>
concept contiguous_range =
    random_access_range<T> && contiguous_iterator<iterator_t<T>> && requires(T& t) {
        { ranges::data(t) } -> same_as<add_pointer_t<range_reference_t<T>>>;
    };

template<class T> concept common_range = range<T> && same_as<iterator_t<T>, sentinel_t<T>>;

template<class T>
concept viewable_range = range<T>
                      && ((view<remove_cvref_t<T>> && constructible_from<remove_cvref_t<T>, T>)
                          || (!view<remove_cvref_t<T>>
                              && (is_lvalue_reference_v<T>
                                  || (movable<remove_reference_t<T>>
                                      && !__is_specialization_of_v<initializer_list, T>))));

template<class R, class T> concept output_range = range<R> && output_iterator<iterator_t<R>, T>;

struct dangling {
    constexpr dangling() = default;
    constexpr dangling(auto&&...) noexcept {}
};

template<range R>
using borrowed_iterator_t = conditional_t<borrowed_range<R>, iterator_t<R>, dangling>;

enum class subrange_kind : bool { unsized, sized };

using std::from_range;
using std::from_range_t;

namespace __detail {
struct __prev_fn {
    template<bidirectional_iterator I> constexpr I operator()(I i) const {
        --i;
        return i;
    }

    template<bidirectional_iterator I> constexpr I operator()(I i, iter_difference_t<I> n) const {
        ranges::advance(i, -n);
        return i;
    }

    template<bidirectional_iterator I>
    constexpr I operator()(I i, iter_difference_t<I> n, I bound) const {
        ranges::advance(i, -n, bound);
        return i;
    }
};
struct __next_fn {
    template<input_or_output_iterator I> constexpr I operator()(I i) const {
        ++i;
        return i;
    }

    template<input_or_output_iterator I> constexpr I operator()(I i, iter_difference_t<I> n) const {
        ranges::advance(i, n);
        return i;
    }

    template<input_or_output_iterator I, sentinel_for<I> S>
    constexpr I operator()(I i, S bound) const {
        ranges::advance(i, bound);
        return i;
    }

    template<input_or_output_iterator I, sentinel_for<I> S>
    constexpr I operator()(I i, iter_difference_t<I> n, S bound) const {
        ranges::advance(i, n, bound);
        return i;
    }
};

struct __distance_fn {
    template<class I, sentinel_for<I> S> requires(!sized_sentinel_for<S, I>)
    constexpr iter_difference_t<I> operator()(I first, S last) const {
        iter_difference_t<I> result = 0;
        while (first != last) {
            ++first;
            ++result;
        }
        return result;
    }

    template<class I, sized_sentinel_for<decay_t<I>> S>
    constexpr iter_difference_t<I> operator()(I const& first, S last) const {
        return last - first;
    }

    template<range R> constexpr range_difference_t<R> operator()(R&& r) const {
        if constexpr (sized_range<remove_cvref_t<R>>)
            return static_cast<range_difference_t<R>>(ranges::size(r));
        else
            return (*this)(ranges::begin(r), ranges::end(r));
    }
};

}  // namespace __detail

inline namespace __cpo {
inline constexpr __detail::__next_fn next;
inline constexpr __detail::__prev_fn prev;
inline constexpr __detail::__distance_fn distance;
}  // namespace __cpo

template<class D> requires is_class_v<D> && same_as<D, remove_cv_t<D>> class view_interface {
    constexpr D& derived() & noexcept { return static_cast<D&>(*this); }
    constexpr D&& derived() && noexcept { return static_cast<D&&>(*this); }
    constexpr D const& derived() const& noexcept { return static_cast<D const&>(*this); }
    constexpr D const&& derived() const&& noexcept { return static_cast<D const&&>(*this); }

public:
    constexpr bool empty() requires sized_range<D> || forward_range<D> {
        if constexpr (sized_range<D>) {
            return ranges::size(derived()) == 0;
        } else {
            return ranges::begin(derived()) == ranges::end(derived());
        }
    }
    constexpr bool empty() const requires sized_range<D const> || forward_range<D const> {
        if constexpr (sized_range<D>) {
            return ranges::size(derived()) == 0;
        } else {
            return ranges::begin(derived()) == ranges::end(derived());
        }
    }

    constexpr auto cbegin() { return ranges::cbegin(derived()); }
    constexpr auto cbegin() const requires range<D const> { return ranges::cbegin(derived()); }
    constexpr auto cend() { return ranges::cend(derived()); }
    constexpr auto cend() const requires range<D const> { return ranges::cend(derived()); }

    constexpr explicit operator bool() requires requires { ranges::empty(derived()); } {
        return !ranges::empty(derived());
    }
    constexpr explicit operator bool() const requires requires { ranges::empty(derived()); } {
        return !ranges::empty(derived());
    }

    constexpr auto data() requires contiguous_iterator<iterator_t<D>> {
        return std::to_address(ranges::begin(derived()));
    }
    constexpr auto data() const requires range<D const> && contiguous_iterator<iterator_t<D const>>
    {
        return std::to_address(ranges::begin(derived()));
    }

    constexpr auto size()
        requires forward_range<D> && sized_sentinel_for<sentinel_t<D>, iterator_t<D>> {
        return __detail::__to_unsigned_like(ranges::end(derived()) - ranges::begin(derived()));
    }
    constexpr auto size() const
        requires forward_range<D const>
              && sized_sentinel_for<sentinel_t<D const>, iterator_t<D const>> {
        return __detail::__to_unsigned_like(ranges::end(derived()) - ranges::begin(derived()));
    }

    constexpr decltype(auto) front() requires forward_range<D> {
        assert(!derived().empty() && "front on empty range is undefined");
        return *ranges::begin(derived());
    }
    constexpr decltype(auto) front() const requires forward_range<D const> {
        assert(!derived().empty() && "front on empty range is undefined");
        return *ranges::begin(derived());
    }

    constexpr decltype(auto) back() requires bidirectional_range<D> && common_range<D> {
        return *ranges::prev(ranges::end(derived()));
    }
    constexpr decltype(auto) back() const
        requires bidirectional_range<D const> && common_range<D const> {
        return *ranges::prev(ranges::end(derived()));
    }

    template<random_access_range R = D>
    constexpr decltype(auto) operator[](range_difference_t<R> n) {
        return ranges::begin(derived())[n];
    }
    template<random_access_range R = D const>
    constexpr decltype(auto) operator[](range_difference_t<R> n) const {
        return ranges::begin(derived())[n];
    }

#if _STD_HAS_EH
    template<random_access_range R = D> requires sized_range<R>
    constexpr decltype(auto) at(range_difference_t<R> n);

    template<random_access_range R = D const> requires sized_range<R>
    constexpr decltype(auto) at(range_difference_t<R> n) const;
#else
    template<random_access_range R = D> requires sized_range<R>
    constexpr decltype(auto) at(range_difference_t<R> n) = _DELETE_NO_EXCEPTIONS;

    template<random_access_range R = D const> requires sized_range<R>
    constexpr decltype(auto) at(range_difference_t<R> n) const = _DELETE_NO_EXCEPTIONS;
#endif
};

template<range R> requires is_object_v<R> class ref_view: public view_interface<ref_view<R>> {
    static void _FUN(R&);
    static void _FUN(R&&) = delete;

public:
    template<__different_from<ref_view> T> requires convertible_to<T, R&> && requires {
        _FUN(declval<T>());
    } constexpr ref_view(T&& t) noexcept: r_(std::addressof(static_cast<R&>(std::forward<T>(t)))) {}

    constexpr R& base() const { return *r_; }
    constexpr iterator_t<R> begin() const { return ranges::begin(*r_); }
    constexpr sentinel_t<R> end() const { return ranges::end(*r_); }
    constexpr bool empty() const requires requires { ranges::empty(declval<R const&>()); } {
        return ranges::empty(*r_);
    }
    constexpr auto size() const requires sized_range<R> { return ranges::size(*r_); }
    constexpr auto data() const requires contiguous_range<R> { return ranges::data(*r_); }

private:
    R* r_;
};

template<class R> ref_view(R&) -> ref_view<R>;

template<class T> inline constexpr bool enable_borrowed_range<ref_view<T>> = true;

template<range R>
requires movable<R>
      && (!__is_specialization_of_v<initializer_list, remove_cvref_t<R>>)class owning_view
    : public view_interface<R> {
public:
    owning_view() requires default_initializable<R> = default;
    owning_view(owning_view&&)                      = default;
    owning_view(owning_view const&)                 = delete;
    constexpr owning_view(R&& t) noexcept(is_nothrow_move_constructible_v<R>): r_(std::move(t)) {}

    constexpr R& base() & noexcept { return r_; }
    constexpr R& base() && noexcept { return std::move(r_); }
    constexpr R const& base() const& noexcept { return r_; }
    constexpr R const& base() const&& noexcept { return std::move(r_); }

    constexpr iterator_t<R> begin() { return ranges::begin(r_); }
    constexpr auto begin() const requires range<R const> { return ranges::begin(r_); }
    constexpr sentinel_t<R> end() { return ranges::end(r_); }
    constexpr auto end() const requires range<R const> { return ranges::end(r_); }
    constexpr bool empty() requires requires { ranges::empty(declval<R&>()); } {
        return ranges::empty(r_);
    }
    constexpr bool empty() const requires requires { ranges::empty(declval<R const&>()); } {
        return ranges::empty(r_);
    }
    constexpr auto size() requires sized_range<R> { return ranges::size(r_); }
    constexpr auto size() const requires sized_range<R const> { return ranges::size(r_); }
    constexpr auto data() requires contiguous_range<R> { return ranges::data(r_); }
    constexpr auto data() const requires contiguous_range<R const> { return ranges::data(r_); }

private:
    R r_;
};

template<class T>

constexpr bool enable_borrowed_range<owning_view<T>> = enable_borrowed_range<T>;

template<class D> requires is_object_v<D> && same_as<D, remove_cv_t<D>>
class range_adaptor_closure {};
template<class Fn>
struct __range_adaptor_closure_fn: Fn, range_adaptor_closure<__range_adaptor_closure_fn<Fn>> {
    constexpr explicit __range_adaptor_closure_fn(Fn&& fn): Fn(std::move(fn)) {}
};

template<class T> T __derive_from_rac(range_adaptor_closure<T>*);

template<class T>
concept __range_adaptor_closure = !range<remove_cvref_t<T>> && requires {
    { __derive_from_rac((remove_cvref_t<T>*)nullptr) } -> same_as<remove_cvref_t<T>>;
};

template<range R, __range_adaptor_closure C> requires invocable<C, R>
constexpr auto operator|(R&& r, C&& c) noexcept(is_nothrow_invocable_v<C, R>) {
    return std::invoke(std::forward<C>(c), std::forward<R>(r));
}

template<__range_adaptor_closure C1, __range_adaptor_closure C2>
constexpr auto operator|(C1&& c1, C2&& c2) {
    return __range_adaptor_closure_fn([_c1 = std::forward<C1>(c1),
                                       _c2 = std::forward<C2>(c2)](auto&& x) {
        return std::invoke(
            std::forward<C2>(_c2), std::invoke(std::forward<C1>(_c1), std::forward<decltype(x)>(x))
        );
    });
}

struct __range_adaptor {
    template<class Self, class... As> requires requires {
        { remove_reference_t<Self>::__rac_argc } -> convertible_to<size_t>;
        requires remove_reference_t<Self>::__rac_argc == sizeof...(As) + 1;
    } constexpr auto operator()(this Self&& self, As&&... as) {
        return __range_adaptor_closure_fn(
            std::bind_back(std::forward<Self>(self), std::forward<As>(as)...)
        );
    }
};

namespace views {
inline constexpr struct __all_fn: range_adaptor_closure<__all_fn> {
    template<class E> requires view<decay_t<E>>
    static constexpr decay_t<E> operator()(E&& e) noexcept(noexcept(auto(std::forward<E>(e)))) {
        return auto(std::forward<E>(e));
    }

    template<class E> requires(!view<decay_t<E>>) && requires { ref_view(declval<E>()); }
    static constexpr auto operator()(E&& e) noexcept {
        return ref_view(std::forward<E>(e));
    }
    template<class E> requires(!view<decay_t<E>>) && (!requires { ref_view(declval<E>()); })
    static constexpr auto operator()(E&& e) noexcept(noexcept(owning_view(declval<E>()))) {
        return owning_view(std::forward<E>(e));
    }

} all;

template<viewable_range E> using all_t = decltype(views::all(declval<E>()));

}  // namespace views

namespace __detail {
template<class I>
using __iota_diff_t = decltype([] {
    if constexpr (!integral<I> || (integral<I> && requires {
                      typename iter_difference_t<I>;
                      requires sizeof(iter_difference_t<I>) > sizeof(I);
                  }))
        return iter_difference_t<I>();
    else {
        constexpr auto sz = sizeof(I);
        if constexpr (sz < sizeof(int16_t))
            return int16_t();
        else if constexpr (sz < sizeof(int32_t))
            return int32_t();
#ifdef __SIZEOF_INT128__
        else if constexpr (sz < sizeof(int64_t))
            return int64_t();
        else
            return __int128_t();
#else
        return uint64_t();
#endif
    }
#ifdef __SIZEOF_INT128__
    static_assert(sizeof(I) <= sizeof(__int128_t));
#else
    static_assert(sizeof(I) <= sizeof(uint64_t));
#endif
    // TODO was 128-bit mandated?
}());

template<class R>
concept __simple_view = view<R> && range<R const> && same_as<iterator_t<R>, iterator_t<R const>>
                     && same_as<sentinel_t<R>, sentinel_t<R const>>;
template<class I>
concept __has_arrow = input_iterator<I> && (is_pointer_v<I> || requires(I i) { i.operator->(); });
template<class R>
concept __range_with_movable_references = input_range<R> && move_constructible<range_reference_t<R>>
                                       && move_constructible<range_rvalue_reference_t<R>>;
template<bool C, class... Views>
concept __all_random_access = (random_access_range<conditional_t<C, Views const, Views>> && ...);
template<bool C, class... Views>
concept __all_bidirectional = (bidirectional_range<conditional_t<C, Views const, Views>> && ...);
template<bool C, class... Views>
concept __all_forward = (forward_range<conditional_t<C, Views const, Views>> && ...);

template<class R, class T>
concept container_compatible_range =
    ranges::input_range<R> && convertible_to<ranges::range_reference_t<R>, T>;

}  // namespace __detail

template<class T> requires is_object_v<T> class empty_view: public view_interface<empty_view<T>> {
public:
using __value_type = T;
    static constexpr T* begin() noexcept { return nullptr; }
    static constexpr T* end() noexcept { return nullptr; }
    static constexpr T* data() noexcept { return nullptr; }
    static constexpr size_t size() noexcept { return 0uz; }
    static constexpr bool empty() noexcept { return true; }
};

  template<class T>
    constexpr bool enable_borrowed_range<empty_view<T>> = true;

namespace views {
template<class T> inline constexpr empty_view<T> empty;
}

}  // namespace std::ranges
