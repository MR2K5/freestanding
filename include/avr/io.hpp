#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace avr {
using std::byte;

struct interrupt_guard {
    std::byte sreg;
    explicit interrupt_guard() noexcept { asm volatile("in %0, 0x3f\ncli" : "=&r"(sreg) : : "cc"); }

    ~interrupt_guard() { asm volatile("out 0x3f, %0" : : "r"(sreg) : "cc"); }
};

struct regfile_t {
    std::byte regs[32];

    std::byte operator[](unsigned idx) const volatile { return regs[idx]; }
    std::byte volatile& operator[](unsigned idx) volatile { return regs[idx]; }
};

inline regfile_t volatile& regfile = *reinterpret_cast<regfile_t volatile*>(0);

}  // namespace avr
