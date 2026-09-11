#include "__rtti.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cxxabi.h>
#include <string_view>
#include <typeinfo>

namespace __cxxabiv1 {

using std::byte;

struct derived_info {
    __class_type_info const* dynamic_type;
    byte const* dynamic_ptr;
    ptrdiff_t offset_to_derived;

    static derived_info get(void const* sub) {
        byte const* const* vtable = *reinterpret_cast<byte const* const* const*>(sub);
        ptrdiff_t off             = reinterpret_cast<ptrdiff_t>(vtable[-2]);
        return {
            .dynamic_type      = reinterpret_cast<__class_type_info const*>(vtable[-1]),
            .dynamic_ptr       = static_cast<byte const*>(sub) + off,
            .offset_to_derived = off
        };
    }
};

struct dyn_cast_params {
    __class_type_info const* dynamic_type;
    byte const* dynamic_ptr;
    ptrdiff_t offset_to_derived;
    byte const* sub;
    __class_type_info const* src_type;
    __class_type_info const* dst_type;
    ptrdiff_t src2dst_offset;
};

struct search_above_res {
    // Info of our answer
    bool found_any_static = false;
    bool found_our_static = false;
    bool has_public       = false;

    void unify(search_above_res const& o) noexcept {
        found_any_static = found_any_static || o.found_any_static;
        found_our_static = found_our_static || o.found_our_static;
        has_public       = has_public || o.has_public;
    }
};

struct search_above_state {
    bool in_public_path = true;
    byte const* cur_ptr = nullptr;
};

struct search_general_state {
    bool in_public_path = true;
    byte const* cur_ptr = nullptr;
};

struct search_general_res {
    byte const* dst_leading     = nullptr;
    byte const* dst_not_leading = nullptr;

    int num_leading     = 0;
    int num_not_leading = 0;

    bool dynamic_to_dst_public = false;
    bool dst_to_src_public     = false;

    bool found_our_src         = false;
    bool dynamic_to_src_public = false;

    enum status { unknown, yes, no };
    status is_dst_derived = unknown;

    bool done = false;
};

search_above_res __class_type_info::found_static_type_above(
    dyn_cast_params const& params, search_above_state const& state
) const noexcept {
    if (params.sub == state.cur_ptr) {
        return {
            .found_any_static = true, .found_our_static = true, .has_public = state.in_public_path
        };
    }
    return {.found_any_static = true};
}

byte const* dyn_cast_to_derived(dyn_cast_params const& params) {
    if (params.src2dst_offset >= 0) {
        // We may have multiple bases, the hint only guarantees that the base is non-virtual and
        // unique. So we need to check this is the correct one
        if (params.offset_to_derived != -params.src2dst_offset) return nullptr;
        return params.dynamic_ptr;
    }
    if (params.src2dst_offset == -2) return nullptr;

    // Search above
    auto res = params.dynamic_type->search_above_dst(params, {.cur_ptr = params.dynamic_ptr});
    if (res.found_our_static && res.has_public) { return params.dynamic_ptr; }
    return nullptr;
}

// Try to cast downwards on the tree
byte const* dyn_cast_down(dyn_cast_params const& params) {
    // If we are casting to a more derived, but unique type in the graph
    // We will have src2dst be the offset to that type if it exists
    if (params.src2dst_offset < 0) return nullptr;

    // Assume the cast is valid. Then sub == dst + src2dst
    byte const* dst_guess = params.sub - params.src2dst_offset;

    // If the object would be outside the class, fail early
    if (reinterpret_cast<std::intptr_t>(dst_guess)
        < reinterpret_cast<std::intptr_t>(params.dynamic_ptr))
        return nullptr;

    // Now we need to find the dst type in the tree
    auto sub_params     = params;
    sub_params.src_type = sub_params.dst_type;
    sub_params.sub      = dst_guess;

    auto dst_res =
        params.dynamic_type->search_above_dst(sub_params, {.cur_ptr = params.dynamic_ptr});
    if (dst_res.found_our_static) return dst_guess;
    return nullptr;
}

byte const* dyn_cast_full(dyn_cast_params const& params) {
    search_general_res res;
    params.dynamic_type->search_general(params, {.cur_ptr = params.dynamic_ptr}, res);

    if (res.num_leading == 0) {
        if (res.num_not_leading == 1 && res.dynamic_to_dst_public && res.dynamic_to_src_public)
            return res.dst_not_leading;
    } else if (res.num_leading == 1) {
        if (res.dst_to_src_public
            || (res.num_not_leading == 0 && res.dynamic_to_dst_public && res.dynamic_to_src_public))
            return res.dst_leading;
    }

    return nullptr;
}

void* __dynamic_cast(
    void const* sub, __class_type_info const* src, __class_type_info const* dst,
    std::ptrdiff_t src2dst_offset
) {
    derived_info d_info = derived_info::get(sub);
    dyn_cast_params params{
        d_info.dynamic_type,
        d_info.dynamic_ptr,
        d_info.offset_to_derived,
        static_cast<byte const*>(sub),
        src,
        dst,
        src2dst_offset
    };

    void const* res = nullptr;

    if (*d_info.dynamic_type == *dst) {
        res = dyn_cast_to_derived(params);
    } else {
        // Try the fast path down
        res = dyn_cast_down(params);
        if (!res) res = dyn_cast_full(params);
    }

    return const_cast<void*>(res);
}

// This checks if (static_ptr, static_type) is a base of (dst_type)
search_above_res __class_type_info::search_above_dst(
    dyn_cast_params const& params, search_above_state const& state
) const noexcept {
    if (*this == *params.src_type) {
        // We have found a (static_type, static_ptr) pair
        return found_static_type_above(params, state);
    }
    return {};
}

search_above_res __si_class_type_info::search_above_dst(
    dyn_cast_params const& params, search_above_state const& state
) const noexcept {
    if (*this == *params.src_type) {
        // We have found a (static_type, static_ptr) pair
        return found_static_type_above(params, state);
    } else {
        // recurse
        return __base_type->search_above_dst(params, state);
    }
}

search_above_res __vmi_class_type_info::search_above_dst(
    dyn_cast_params const& params, search_above_state const& state
) const noexcept {
    if (*this == *params.src_type) { return found_static_type_above(params, state); }

    bool has_repeat = __flags & __non_diamond_repeat_mask;

    search_above_res final_res;

    for (unsigned i = 0; i < __base_count; ++i) {
        auto& base = __base_info[i];

        auto nw           = state;
        nw.in_public_path = state.in_public_path && (base.__offset_flags & base.__public_mask);
        nw.cur_ptr        = nw.cur_ptr + base.calculate_offset(state.cur_ptr);

        auto res = base.__base_type->search_above_dst(params, nw);
        final_res.unify(res);

        if (res.found_any_static && !has_repeat) break;
        if (res.found_our_static && res.has_public) break;
    }

    return final_res;
}

ptrdiff_t __base_class_type_info::calculate_offset(void const* cur_ptr) const noexcept {
    auto off = __offset_flags >> __offset_shift;
    if (__offset_flags & __virtual_mask) {
        auto vtable = *static_cast<void const* const*>(cur_ptr);
        off         = *reinterpret_cast<ptrdiff_t const*>(static_cast<char const*>(vtable) + off);
    }

    return off;
}

void __class_type_info::search_general(
    dyn_cast_params const& params, search_general_state const& state, search_general_res& res
) const noexcept {
    if (*this == *params.src_type && params.sub == state.cur_ptr) {
        res.found_our_src = true;
        if (!res.dynamic_to_src_public) res.dynamic_to_src_public = state.in_public_path;
    } else if (*this == *params.dst_type) {
        if (res.dst_leading == state.cur_ptr || res.dst_not_leading == state.cur_ptr) {
            if (state.in_public_path) res.dynamic_to_dst_public = true;
        } else {
            res.dst_not_leading        = state.cur_ptr;
            res.num_not_leading       += 1;
            res.dynamic_to_dst_public  = state.in_public_path;
            res.is_dst_derived         = search_general_res::no;
        }
    }
}

void __si_class_type_info::search_general(
    dyn_cast_params const& params, search_general_state const& state, search_general_res& res
) const noexcept {
    if (*this == *params.src_type && params.sub == state.cur_ptr) {
        res.found_our_src = true;
        if (!res.dynamic_to_src_public) res.dynamic_to_src_public = state.in_public_path;
    } else if (*this == *params.dst_type) {
        if (res.dst_leading == state.cur_ptr || res.dst_not_leading == state.cur_ptr) {
            if (state.in_public_path) res.dynamic_to_dst_public = true;
        } else {

            bool does_dst_point_to_our_static = false;

            if (res.is_dst_derived != search_general_res::no) {
                auto above = __base_type->search_above_dst(
                    params, {.in_public_path = true, .cur_ptr = state.cur_ptr}
                );
                res.is_dst_derived           = above.found_any_static ? res.yes : res.no;
                does_dst_point_to_our_static = above.found_our_static;

                if (above.found_our_static) {
                    res.num_leading       += 1;
                    res.dst_leading        = state.cur_ptr;
                    res.dst_to_src_public  = above.has_public;
                }
            }

            if (!does_dst_point_to_our_static) {
                res.num_not_leading       += 1;
                res.dst_not_leading        = state.cur_ptr;
                res.dynamic_to_dst_public  = state.in_public_path;

                if (res.num_leading == 1 && !res.dst_to_src_public) res.done = true;
            }
        }
    } else {
        __base_type->search_general(params, state, res);
    }
}

void __vmi_class_type_info::search_general(
    dyn_cast_params const& params, search_general_state const& state, search_general_res& res
) const noexcept {

    if (*this == *params.src_type && params.sub == state.cur_ptr) {
        // We found the source object, which is not
        res.found_our_src = true;
        if (!res.dynamic_to_src_public) res.dynamic_to_src_public = state.in_public_path;
    } else if (*this == *params.dst_type) {
        // Have we been here?
        if (res.dst_leading == state.cur_ptr || res.dst_not_leading == state.cur_ptr) {
            // Yes, just update the path
            if (state.in_public_path) res.dynamic_to_dst_public = true;
            return;
        }

        // New node
        res.dynamic_to_dst_public = state.in_public_path;

        // Search if this points to our static
        // Optimize and search only if we know it is possible to match
        bool dst_points_to_our_static   = false;
        bool is_dst_derived_from_static = false;

        if (res.is_dst_derived != search_general_res::no) {
            for (unsigned i = 0; i < __base_count; ++i) {
                auto& base = __base_info[i];
                if (res.done) break;

                auto above_res = base.__base_type->search_above_dst(
                    params, {
                                .in_public_path = bool(base.__offset_flags & base.__public_mask),
                                .cur_ptr = state.cur_ptr + base.calculate_offset(state.cur_ptr),
                            }
                );

                if (above_res.found_any_static) {
                    is_dst_derived_from_static = true;
                    if (above_res.found_our_static) {
                        dst_points_to_our_static  = true;
                        res.found_our_src         = true;
                        res.dst_leading           = state.cur_ptr;
                        res.dst_to_src_public     = above_res.has_public;
                        res.num_leading          += 1;

                        // FOund multiple dst types that lead to our static. ambiguous
                        if (res.num_leading == 2) {
                            res.done = true;
                            break;
                        }
                        if (res.dst_to_src_public) break;
                        if (!(__flags & __diamond_shaped_mask)) break;
                    } else {
                        // FOund a base, break if it cannot repeat
                        if (!(__flags & __non_diamond_repeat_mask)) break;
                    }
                }
            }

            res.is_dst_derived = is_dst_derived_from_static ? res.yes : res.no;

            if (!dst_points_to_our_static) {
                // FOund dst that does not point
                res.num_not_leading += 1;
                res.dst_not_leading  = state.cur_ptr;
                // If we have already found a non-public one that points, the cast is ambiguous
                if (res.num_leading == 1 && !res.dst_to_src_public) res.done = true;
            }
        }
    } else {
        for (unsigned i = 0; i < __base_count; ++i) {
            auto& base = __base_info[i];

            base.__base_type->search_general(
                params,
                {.in_public_path =
                     state.in_public_path && bool(base.__offset_flags & base.__public_mask),
                 .cur_ptr = state.cur_ptr + base.calculate_offset(state.cur_ptr)},
                res
            );
            if (res.done) break;
            if (!(__flags & __diamond_shaped_mask)) {
                if (__flags & __non_diamond_repeat_mask) {
                    if (res.num_leading == 1 && res.dst_to_src_public) break;
                } else if (res.num_leading == 1)
                    break;
            }
        }
    }
}

__array_type_info::~__array_type_info()                         = default;
__function_type_info::~__function_type_info()                   = default;
__enum_type_info::~__enum_type_info()                           = default;
__class_type_info::~__class_type_info()                         = default;
__si_class_type_info::~__si_class_type_info()                   = default;
__vmi_class_type_info::~__vmi_class_type_info()                 = default;
__pbase_type_info::~__pbase_type_info()                         = default;
__pointer_type_info::~__pointer_type_info()                     = default;
__pointer_to_member_type_info::~__pointer_to_member_type_info() = default;
__fundamental_type_info::~__fundamental_type_info()             = default;

struct unambiguous_base_state {
    __class_type_info const* target;  // Target base type
    void* found       = nullptr;      // Pointer to found type
    bool is_public    = false;        // If we have a public oath to found
    bool is_ambiguous = false;        // Found 2 different base objects. Early return
};

bool __class_type_info::check_self(
    void* cur, bool is_public, unambiguous_base_state& state
) const noexcept {
    if (*state.target == *this) {
        if (!state.found) {
            // First time
            state.found     = cur;
            state.is_public = is_public;
        } else if (state.found == cur) {
            // Diamond
            if (is_public) state.is_public = true;
        } else {
            // Ambiguous
            state.is_ambiguous = true;
        }
        return true;
    }
    return false;
}

// Return true if target is found anywhere in this branch
bool __class_type_info::has_unambiguous_public_base(
    void* cur, bool is_public, unambiguous_base_state& state
) const noexcept {
    return check_self(cur, is_public, state);
}

bool __si_class_type_info::has_unambiguous_public_base(
    void* cur, bool is_public, unambiguous_base_state& state
) const noexcept {
    if (check_self(cur, is_public, state)) return true;

    return __base_type->has_unambiguous_public_base(cur, is_public, state);
}

bool __vmi_class_type_info::has_unambiguous_public_base(
    void* cur, bool is_public, unambiguous_base_state& state
) const noexcept {
    if (check_self(cur, is_public, state)) return true;

    bool any_found = false;

    for (unsigned i = 0; i < __base_count; ++i) {
        auto& base = __base_info[i];

        void* new_ptr       = (byte*)cur + base.calculate_offset(cur);
        bool base_is_public = base.__offset_flags & base.__public_mask;

        if (base.__base_type->has_unambiguous_public_base(
                new_ptr, is_public && base_is_public, state
            )) {
            any_found = true;

            if (state.is_ambiguous) return true;

            // early returns
            // We cannot find an ambiguous base, or a more public one than the current

            if ((!is_public || state.is_public || !(__flags & __diamond_shaped_mask))
                && !(__flags & __non_diamond_repeat_mask))
                return true;

            // Cases: if a repeating base exists, we must search all anyways.
            // otherwise, if the path to here is not public, we cannot find a more public path from
            // here anyway if the currently found path is public already, no need to search and if
            // there is no diamond, we cannot find the same base anyway so there is more public
            // path}
        }
    }
    return any_found;
}

// TODO inspect whether we can enable exceptions but no rtti
// It might be possible to build only this file w/ rtti
// Or maybe we can replace dynamic_cast<> with __dyanamic_cast. But then we need to get they
// type_info pointers somehow. maybe declaring the magnled symbols as extern?

bool __class_type_info::__can_catch(
    type_info const* thrown_type, bool, void*& adjusted
) const noexcept {
    if (*this == *thrown_type) return true;

    // We need to check if thrown_type is derived from us.

    // If thrown is a class type we can search_above to walk the base classes
    auto* class_info = dynamic_cast<__class_type_info const*>(thrown_type);
    if (!class_info) return false;

    unambiguous_base_state st{.target = this};
    class_info->has_unambiguous_public_base(adjusted, true, st);

    if (st.found && st.is_public && !st.is_ambiguous) {
        adjusted = st.found;
        return true;
    }
    return false;
}

bool __pbase_type_info::__can_catch(
    type_info const* thrown_type, bool, void*&
) const noexcept {
    bool needs_strcmp = this->__flags & (__incomplete_class_mask | __incomplete_mask);
    if (!needs_strcmp) {
        auto thrown_pbase = dynamic_cast<__pbase_type_info const*>(thrown_type);
        if (!thrown_pbase) return false;
        needs_strcmp = thrown_pbase->__flags & (__incomplete_class_mask | __incomplete_mask);
    }

    return *this == *thrown_type
        || (needs_strcmp
            && std::string_view(this->name()) == std::string_view(thrown_type->name()));
}

bool __pointer_type_info::__can_catch(
    type_info const* thrown_type, bool is_ref, void*& adjusted
) const noexcept {
    // nullptr_t
    if (*thrown_type == typeid(nullptr_t)) {
        adjusted = nullptr;
        return true;
    }

    if (__pbase_type_info::__can_catch(thrown_type, is_ref, adjusted)) {
        if (adjusted) adjusted = *static_cast<void**>(adjusted);
        return true;
    }

    auto p = dynamic_cast<__pbase_type_info const*>(thrown_type);
    if (!p) return false;
    if (adjusted) adjusted = *static_cast<void**>(adjusted);

    // Check that the flags match
    // TODO __restrict__?
    if (p->__flags & ~__flags & (__const_mask | __volatile_mask)) return false;
    if (~p->__flags & __flags & (__noexcept_mask)) return false;

    if (*__pointee == *p->__pointee) return true;  // Pointed-to types match
    if (*__pointee == typeid(void)) {
        // pointer to function cannot be converted to void
        return dynamic_cast<__function_type_info const*>(p->__pointee) == nullptr;
    }

    // Pointer to pointer
    auto pointer_to_pointer = dynamic_cast<__pointer_type_info const*>(__pointee);
    if (pointer_to_pointer) {
        return pointer_to_pointer->can_catch_nested(p->__pointee, !(__flags & __const_mask));
    }

    // Pointer to member
    auto pointer_to_member = dynamic_cast<__pointer_to_member_type_info const*>(__pointee);
    if (pointer_to_member) {
        return pointer_to_member->can_catch_nested(p->__pointee, !(__flags & __const_mask));
    }

    // Pointer to class
    auto catch_class  = dynamic_cast<__class_type_info const*>(__pointee);
    auto thrown_class = dynamic_cast<__class_type_info const*>(p->__pointee);
    if (!catch_class || !thrown_class) return false;

    if (p->__flags & __incomplete_class_mask) return false;

    bool has_obj = adjusted != nullptr;
    unambiguous_base_state st{.target = catch_class};
    if (!thrown_class->has_unambiguous_public_base(adjusted, true, st) || st.is_ambiguous
        || !st.is_public)
        return false;
    if (has_obj) adjusted = st.found;
    return true;
}

bool __pointer_type_info::can_catch_nested(
    type_info const* inner_type, bool outer_no_const
) const noexcept {
    if (outer_no_const) return *this == *inner_type;

    // We get the inner type, but we do not know if it is a poitner type yet.
    // At this point the innermost pointed type must match, as we are at most a qualification
    // conversion
    auto inner_pointer_type = dynamic_cast<__pointer_type_info const*>(inner_type);
    if (!inner_pointer_type) return false;

    if ((__flags ^ inner_pointer_type->__flags) & (__noexcept_mask))
        return false;  // Cannot remove noexcept anymore
    // Check that qualifiers is a superset
    if (inner_pointer_type->__flags & ~__flags & (__const_mask | __volatile_mask)) return false;

    if (*__pointee == *inner_pointer_type->__pointee) return true;

    auto pointer = dynamic_cast<__pointer_type_info const*>(__pointee);
    if (pointer)
        return pointer->can_catch_nested(inner_pointer_type->__pointee, !(__flags & __const_mask));

    // pointer to member
    auto pmem = dynamic_cast<__pointer_to_member_type_info const*>(__pointee);
    if (pmem)
        return pmem->can_catch_nested(inner_pointer_type->__pointee, !(__flags & __const_mask));

    return false;
}

bool __pointer_to_member_type_info::__can_catch(
    type_info const* thrown_type, bool is_ref, void*& adjusted
) const noexcept {
    if (*thrown_type == typeid(nullptr_t)) {
        struct X{};
        if (dynamic_cast<__function_type_info const*>(__pointee)) {
            constinit static int (X::* const np)() = nullptr;
            adjusted = const_cast<int (X::**)()>(&np);
        } else {
            static int X::* const np= nullptr;
            adjusted = const_cast<int X::**>(&np);
        }
        return true;
    }

    if (__pbase_type_info::__can_catch(thrown_type, is_ref, adjusted)) return true;

    auto thrown_ptr = dynamic_cast<__pointer_to_member_type_info const*>(thrown_type);
    if (!thrown_ptr) return false;
    if (__flags & ~thrown_ptr->__flags & (__noexcept_mask)) return false;
    if (~__flags & thrown_ptr->__flags & (__const_mask | __volatile_mask)) return false;
    if (*__pointee != *thrown_ptr->__pointee) return false;
    if (*__context != *thrown_ptr->__context) return false;
    return true;
}

bool __pointer_to_member_type_info::can_catch_nested(type_info const* thrown_type, bool outer_no_const) const noexcept {
    if (outer_no_const) return *this == *thrown_type;

    auto thrown_pmem = dynamic_cast<__pointer_to_member_type_info const*>(thrown_type);
    if (!thrown_pmem) return false;

    if (~__flags & thrown_pmem->__flags & (__const_mask | __volatile_mask)) return false;
    if ((__flags ^ thrown_pmem->__flags) & __noexcept_mask) return false;
    
    return *__pointee == *thrown_pmem->__pointee && *__context == *thrown_pmem->__context;
}

}  // namespace __cxxabiv1

namespace std {

bool type_info::__can_catch(type_info const* thrown_type, bool, void*&) const noexcept {
    return *this == *thrown_type;
}

type_info::~type_info() = default;

bool type_info::before(type_info const& o) const noexcept {
    return __type_name < o.__type_name;
}

char const* type_info::name() const noexcept {
    return __type_name;
}

bad_cast::~bad_cast()     = default;
bad_typeid::~bad_typeid() = default;

}  // namespace std
