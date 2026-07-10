#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace avr {

struct interrupt_guard {
    explicit interrupt_guard() noexcept { asm volatile("cli" : : : "memory"); }

    ~interrupt_guard() { asm volatile("sei" : : : "memory"); }
};

struct regfile_t {
    std::byte regs[32];

    std::byte operator[](unsigned idx) const volatile { return regs[idx]; }
    std::byte volatile& operator[](unsigned idx) volatile { return regs[idx]; }
};

inline regfile_t volatile& regfile = *reinterpret_cast<regfile_t volatile*>(0);

}  // namespace avr
