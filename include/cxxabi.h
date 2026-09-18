#pragma once

#include <cstddef>
#include <cstdint>

// #include <__rtti.hpp>

#include <unwind.h>

namespace std {
    using terminate_handler = void(*)();
    class type_info;
}

#ifdef __cplusplus
namespace __cxxabiv1 {
struct __class_type_info;

extern "C" {
#endif

#ifndef __arm__
int __cxa_guard_acquire(int64_t* p) noexcept;
void __cxa_guard_release(int64_t* p) noexcept;
void __cxa_guard_abort(int64_t* p) noexcept;
#else
int __cxa_guard_acquire(int* p) noexcept;
void __cxa_guard_release(int* p) noexcept;
void __cxa_guard_abort(int* p) noexcept;
#endif

[[noreturn]] void __cxa_pure_virtual() noexcept;
[[noreturn]] void __cxa_deleted_virtual() noexcept;

int __cxa_atexit(void (*)(void*), void*, void*) noexcept;
void __cxa_finalize(void* dso) noexcept;

void* __dynamic_cast(
    void const* sub, __class_type_info const* src, __class_type_info const* dst,
    std::ptrdiff_t src2dst_offset
) noexcept;

#if defined(__arm__)

void* __cxa_allocate_exception(size_t sz);
void __cxa_free_exception(void* p) noexcept;
[[noreturn]] void __cxa_throw(void*, std::type_info const*, void (*dtor)(void*));
[[noreturn]] void __cxa_rethrow();
void* __cxa_begin_catch(_Unwind_Control_Block* ucpb) noexcept;
void* __cxa_get_exception_pointer(_Unwind_Control_Block* ucpb) noexcept;
void __cxa_end_catch() noexcept;

// Internal

enum __cxa_type_match_result {
    ctm_failed                     = 0,
    ctm_succeeded                  = 1,
    ctm_succeeded_with_ptr_to_base = 2
};

__cxa_type_match_result __cxa_type_match(
    _Unwind_Control_Block* ucbp, std::type_info const* rttip, bool is_reference_type,
    void** matched_object
) noexcept;

[[noreturn]] void __cxa_call_terminate(_Unwind_Control_Block* ucbp) noexcept;


struct __cxa_exception {
    _Unwind_Control_Block ucb;
    std::type_info const* exceptionType;        // RTTI object describing the type of the exception
    void (*exceptionDestructor)(void*);         // Destructor for the exception object (may be NULL)
    std::terminate_handler terminateHandler;    // Handler in force after evaluating throw expr
    __cxa_exception* nextCaughtException;       // Chain of "currently caught" c++ exception objects
    uint32_t handlerCount;                      // Count of how many handlers this EO is "caught" in
    uint32_t refcount;
    alignas (std::max_align_t) std::byte exception[];
};

// TODO We might need __cxa_dependent_exception if we ever support multi-threded rethrow_exception

struct __cxa_eh_globals {
    uint32_t uncaught_exceptions;
    __cxa_exception* caught_exceptions;
};

constinit thread_local static __cxa_eh_globals __cxa_globals {};

void __cxa_increment_exception_refcount(void* exc) noexcept;
void __cxa_decrement_exception_refcount(void* exc) noexcept;

#endif

#ifdef __cplusplus
}

}  // namespace __cxxabiv1

namespace abi = __cxxabiv1;

#endif

#include <__rtti.hpp>