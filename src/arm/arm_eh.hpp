#pragma once

#include <array>
#include <stdint.h>
#include <typeinfo>

#include <unwind.h>

extern "C" {

struct _Unwind_Context {
    std::array<uint32_t, 16> regs;

#if defined(__ARM_FP) && __ARM_FP & 0x02
    std::array<uint32_t, 32> vfp;
#endif

    uint32_t flags;
};
static_assert(sizeof(_Unwind_Context) % 8 == 4);


uintptr_t prel31(uint32_t const* place);
}
