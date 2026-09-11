
#include <cxxabi.h>

abi::__cxa_exception* cxa_from_except_ptr(void* ptr) noexcept;

[[noreturn]] void abi::__cxa_throw(void* exception, std::type_info const* thrown_type, void (*dtor)(void*)) {
    auto* cxa                = cxa_from_except_ptr(exception);
    cxa->exceptionType       = thrown_type;
    cxa->exceptionDestructor = dtor;
    cxa->refcount            = 1;

    abi::__cxa_globals.uncaught_exceptions += 1;

    _Unwind_RaiseException(&cxa->ucb);
    __cxa_call_terminate(&cxa->ucb);
}