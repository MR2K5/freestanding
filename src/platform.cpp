#include <atomic>
#include <cstdlib>
#include <cxxabi.h>
#include <debugging>

namespace std {

bool is_debugger_present() noexcept {
    return false;
}

}  // namespace std

extern "C" [[gnu::weak]] void _Exit(int) noexcept {
    while (1);
}

namespace {
enum class state : uint32_t { unlocked, locked, waiting };
}

int abi::__cxa_guard_acquire(int64_t* p) noexcept {
    std::atomic_ref lock(*reinterpret_cast<state*>(p));
    std::atomic_ref flag(*reinterpret_cast<uint8_t*>(p));

    while (1) {
        state expected{};
        if (lock.compare_exchange_weak(
                expected, state::locked, std::memory_order::acquire, std::memory_order::relaxed
            )) {
            if (flag.load(std::memory_order::acquire) != 0) {
                lock.store(state::unlocked, std::memory_order::release);
                return 0;
            }
            return 1;
        }

        if (expected == state::locked) {
            lock.compare_exchange_weak(
                expected, state::waiting, std::memory_order::relaxed, std::memory_order::relaxed
            );
        }
        lock.wait(state::waiting, std::memory_order::relaxed);
    }
}

void abi::__cxa_guard_release(int64_t* p) noexcept {
    std::atomic_ref lock(*reinterpret_cast<state*>(p));
    std::atomic_ref flag(*reinterpret_cast<uint8_t*>(p));

    flag.store(1, std::memory_order::release);

    auto old = lock.exchange(state::unlocked, std::memory_order::release);
    if (old == state::waiting) { lock.notify_all(); }
}

void abi::__cxa_guard_abort(int64_t* p) noexcept {
    std::atomic_ref lock(*reinterpret_cast<state*>(p));

    auto old = lock.exchange(state::unlocked, std::memory_order::release);
    if (old == state::waiting) { lock.notify_all(); }
}
