#include <cxxabi.h>
#include <exception>

std::exception_ptr std::current_exception() noexcept {
    return nullptr;
}

void __cxxabiv1::__cxa_increment_exception_refcount(void* obj) noexcept {
    if (obj) std::terminate();
}

void __cxxabiv1::__cxa_decrement_exception_refcount(void* obj) noexcept {
    if (obj) std::terminate();
}
