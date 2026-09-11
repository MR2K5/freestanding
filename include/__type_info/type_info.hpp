#pragma once

namespace std {
class type_info {
public:
    virtual ~type_info();
    constexpr bool operator==(type_info const& o) const noexcept {
        if consteval {
            return this == &o;
        } else {
            return __type_name == o.__type_name;
        }
    }
    // bool operator!=(type_info const&) const;
    bool before(type_info const&) const noexcept;
    char const* name() const noexcept;

    virtual bool __can_catch(type_info const* thrown_type, bool is_ref, void*& asjusted_ptr) const noexcept;

private:
    type_info(type_info const& rhs)            = default;
    type_info& operator=(type_info const& rhs) = default;

    char const* __type_name;
};
}