#include <atomic>
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
