#pragma once
// code: language=c++
// IWYU pragma: private: include <iterator>

#include <__functional/core.hpp>
#include <__memory/base.hpp>

#include <concepts>
#include <cstddef>
#include <type_traits>

#include <__iterator/iter_move.hpp>

namespace std {

template<class I> struct incrementable_traits {};

template<class T> requires is_object_v<T> struct incrementable_traits<T*> {
    using difference_type = ptrdiff_t;
};

template<class T> struct incrementable_traits<T const>: incrementable_traits<T> {};
template<class T> requires requires { typename T::difference_type; }
struct incrementable_traits<T> {
    using difference_type = T::difference_type;
};

template<class T>
requires (!requires {
    typename T::difference_type;} && requires(T const& a, T const& b) {
    { a - b } -> integral;
})
struct incrementable_traits<T> {
    using difference_type = make_signed_t<decltype(declval<T>() - declval<T>())>;
};

// indirectly_readable_traits
namespace __detail {

template<class> struct __cond_value_type {};
template<class T> requires is_object_v<T> struct __cond_value_type<T> {
    using value_type = remove_cv_t<T>;
};
template<class T> concept __has_member_vt = requires { typename T::value_type; };
template<class T> concept __has_member_et = requires { typename T::element_type; };

}  // namespace __detail

template<class I> struct indirectly_readable_traits {};
template<class T> struct indirectly_readable_traits<T*>: __detail::__cond_value_type<T> {};
template<class I> requires is_array_v<I> struct indirectly_readable_traits<I> {
    using value_type = remove_cv_t<remove_extent_t<I>>;
};
template<class T> struct indirectly_readable_traits<T const>: indirectly_readable_traits<T> {};
template<__detail::__has_member_vt T>
struct indirectly_readable_traits<T>: __detail::__cond_value_type<typename T::value_type> {};
template<__detail::__has_member_et T>
struct indirectly_readable_traits<T>: __detail::__cond_value_type<typename T::element_type> {};
template<__detail::__has_member_vt T>
requires __detail::__has_member_et<T> && same_as<typename T::value_type, typename T::element_type>
struct indirectly_readable_traits<T>: __detail::__cond_value_type<typename T::value_type> {};

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag: input_iterator_tag {};
struct bidirectional_iterator_tag: forward_iterator_tag {};
struct random_access_iterator_tag: bidirectional_iterator_tag {};
struct contiguous_iterator_tag: random_access_iterator_tag {};

template<class T> struct iterator_traits {
    using __primary_template = void;
};

namespace __detail {

template<class T>
inline constexpr bool __is_iter_traits_primary = requires { typename T::__primary_template; };

template<class T> concept __signed_integer_like = signed_integral<T>;
template<class T> concept __integer_like        = integral<T> && !same_as<T, bool>;
template<class T> constexpr auto __to_unsigned_like(T x) noexcept {
    return make_unsigned_t<T>(x);
}
template<class T> constexpr auto __to_signed_like(T x) noexcept {
    return make_signed_t<T>(x);
}
template<class T> using __make_signed_like   = make_signed_t<T>;
template<class T> using __make_unsigned_like = make_unsigned_t<T>;

template<class T>
concept __dereferenceable = requires(T& t) {
    { *t } -> __referenceable;
};

}  // namespace __detail

template<class T>
using iter_value_t = conditional_t<
    __detail::__is_iter_traits_primary<iterator_traits<remove_cvref_t<T>>>,
    indirectly_readable_traits<remove_cvref_t<T>>, iterator_traits<remove_cvref_t<T>>>::value_type;

template<__detail::__dereferenceable T> using iter_reference_t = decltype(*declval<T&>());
template<__detail::__dereferenceable T> requires requires(T& t) {
    { ranges::iter_move(t) } -> __detail::__referenceable;
} using iter_rvalue_reference_t = decltype(ranges::iter_move(std::declval<T&>()));

namespace __detail {
template<class T>
concept __indirectly_readable_impl =
    requires(T const in) {
        typename iter_value_t<T>;
        typename iter_reference_t<T>;
        typename iter_rvalue_reference_t<T>;
        { *in } -> same_as<iter_reference_t<T>>;
        { ranges::iter_move(in) } -> same_as<iter_rvalue_reference_t<T>>;
    } && common_reference_with<iter_reference_t<T>&&, iter_value_t<T>&>
    && common_reference_with<iter_reference_t<T>&&, iter_rvalue_reference_t<T>&&>
    && common_reference_with<iter_rvalue_reference_t<T>&&, iter_value_t<T> const&>;
}  // namespace __detail

template<class T>
concept indirectly_readable = __detail::__indirectly_readable_impl<remove_cvref_t<T>>;

template<indirectly_readable T>
using iter_const_reference_t =
    common_reference_t<std::iter_value_t<T> const&&, iter_reference_t<T>>;

template<class T>
using iter_difference_t = conditional_t<
    __detail::__is_iter_traits_primary<iterator_traits<remove_cvref_t<T>>>,
    incrementable_traits<remove_cvref_t<T>>, iterator_traits<remove_cvref_t<T>>>::difference_type;

template<class Out, class T>
concept indirectly_writable = requires(Out&& o, T&& t) {
    *o                                                               = std::forward<T>(t);
    *std::forward<Out>(o)                                            = std::forward<T>(t);
    const_cast<iter_reference_t<Out> const&&>(*o)                    = std::forward<T>(t);
    const_cast<iter_reference_t<Out> const&&>(*std::forward<Out>(o)) = std::forward<T>(t);
};

template<class I>
concept weakly_incrementable = movable<I> && requires(I i) {
    typename iter_difference_t<I>;
    requires __detail::__signed_integer_like<std::iter_difference_t<I>>;
    { ++i } -> std::same_as<I&>;  // not required to be equality-preserving
    i++;                          // not required to be equality-preserving
};

template<class I>
concept incrementable = regular<I> && weakly_incrementable<I> && requires(I i) {
    { i++ } -> same_as<I>;
};

template<class I>
concept input_or_output_iterator = requires(I i) {
    { *i } -> __detail::__referenceable;
} && weakly_incrementable<I>;

template<class S, class I>
concept sentinel_for = semiregular<S> && input_or_output_iterator<I>
                    && __detail::__weakly_equality_comparable_with<I, S>;

template<class S, class I> inline constexpr bool disable_sized_sentinel_for = false;

template<class S, class I>
concept sized_sentinel_for =
    sentinel_for<S, I> && !disable_sized_sentinel_for<remove_cv_t<S>, remove_cv_t<I>>
    && requires(I const& i, S const& s) {
           { s - i } -> same_as<iter_difference_t<I>>;
           { i - s } -> same_as<iter_difference_t<I>>;
       };

namespace __detail {

template<class T>
using __iter_traits =
    conditional_t<__is_iter_traits_primary<iterator_traits<T>>, T, iterator_traits<T>>;

template<class T>
using __iter_concept = decltype([] {
    if constexpr (requires { typename __iter_traits<T>::iterator_concept; }) {
        return type_identity<typename __iter_traits<T>::iterator_concept>();
    } else if constexpr (requires { typename __iter_traits<T>::iterator_category; }) {
        return type_identity<typename T::iterator_category>();
    } else {
        return conditional<
            __is_iter_traits_primary<iterator_traits<T>>, random_access_iterator_tag, __empty>();
    }
}())::type;

}  // namespace __detail

template<class I>
concept input_iterator = input_or_output_iterator<I> && indirectly_readable<I>
                      && requires { typename __detail::__iter_concept<I>; };

template<class I, class T>
concept output_iterator = input_or_output_iterator<I> && indirectly_writable<I, T>
                       && requires(I i, T&& t) { *i++ = std::forward<T>(t); };

template<class I>
concept forward_iterator =
    input_iterator<I> && derived_from<__detail::__iter_concept<I>, forward_iterator_tag>
    && incrementable<I> && sentinel_for<I, I>;

template<class I>
concept bidirectional_iterator =
    forward_iterator<I> && derived_from<__detail::__iter_concept<I>, bidirectional_iterator_tag>
    && requires(I i) {
           { --i } -> same_as<I&>;
           { i-- } -> same_as<I>;
       };

template<class I>
concept random_access_iterator =
    bidirectional_iterator<I>
    && derived_from<__detail::__iter_concept<I>, random_access_iterator_tag> && totally_ordered<I>
    && sized_sentinel_for<I, I> && requires(I i, I const j, iter_difference_t<I> const n) {
           { i += n } -> same_as<I&>;
           { j + n } -> same_as<I>;
           { n + j } -> same_as<I>;
           { i -= n } -> same_as<I&>;
           { j - n } -> same_as<I>;
           { j[n] } -> same_as<iter_reference_t<I>>;
       };

template<class I>
concept contiguous_iterator =
    random_access_iterator<I> && derived_from<__detail::__iter_concept<I>, contiguous_iterator_tag>
    && is_lvalue_reference_v<iter_reference_t<I>>
    && same_as<iter_value_t<I>, remove_cvref_t<iter_reference_t<I>>> && requires(I const& i) {
           { std::to_address(i) } -> same_as<add_pointer_t<iter_reference_t<I>>>;
       };

namespace __detail {

template<class Iter>
concept __iter_traits_4 = requires {
    typename Iter::difference_type;
    typename Iter::value_type;
    typename Iter::reference;
    typename Iter::iterator_category;
};

template<class Iter>
concept __LegacyIterator = requires(Iter i) {
    { *i } -> __referenceable;
    { ++i } -> std::same_as<Iter&>;
    { *++i } -> __referenceable;
} && copyable<Iter>;

template<class Iter>
concept __LegacyInputIterator =
    __LegacyIterator<Iter> && equality_comparable<Iter> && requires(Iter i) {
        typename incrementable_traits<Iter>::difference_type;
        typename indirectly_readable_traits<Iter>::value_type;
        typename common_reference_t<
            iter_reference_t<Iter>&&, typename indirectly_readable_traits<Iter>::value_type&>;
        *i++;
        typename common_reference_t<
            decltype(*i++)&&, typename indirectly_readable_traits<Iter>::value_type&>;
        requires signed_integral<typename incrementable_traits<Iter>::difference_type>;
    };

template<class It>
concept __LegacyForwardIterator =
    __LegacyInputIterator<It> && constructible_from<It> && is_reference_v<iter_reference_t<It>>
    && same_as<
        remove_cvref_t<iter_reference_t<It>>, typename indirectly_readable_traits<It>::value_type>
    && requires(It it) {
           { it++ } -> convertible_to<It const&>;
           { *it++ } -> same_as<iter_reference_t<It>>;
       };

template<class I>

concept __LegacyBidirectionalIterator = __LegacyForwardIterator<I> && requires(I i) {
    { --i } -> same_as<I&>;
    { i-- } -> convertible_to<I const&>;
    { *i-- } -> same_as<iter_reference_t<I>>;
};

template<class I>
concept __LegacyRandomAccessIterator =
    __LegacyBidirectionalIterator<I> && totally_ordered<I>
    && requires(I i, typename incrementable_traits<I>::difference_type n) {
           { i += n } -> same_as<I&>;
           { i -= n } -> same_as<I&>;
           { i + n } -> same_as<I>;
           { n + i } -> same_as<I>;
           { i - n } -> same_as<I>;
           { i - i } -> same_as<decltype(n)>;
           { i[n] } -> convertible_to<iter_reference_t<I>>;
       };

}  // namespace __detail

template<class I> requires __detail::__iter_traits_4<I> && requires { typename I::pointer; }
struct iterator_traits<I> {
    using __primary_template = void;
    using value_type         = I::value_type;
    using difference_type    = I::difference_type;
    using pointer            = I::pointer;
    using reference          = I::reference;
    using iterator_category  = I::iterator_category;
};

template<class I> requires __detail::__iter_traits_4<I> && (!requires { typename I::pointer; })
struct iterator_traits<I> {
    using __primary_template = void;
    using value_type         = I::value_type;
    using difference_type    = I::difference_type;
    using pointer            = void;
    using reference          = I::reference;
    using iterator_category  = I::iterator_category;
};

template<class I> requires(!__detail::__iter_traits_4<I>) && __detail::__LegacyInputIterator<I>
struct iterator_traits<I> {
    using __primary_template = void;
    using difference_type    = incrementable_traits<I>::difference_type;
    using value_type         = indirectly_readable_traits<I>::value_type;
    using pointer            = decltype([] consteval {
        if constexpr (requires { typename I::pointer; }) {
            return type_identity<typename I::pointer>();
        } else if constexpr (requires(I& i) { i.operator->(); }) {
            return type_identity<decltype(declval<I&>().operator->())>();
        } else {
            return type_identity<void>();
        }
    }())::type;
    using reference          = decltype([] consteval {
        if constexpr (requires { typename I::reference; }) {
            return type_identity<typename I::reference>();
        } else {
            return type_identity<iter_reference_t<I>>();
        }
    }())::type;

    using iterator_category = conditional_t<
        __detail::__LegacyRandomAccessIterator<I>, random_access_iterator_tag,
        conditional_t<
            __detail::__LegacyBidirectionalIterator<I>, bidirectional_iterator_tag,
            conditional_t<
                __detail::__LegacyForwardIterator<I>, forward_iterator_tag, input_iterator_tag>>>;
};

template<class I> requires(!__detail::__iter_traits_4<I>) && __detail::__LegacyIterator<I>
                       && (!__detail::__LegacyInputIterator<I>)struct iterator_traits<I> {
    using __primary_template = void;
    using value_type         = void;
    using pointer            = void;
    using reference          = void;
    using iterator_category  = output_iterator_tag;

    using difference_type = decltype([] consteval {
        if constexpr (requires { typename incrementable_traits<I>::difference_type; }) {
            return type_identity<typename incrementable_traits<I>::difference_type>();
        } else {
            return type_identity<void>();
        }
    }())::type;
};

template<class T> requires is_object_v<T> struct iterator_traits<T*> {
    using difference_type    = ptrdiff_t;
    using value_type         = remove_cv_t<T>;
    using pointer            = T*;
    using reference          = T&;
    using iterator_category  = random_access_iterator_tag;
    using iterator_concept   = contiguous_iterator_tag;
};

template<class In, class Out>
concept indirectly_movable =
    indirectly_readable<In> && indirectly_writable<Out, iter_rvalue_reference_t<In>>;

template<class In, class Out>
concept indirectly_movable_storable =
    indirectly_movable<In, Out> && indirectly_writable<Out, iter_value_t<In>>
    && movable<iter_value_t<In>>
    && constructible_from<iter_value_t<In>, iter_rvalue_reference_t<In>>
    && assignable_from<iter_value_t<In>&, iter_rvalue_reference_t<In>>;

template<class In, class Out>
concept indirectly_copyable =
    indirectly_readable<In> && indirectly_writable<Out, iter_reference_t<In>>;

template<class In, class Out>

concept indirectly_copyable_storable =
    indirectly_copyable<In, Out> && indirectly_writable<Out, iter_value_t<In>&>
    && indirectly_writable<Out, iter_value_t<In> const&>
    && indirectly_writable<Out, iter_value_t<In>&&>
    && indirectly_writable<Out, iter_value_t<In> const&&> && copyable<iter_value_t<In>>
    && constructible_from<iter_value_t<In>, iter_reference_t<In>>
    && assignable_from<iter_value_t<In>&, iter_reference_t<In>>;

template<class F, class... Is>
requires(indirectly_readable<Is> && ...) && invocable<F, iter_reference_t<Is>...>
using indirect_result_t = invoke_result_t<F, iter_reference_t<Is>...>;

namespace __detail {

template<class I, class Proj> struct __projected_impl {
    struct __type {
        using value_type = remove_cvref_t<indirect_result_t<Proj&, I>>;
        indirect_result_t<Proj&, I> operator*() const;
    };
};
template<weakly_incrementable I, class Proj> struct __projected_impl<I, Proj> {
    struct __type {
        using value_type      = remove_cvref_t<indirect_result_t<Proj&, I>>;
        using difference_type = iter_difference_t<I>;
        indirect_result_t<Proj&, I> operator*() const;
    };
};

template<class T> inline constexpr bool is_projected                                        = false;
template<class I, class Proj> inline constexpr bool is_projected<__projected_impl<I, Proj>> = true;

template<class T> struct __indirect_value {
    using type = iter_value_t<T>&;
};
template<class I, class Proj> struct __indirect_value<__projected_impl<I, Proj>> {
    using type = invoke_result_t<Proj&, typename __indirect_value<I>::type>;
};
template<class T> using __indirect_value_t = __indirect_value<T>::type;

}  // namespace __detail

template<class F, class I>
concept indirectly_unary_invocable =
    indirectly_readable<I> && copy_constructible<F>
    && invocable<F&, __detail::__indirect_value_t<I>> && invocable<F&, iter_reference_t<I>>
    && common_reference_with<
        invoke_result_t<F&, __detail::__indirect_value_t<I>>,
        invoke_result_t<F&, iter_reference_t<I>>>;

template<class F, class I>
concept indirectly_regular_unary_invocable =
    indirectly_readable<I> && copy_constructible<F>
    && regular_invocable<F&, __detail::__indirect_value_t<I>>
    && regular_invocable<F&, iter_reference_t<I>>
    && common_reference_with<
        invoke_result_t<F&, __detail::__indirect_value_t<I>>,
        invoke_result_t<F&, iter_reference_t<I>>>;

template<class F, class I>
concept indirect_unary_predicate =
    indirectly_readable<I> && copy_constructible<F>
    && predicate<F&, __detail::__indirect_value_t<I>> && predicate<F&, iter_reference_t<I>>;

template<class F, class I1, class I2>

concept indirect_binary_predicate =
    indirectly_readable<I1> && indirectly_readable<I2> && copy_constructible<F>
    && predicate<F&, __detail::__indirect_value_t<I1>, __detail::__indirect_value_t<I2>>
    && predicate<F&, __detail::__indirect_value_t<I1>, iter_reference_t<I2>>
    && predicate<F&, iter_reference_t<I1>, __detail::__indirect_value_t<I2>>
    && predicate<F&, iter_reference_t<I1>, iter_reference_t<I2>>;

template<class F, class I1, class I2 = I1>
concept indirect_equivalence_relation =
    indirectly_readable<I1> && indirectly_readable<I2> && copy_constructible<F>
    && equivalence_relation<F&, __detail::__indirect_value_t<I1>, __detail::__indirect_value_t<I2>>
    && equivalence_relation<F&, __detail::__indirect_value_t<I1>, iter_reference_t<I2>>
    && equivalence_relation<F&, iter_reference_t<I1>, __detail::__indirect_value_t<I2>>
    && equivalence_relation<F&, iter_reference_t<I1>, iter_reference_t<I2>>;

template<class F, class I1, class I2 = I1>
concept indirect_strict_weak_order =
    indirectly_readable<I1> && indirectly_readable<I2> && copy_constructible<F>
    && strict_weak_order<F&, __detail::__indirect_value_t<I1>, __detail::__indirect_value_t<I2>>
    && strict_weak_order<F&, __detail::__indirect_value_t<I1>, iter_reference_t<I2>>
    && strict_weak_order<F&, iter_reference_t<I1>, __detail::__indirect_value_t<I2>>
    && strict_weak_order<F&, iter_reference_t<I1>, iter_reference_t<I2>>;

template<indirectly_readable I, indirectly_regular_unary_invocable<I> Proj>
using projected = __detail::__projected_impl<I, Proj>::__type;

template<indirectly_readable I, indirectly_regular_unary_invocable<I> Proj>
using projected_value_t = remove_cvref_t<invoke_result_t<Proj, iter_value_t<I>&>>;

template<
    class I1, class I2, class Out, class Comp = ranges::less, class Proj1 = identity,
    class Proj2 = identity>
concept mergeable = input_iterator<I1> && input_iterator<I2> && weakly_incrementable<Out>
                 && indirectly_copyable<I1, Out> && indirectly_copyable<I2, Out>
                 && indirect_strict_weak_order<Comp, projected<I1, Proj1>, projected<I2, Proj2>>;

struct default_sentinel_t {};
inline constexpr default_sentinel_t default_sentinel;

struct unreachable_sentinel_t {
    template<weakly_incrementable I>
    friend constexpr bool operator==(unreachable_sentinel_t, I const&) noexcept {
        return false;
    }
};
inline constexpr unreachable_sentinel_t unreachable_sentinel;

template<indirectly_readable T>
using iter_common_reference_t =
    common_reference_t<iter_reference_t<T>, __detail::__indirect_value_t<T>>;

template<
    class I1, class I2, class Comp,

    class Proj1 = identity, class Proj2 = identity>
concept indirectly_comparable =
    indirect_binary_predicate<Comp, projected<I1, Proj1>, projected<I2, Proj2>>;

}  // namespace std
