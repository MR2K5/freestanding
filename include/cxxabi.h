#pragma once

#include <stdint.h>

#include <__rtti.hpp>

#ifdef __cplusplus
namespace __cxxabiv1 {
extern "C" {
#endif

int __cxa_guard_acquire(int64_t* p) noexcept;
void __cxa_guard_release(int64_t* p) noexcept;
void __cxa_guard_abort(int64_t* p) noexcept;

[[noreturn]] void __cxa_pure_virtual() noexcept;
[[noreturn]] void __cxa_deleted_virtual() noexcept;

int __cxa_atexit(void (*)(void*), void*, void*) noexcept;
void __cxa_finalize(void* dso) noexcept;

void* __dynamic_cast(
    void const* sub, __class_type_info const* src, __class_type_info const* dst,
    std::ptrdiff_t src2dst_offset
);

#ifdef __cplusplus
}

}  // namespace __cxxabiv1

namespace abi = __cxxabiv1;

#endif
