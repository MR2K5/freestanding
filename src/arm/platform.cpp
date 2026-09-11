#include <atomic>
#include <cstdlib>
#include <cxxabi.h>

extern "C" int __cxxabiv1::__cxa_guard_acquire(int* p) noexcept {
    std::atomic_ref g(*p);

    while (1) {
        int expect = 0;
        if (g.compare_exchange_weak(
                expect, 2, std::memory_order::acq_rel, std::memory_order::acquire
            ))
            return 1;

        if (expect & 1) return 0;
        g.wait(2, std::memory_order::relaxed);
    }
}

extern "C" void __cxa_guard_release(int* guard) noexcept {
    std::atomic_ref g(*guard);
    auto old = g.exchange(1, std::memory_order::release);
    if (old & 2) g.notify_all();
}

extern "C" void __cxa_guard_abort(int* guard) noexcept {
    std::atomic_ref g(*guard);
    auto old = g.exchange(0, std::memory_order::release);
    if (old & 2) g.notify_all();
}

void _Exit(int) {
    while (1);
}

extern "C" {

void __aeabi_memcpy(void* dest, void const* src, size_t n) {
    memcpy(dest, src, n);
}
void __aeabi_memcpy4(void* dest, void const* src, size_t n) {
    memcpy(dest, src, n);
}
void __aeabi_memcpy8(void* dest, void const* src, size_t n) {
    memcpy(dest, src, n);
}

void __aeabi_memmove(void* dest, void const* src, size_t n) {
    memmove(dest, src, n);
}
void __aeabi_memmove4(void* dest, void const* src, size_t n) {
    memmove(dest, src, n);
}
void __aeabi_memmove8(void* dest, void const* src, size_t n) {
    memmove(dest, src, n);
}

// Note the reversed order of 'n' and 'c'
void __aeabi_memset(void* dest, size_t n, int c) {
    memset(dest, c, n);
}
void __aeabi_memset4(void* dest, size_t n, int c) {
    memset(dest, c, n);
}
void __aeabi_memset8(void* dest, size_t n, int c) {
    memset(dest, c, n);
}

void __aeabi_memclr(void* dest, size_t n) {
    memset(dest, 0, n);
}
void __aeabi_memclr4(void* dest, size_t n) {
    memset(dest, 0, n);
}
void __aeabi_memclr8(void* dest, size_t n) {
    memset(dest, 0, n);
}

}  // extern "C"
