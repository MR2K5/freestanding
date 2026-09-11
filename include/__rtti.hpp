#pragma once

#include <cstddef>
#include <__type_info/type_info.hpp>

namespace __cxxabiv1 {

// https://itanium-cxx-abi.github.io/cxx-abi/abi.html#rtti

struct __class_type_info;

struct search_above_res;
struct search_above_state;

struct search_general_state;
struct search_general_res;

struct dyn_cast_params;

struct unambiguous_base_state;

struct __fundamental_type_info: std::type_info {
    ~__fundamental_type_info() override;
};

struct __array_type_info: std::type_info {
    ~__array_type_info() override;
};

struct __function_type_info: std::type_info {
    ~__function_type_info() override;
};

struct __enum_type_info: std::type_info {
    ~__enum_type_info() override;
};

struct __class_type_info: std::type_info {
    ~__class_type_info() override;
    virtual search_above_res
    search_above_dst(dyn_cast_params const& params, search_above_state const& state) const noexcept;

    virtual void search_general(
        dyn_cast_params const& params, search_general_state const& state, search_general_res& res
    ) const noexcept;

    search_above_res found_static_type_above(
        dyn_cast_params const& params, search_above_state const& state
    ) const noexcept;

    bool __can_catch(
        type_info const* thrown_type, bool is_ref, void*& asjusted_ptr
    ) const noexcept override;

    bool check_self(void* cur, bool is_public, unambiguous_base_state& state) const noexcept;
    virtual bool has_unambiguous_public_base(void* cur, bool is_public, unambiguous_base_state& state) const noexcept;
};

struct __si_class_type_info: public __class_type_info {
    ~__si_class_type_info() override;
    __class_type_info const* __base_type;

    virtual search_above_res search_above_dst(
        dyn_cast_params const& params, search_above_state const& state
    ) const noexcept override;

    virtual void search_general(
        dyn_cast_params const& params, search_general_state const& state, search_general_res& res
    ) const noexcept override;

    virtual bool has_unambiguous_public_base(void* cur, bool is_public, unambiguous_base_state& state) const noexcept override;
};

struct __base_class_type_info {
public:
    __class_type_info const* __base_type;
    long __offset_flags;

    enum __offset_flags_masks { __virtual_mask = 0x1, __public_mask = 0x2, __offset_shift = 8 };

    ptrdiff_t calculate_offset(void const* cur_ptr) const noexcept;
};

struct __vmi_class_type_info: __class_type_info {
    ~__vmi_class_type_info() override;

    unsigned int __flags;
    unsigned int __base_count;
    __base_class_type_info __base_info[];

    enum __flags_masks { __non_diamond_repeat_mask = 0x1, __diamond_shaped_mask = 0x2 };

    virtual search_above_res search_above_dst(
        dyn_cast_params const& params, search_above_state const& state
    ) const noexcept override;

    virtual void search_general(
        dyn_cast_params const& params, search_general_state const& state, search_general_res& res
    ) const noexcept override;

    virtual bool has_unambiguous_public_base(void* cur, bool is_public, unambiguous_base_state& state) const noexcept override;
};

struct __pbase_type_info: std::type_info {
    ~__pbase_type_info() override;

    unsigned int __flags;
    std::type_info const* __pointee;

    enum __masks {
        __const_mask            = 0x1,
        __volatile_mask         = 0x2,
        __restrict_mask         = 0x4,
        __incomplete_mask       = 0x8,
        __incomplete_class_mask = 0x10,
        __transaction_safe_mask = 0x20,
        __noexcept_mask         = 0x40
    };

    bool __can_catch(
        type_info const* thrown_type, bool is_ref, void*& asjusted_ptr
    ) const noexcept override;
};

struct __pointer_type_info: __pbase_type_info {
    ~__pointer_type_info() override;

    bool __can_catch(
        type_info const* thrown_type, bool is_ref, void*& asjusted_ptr
    ) const noexcept override;

    bool can_catch_nested(type_info const* thrown_type, bool outer_no_const) const noexcept;
};

struct __pointer_to_member_type_info: __pbase_type_info {
    ~__pointer_to_member_type_info() override;

    __class_type_info const* __context;

    bool __can_catch(
        type_info const* thrown_type, bool is_ref, void*& asjusted_ptr
    ) const noexcept override;

    bool can_catch_nested(type_info const* thrown_type, bool outer_no_const) const noexcept;
};

}  // namespace __cxxabiv1
