#include "__rtti.hpp"
#include <cstddef>
#include <cstdint>
#include <cxxabi.h>
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

__fundamental_type_info::~__fundamental_type_info()             = default;
__array_type_info::~__array_type_info()                         = default;
__function_type_info::~__function_type_info()                   = default;
__enum_type_info::~__enum_type_info()                           = default;
__class_type_info::~__class_type_info()                         = default;
__si_class_type_info::~__si_class_type_info()                   = default;
__vmi_class_type_info::~__vmi_class_type_info()                 = default;
__pbase_type_info::~__pbase_type_info()                         = default;
__pointer_type_info::~__pointer_type_info()                     = default;
__pointer_to_member_type_info::~__pointer_to_member_type_info() = default;

}  // namespace __cxxabiv1

namespace std {

type_info::~type_info() = default;

bool type_info::before(type_info const& o) const noexcept {
    return __type_name < o.__type_name;
}

char const* type_info::name() const noexcept {
    return __type_name;
}

bad_cast::~bad_cast() = default;
bad_typeid::~bad_typeid() = default;

}  // namespace std
