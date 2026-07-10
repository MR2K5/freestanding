#include <cstdlib>
#include <new>

#include <config.hpp>
#include <utility>

#if _STD_ENABLE_HEAP
#  error "Not implemented"
#else

// Regular New / Array New
void* operator new(std::size_t) {
    std::abort();
}
void* operator new[](std::size_t) {
    std::abort();
}

// Nothrow Variants
void* operator new(std::size_t, std::nothrow_t const&) noexcept {
    return nullptr;
}
void* operator new[](std::size_t, std::nothrow_t const&) noexcept {
    return nullptr;
}

// Global Deletions
void operator delete(void*) noexcept {}
void operator delete[](void*) noexcept {}
void operator delete(void*, std::size_t) noexcept {}
void operator delete[](void*, std::size_t) noexcept {}

#endif

constinit static std::new_handler nh = nullptr;

std::new_handler std::get_new_handler() noexcept {
#if _STD_HAS_THREADS
#  error unimplemented
#else
    return nh;
#endif
}

std::new_handler std::set_new_handler(std::new_handler h) noexcept {
#if _STD_HAS_THREADS
#  error unimplemented
#else
    return std::exchange(nh, h);
#endif
}
