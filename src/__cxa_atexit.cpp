#include <cstddef>
#include <cxxabi.h>

extern "C" {
// 1. CRITICAL: You must define this global symbol.
// The compiler will pass its address as the third argument to __cxa_atexit.
[[gnu::weak]] void* __dso_handle = nullptr;

// 2. Define a simple layout to register the destructor hooks
struct __atexit_entry {
    void (*destructor)(void*);
    void* arg;
};

// For a bare-metal environment, a fixed-size array avoids heap allocations
constexpr std::size_t MAX_DESTRUCTORS = 32;
static __atexit_entry __destructor_registry[MAX_DESTRUCTORS];
static std::size_t __registry_count = 0;

// 3. The Core ABI Entry Point
int __cxa_atexit(void (*destructor)(void*), void* arg, void*) noexcept {
    if (__registry_count >= MAX_DESTRUCTORS) {
        return -1;  // Out of registry slots
    }

    // Store the function pointer and its object context argument
    __destructor_registry[__registry_count++] = {destructor, arg};
    return 0;  // Success
}

// 4. Optional: Call this in your shutdown loop if your system ever exits
void __cxa_finalize(void*) noexcept {
    // Walk backward through the registry to honor LIFO execution order
    while (__registry_count > 0) {
        __registry_count--;
        auto& entry = __destructor_registry[__registry_count];
        if (entry.destructor) { entry.destructor(entry.arg); }
    }
}
}
