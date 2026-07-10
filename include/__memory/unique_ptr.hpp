#pragma once
// code: language=c++
// IWYU pragma: private: include <memory>

#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

namespace std {

template<class T> struct default_delete {
    default_delete() = default;
    template<class U> requires convertible_to<U*, T*>
    constexpr default_delete(default_delete<U> const&) noexcept {}

    constexpr void operator()(T* ptr) const noexcept {
        static_assert(sizeof(T) > 0, "Calling delete on incomplete type is undefined");
        delete ptr;
    }
};
template<class T> struct default_delete<T[]> {
    default_delete() = default;
    template<class U> requires convertible_to<U (*)[], T (*)[]>
    constexpr default_delete(default_delete<U[]> const&) noexcept {}

    template<class U> requires convertible_to<U (*)[], T (*)[]>
    constexpr void operator()(U* ptr) const noexcept {
        static_assert(sizeof(U) > 0, "Calling delete on incomplete type is undefined");
        delete[] ptr;
    }
};

namespace __detail {}  // namespace __detail

template<class T_, class Del = default_delete<T_>> class unique_ptr {
    static constexpr bool array_form = is_unbounded_array_v<T_>;
    using T                          = remove_extent_t<T_>;

    static constexpr bool __deleter_default_init = default_initializable<Del> && !is_pointer_v<Del>;

    template<class U, class E> friend class unique_ptr;

public:
    using pointer      = decltype([] {
        if constexpr (requires { typename remove_reference_t<Del>::pointer; })
            return type_identity<typename remove_reference_t<Del>::pointer>();
        else
            return type_identity<T*>();
    }())::type;
    using element_type = T;
    using deleter_type = Del;
    static_assert(!is_rvalue_reference_v<Del>, "unique_ptr deleter cannot be an rvalue-ref");

private:
    template<class U>
    static constexpr bool __not_array_narrowing =
        same_as<U, pointer> || same_as<U, nullptr_t>
        || (same_as<pointer, element_type*> && is_pointer_v<U>
            && is_convertible_v<remove_pointer_t<U> (*)[], element_type (*)[]>);

    using __overload_3_type = conditional_t<is_reference_v<Del>, Del, Del const&>;
    using __overload_4_type = conditional_t<is_reference_v<Del>, remove_reference_t<Del>&&, Del&&>;

public:
    constexpr pointer get() const noexcept { return ptr_; }
    constexpr Del& get_deleter() noexcept { return del_; }
    constexpr Del const& get_deleter() const noexcept { return del_; }
    constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }

    constexpr pointer release() noexcept { return std::exchange(ptr_, nullptr); }
    constexpr void reset(pointer ptr = pointer()) noexcept requires(!array_form) {
        auto old = std::exchange(ptr_, ptr);
        if (old) del_(old);
    }
    template<class U> requires array_form
                            && (same_as<U, pointer>
                                || (same_as<pointer, element_type*> && is_pointer_v<U>
                                    && convertible_to<remove_pointer_t<U> (*)[], T (*)[]>))
    constexpr void reset(U ptr) noexcept {
        auto old = std::exchange(ptr_, ptr);
        if (old) del_(old);
    }
    constexpr void reset(nullptr_t = nullptr) noexcept requires array_form {
        auto old = std::exchange(ptr_, nullptr);
        if (old) del_(old);
    }

    constexpr void swap(unique_ptr& o) noexcept {
        using std::swap;
        swap(ptr_, o.ptr_);
        swap(del_, o.del_);
    }

    constexpr add_lvalue_reference_t<T> operator*() const noexcept(noexcept(*declval<pointer&>()))
        requires(!array_form) {
        assert(ptr_ != nullptr);
        return *ptr_;
    }
    constexpr pointer operator->() const noexcept requires(!array_form) {
        assert(ptr_ != nullptr);
        return ptr_;
    }

    constexpr add_lvalue_reference_t<T> operator[](size_t i) const noexcept requires array_form {
        assert(ptr_ != nullptr);
        return ptr_[i];
    }

    constexpr ~unique_ptr() {
        if (ptr_) del_(ptr_);
    }

    constexpr unique_ptr() noexcept requires __deleter_default_init: ptr_(), del_() {}
    constexpr unique_ptr(nullptr_t) noexcept requires __deleter_default_init: unique_ptr() {}

    constexpr explicit unique_ptr(pointer p) noexcept
        requires __deleter_default_init && (!array_form)
        : ptr_(p), del_() {}

    // 3-4
    constexpr unique_ptr(pointer p, __overload_3_type d) noexcept requires(!array_form)
        : ptr_(p), del_(std::forward<decltype(d)>(d)) {}
    constexpr unique_ptr(pointer p, __overload_4_type d) noexcept
        requires(!array_form && !is_reference_v<Del>)
        : ptr_(p), del_(std::forward<decltype(d)>(d)) {}
    constexpr unique_ptr(pointer p, __overload_4_type d) noexcept
        requires(!array_form && is_reference_v<Del>) = delete;

    constexpr unique_ptr(unique_ptr&& o) noexcept requires is_move_constructible_v<Del>
        : ptr_(std::exchange(o.ptr_, nullptr)), del_([&o] -> decltype(auto) {
              if constexpr (is_reference_v<Del>)
                  return o.del_;
              else
                  return std::move(o.del_);
          }()) {}

    template<class U, class E>
    requires(!array_form
             && !is_array_v<U> && is_convertible_v<typename unique_ptr<U, E>::pointer, pointer>
             && (is_reference_v<Del> ? is_same_v<Del, E> : is_convertible_v<E, Del>))
    constexpr unique_ptr(unique_ptr<U, E>&& o) noexcept
        : ptr_(std::exchange(o.ptr_, nullptr)), del_([&o] -> decltype(auto) {
              if constexpr (is_reference_v<Del>)
                  return o.del_;
              else
                  return std::move(o.del_);
          }()) {}

    unique_ptr(unique_ptr const&) = delete;

    // Array constructors
    template<class U> requires __deleter_default_init && array_form && __not_array_narrowing<U>
    explicit constexpr unique_ptr(U p) noexcept: ptr_(std::move(p)), del_() {}

    // 3-4
    template<class U> requires array_form && __not_array_narrowing<U>
    constexpr unique_ptr(U p, __overload_3_type d) noexcept
        : ptr_(p), del_(std::forward<delctype(d)>(d)) {}
    template<class U>
    constexpr unique_ptr(U p, __overload_4_type d) noexcept
        requires(array_form && !is_reference_v<Del> && __not_array_narrowing<U>)
        : ptr_(p), del_(std::forward<delctype(d)>(d)) {}
    template<class U>
    constexpr unique_ptr(U p, __overload_4_type d) noexcept
        requires array_form && is_reference_v<Del> && __not_array_narrowing<U> = delete;

    template<class U, class E>
    requires(array_form && is_array_v<U> && is_same_v<element_type*, pointer>
             && is_same_v<
                 typename unique_ptr<U, E>::pointer, typename unique_ptr<U, E>::element_type*>
             && is_convertible_v<typename unique_ptr<U, E>::element_type (*)[], element_type (*)[]>
             && (is_reference_v<Del> ? is_same_v<Del, E> : is_convertible_v<E, Del>))
    constexpr unique_ptr(unique_ptr<U, E>&& o) noexcept
        : ptr_(std::exchange(o.ptr_, nullptr)), del_([&o] -> decltype(auto) {
              if constexpr (is_reference_v<Del>)
                  return o.del_;
              else
                  return std::move(o.del_);
          }()) {}

    constexpr unique_ptr& operator=(unique_ptr&& o) noexcept {
        if (this != &o) {
            reset(o.release());
            if constexpr (is_reference_v<Del>) {
                del_ = o.del_;
            } else {
                del_ = std::move(o.del_);
            }
        }
        return *this;
    }
    unique_ptr& operator=(unique_ptr const&) = delete;

    template<class U, class E> requires(
        is_assignable_v<Del&, E &&> && !array_form
        && !is_array_v<U> && is_convertible_v<typename unique_ptr<U, E>::pointer, pointer>
    )
    constexpr unique_ptr& operator=(unique_ptr<U, E>&& o) noexcept {
        reset(o.release());
        del_ = std::forward<Del>(o.del_);
        return *this;
    }
    template<class U, class E> requires(
        is_assignable_v<Del&, E &&> && array_form
        && is_array_v<U> && is_same_v<pointer, element_type*>
        && is_same_v<typename unique_ptr<U, E>::pointer, typename unique_ptr<U, E>::element_type*>
        && is_convertible_v<typename unique_ptr<U, E>::element_type (*)[], element_type (*)[]>
    )
    constexpr unique_ptr& operator=(unique_ptr<U, E>&& o) noexcept {
        reset(o.release());
        del_ = std::forward<Del>(o.del_);
        return *this;
    }
    constexpr unique_ptr& operator=(nullptr_t) noexcept {
        reset();
        return *this;
    }

private:
    pointer ptr_;
    Del del_;
};

#define _DECL_UNIQ_RELOP(op)                                                                       \
    template<class T1, class D1, class T2, class D2>                                               \
    constexpr bool operator op(                                                                    \
        unique_ptr<T1, D1> const& x, unique_ptr<T2, D2> const& y                                   \
    ) noexcept {                                                                                   \
        return x.get() op y.get();                                                                 \
    }

_DECL_UNIQ_RELOP(==)
_DECL_UNIQ_RELOP(!=)

#undef _DECL_UNIQ_RELOP

#define _DECL_UNIQ_RELOP(op, name)                                                                 \
    template<class T1, class D1, class T2, class D2>                                               \
    constexpr bool operator op(                                                                    \
        unique_ptr<T1, D1> const& x, unique_ptr<T2, D2> const& y                                   \
    ) noexcept {                                                                                   \
        return std::name<common_type_t<                                                            \
            typename unique_ptr<T1, D1>::pointer, typename unique_ptr<T2, D2>::pointer>>()(        \
            x.get(), y.get()                                                                       \
        );                                                                                         \
    }
_DECL_UNIQ_RELOP(<, less)
_DECL_UNIQ_RELOP(>, greater)
_DECL_UNIQ_RELOP(<=, less_equal)
_DECL_UNIQ_RELOP(>=, greater_equal)
#undef _DECL_UNIQ_RELOP

template<class T1, class D1, class T2, class D2> requires three_way_comparable_with<
    typename unique_ptr<T1, D1>::pointer, typename unique_ptr<T2, D2>::pointer>
constexpr compare_three_way_result_t<
    typename unique_ptr<T1, D1>::pointer, typename unique_ptr<T2, D2>::pointer>
operator<=>(unique_ptr<T1, D1> const& x, unique_ptr<T2, D2> const& y) noexcept {
    return std::compare_three_way()(x.get(), y.get());
}

template<class T, class D>
constexpr bool operator==(unique_ptr<T, D> const& x, nullptr_t) noexcept {
    return x.get() == nullptr;
}
template<class T, class D> requires three_way_comparable<typename unique_ptr<T, D>::pointer>
constexpr compare_three_way_result_t<typename unique_ptr<T, D>::pointer>
operator<=>(unique_ptr<T, D> const& x, nullptr_t) noexcept {
    return compare_three_way()(x.get(), nullptr);
}

template<class T, class... As> requires(!is_array_v<T>)
constexpr unique_ptr<T> make_unique(As&&... as) {
    return unique_ptr<T>(new T(std::forward<As>(as)...));
}
template<class T> requires is_unbounded_array_v<T> constexpr unique_ptr<T> make_unique(size_t n) {
    return unique_ptr<T>(new remove_extent_t<T>[n]());
}
template<class T> requires is_bounded_array_v<T> constexpr unique_ptr<T>
make_unique(auto&&...) = delete ("Cannot create unique_ptr to bounded array");

template<class T> requires(!is_array_v<T>) constexpr unique_ptr<T> make_unique_for_overwrite() {
    return unique_ptr<T>(new T);
}
template<class T> requires is_unbounded_array_v<T>
constexpr unique_ptr<T> make_unique_for_overwrite(size_t n) {
    return unique_ptr<T>(new remove_extent_t<T>[n]);
}

template<class T> requires is_bounded_array_v<T> constexpr unique_ptr<T>
make_unique_for_overwrite(auto&&...) = delete ("Cannot create unique_ptr to bounded array");

template<class T, class D> requires is_swappable_v<D>
constexpr void swap(unique_ptr<T, D>& x, unique_ptr<T, D>& y) noexcept {
    x.swap(y);
}

}  // namespace std
