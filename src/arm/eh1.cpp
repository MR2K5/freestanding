#include "arm_eh.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <unwind.h>
#include <utility>

extern "C" {

static constexpr uint32_t EXIDX_CANTUNWIND = 0x1;

struct [[clang::internal_linkage]] exidx_entry {
    uint32_t fn_offset;
    uint32_t data;
};

extern exidx_entry const __arm_exidx_start[];
extern exidx_entry const __arm_exidx_end[];

// The returned value has also th ethumb bit
uintptr_t prel31(uint32_t const* place) {
    int32_t SAP = ((int32_t)(*place << 1)) >> 1;
    auto P      = reinterpret_cast<uintptr_t>(place);
    return static_cast<uintptr_t>(SAP) + P;
}

// TODO check that pc is in .text
static exidx_entry const* find_exidx(uintptr_t pc) {
    pc &= ~1u;

    auto found = std::ranges::upper_bound(
        __arm_exidx_start, __arm_exidx_end, pc, {},
        [](exidx_entry const& e) -> uintptr_t { return prel31(&e.fn_offset) & ~1; }
    );
    if (found <= __arm_exidx_start) return nullptr;
    return found - 1;
}
}

[[gnu::naked]] _Unwind_Reason_Code _Unwind_RaiseException(_Unwind_Control_Block* ucbp) noexcept {
    asm(R"(
        push {lr}
        sub sp, sp, %[size]
        
        stmia sp,{r0-r12}
        add r12, sp, %[size] + 4
        str r12, [sp, #52]
        str lr, [sp, #56]
        str lr, [sp, #60]
        
        add r12, sp, #64
        )"
#if defined(__ARM_FP) && __ARM_FP & 0x02
        "vstmia r12, {d0-d15}"
#endif
        R"(
        mov r1, sp
        bl unwind_raise_impl
        add sp, sp, %[size]
        pop {pc}
    )"
        :
        : [size] "i"(sizeof(_Unwind_Context)));
}

[[gnu::naked, clang::nouwtable, noreturn]] static void
unwind_install_context(_Unwind_Context const* ctx) {
    asm(
#if defined(__ARM_FP) && __ARM_FP & 0x02
        R"(
        add r2, r0, #64
        vldm r2, {d0-d15}
        )"
#endif
        // Load all regs. First r1-r12, lr. Then we can create a new sp at target - 8, write r0 and
        // pc there, and install the sp. and then pop r11 = target sp - 12  [ r0, r11, r12, pc ]
        R"(
            mov r12, r0
            ldm r12, {r0-r10}
            ldr lr, [r12, #56]
            
            ldr r11, [r12, #52]
            sub r11, r11, #16

            str r0, [r11]
            ldr r0, [r12, #44]
            str r0, [r11, #4]
            ldr r0, [r12, #48]
            str r0, [r11, #8]
            ldr r0, [r12, #60]
            str r0, [r11, #12]
            
            mov sp, r11
            pop {r0, r11, r12, pc}
        )"
    );
}

static std::pair<PersonalityRoutine, uint32_t const*> resolve_entry(uint32_t const* data) {
        // 2. Inlined Compact Model in .ARM.exidx
    if (*data & 0x80000000) {
        // Must be PR0: top byte == 0x80
        if ((*data >> 24) != 0x80) {
            std::abort(); // Corrupt or non-PR0 inlined entry
        }
        return {__aeabi_unwind_cpp_pr0, data};
    }

    // 3. Follow PREL31 offset into .ARM.extab
    auto const* extab = reinterpret_cast<uint32_t const*>(prel31(data));

    // 4. Compact Model in .ARM.extab (PR0, PR1, or PR2)
    if (*extab & 0x80000000) {
        uint32_t header = *extab >> 24;
        if ((header & 0xF0) != 0x80) {
            std::abort(); // Invalid compact model format
        }

        PersonalityRoutine pr = nullptr;
        switch (header & 0x0F) {
        case 0: pr = __aeabi_unwind_cpp_pr0; break;
        case 1: pr = __aeabi_unwind_cpp_pr1; break;
        case 2: pr = __aeabi_unwind_cpp_pr2; break;
        default: std::abort();
        }
        return {pr, extab};
    }

    // 5. Generic Model in .ARM.extab
    // Word 0 is a PREL31 reference to the custom personality routine (e.g. __gxx_personality_v0)
    auto pr = reinterpret_cast<PersonalityRoutine>(prel31(extab));
    return {pr, extab + 1};
}

// Basically phase 2
[[noreturn]] static void _Unwind_Next_Frame(_Unwind_Control_Block* ucpb, _Unwind_Context* ctx) {
    while (1) {
        _Unwind_Reason_Code ret;
        PersonalityRoutine pr;

        if (ucpb->unwinder_cache.reserved1 == 0) {
            auto entry = find_exidx(ctx->regs[15] - 2);
            if (!entry || entry->data == EXIDX_CANTUNWIND) std::abort();

            uint32_t const* ehtp;
            std::tie(pr, ehtp) = resolve_entry(&entry->data);
            ucpb->pr_cache     = {
                .fnstart    = prel31(&entry->fn_offset),
                .ehtp       = const_cast<uint32_t*>(ehtp),
                .additional = ehtp == &entry->data ? 1u : 0u,  // Set bit 0 if inlined
                .reserved1  = 0
            };

            ucpb->unwinder_cache.reserved2 = ctx->regs[15];
            ret                            = pr(_US_UNWIND_FRAME_STARTING, ucpb, ctx);
        } else {
            // We have a cached pr coming from Resume
            // pr_cache is already set
            
            pr = reinterpret_cast<PersonalityRoutine>(
                std::exchange(ucpb->unwinder_cache.reserved1, 0)
            );
            ret = pr(_US_UNWIND_FRAME_RESUME, ucpb, ctx);
        }

        if (ret == _URC_FAILURE) std::abort();
        if (ret == _URC_CONTINUE_UNWIND) continue;
        if (ret == _URC_INSTALL_CONTEXT) {
            // Cache the entry
            ucpb->unwinder_cache.reserved1 = reinterpret_cast<uintptr_t>(pr);
            unwind_install_context(ctx);
        }
        std::unreachable();
    }
}

extern "C" [[gnu::used]] _Unwind_Reason_Code
unwind_raise_impl(_Unwind_Control_Block* ucpb, _Unwind_Context* ctx) {
    auto initial_ctx = *ctx;

    while (1) {
        _Unwind_Reason_Code ret;
        auto entry = find_exidx(ctx->regs[15] - 2);
        if (!entry || entry->data == EXIDX_CANTUNWIND) return _URC_FAILURE;

        auto [pr, data] = resolve_entry(&entry->data);

        ucpb->pr_cache = {
            .fnstart    = prel31(&entry->fn_offset),
            .ehtp       = const_cast<uint32_t*>(data),
            .additional = data == &entry->data ? 1u : 0u,  // Set bit 0 if inlined
            .reserved1  = 0
        };

        ret = pr(_US_VIRTUAL_UNWIND_FRAME, ucpb, ctx);

        if (ret == _URC_CONTINUE_UNWIND) continue;
        if (ret == _URC_HANDLER_FOUND) break;
        if (ret == _URC_FAILURE) return ret;
        std::unreachable();
    }

    _Unwind_Next_Frame(ucpb, &initial_ctx);
}

extern "C" [[noreturn, gnu::used]] _Unwind_Reason_Code
unwind_resume_impl(_Unwind_Control_Block* ucpb, _Unwind_Context* ctx) noexcept {
    ctx->regs[15] = ucpb->unwinder_cache.reserved2;

    _Unwind_Next_Frame(ucpb, ctx);
}

[[gnu::naked, noreturn]] void _Unwind_Resume(_Unwind_Control_Block* ucbp) {
    asm(R"(
        push {lr}
        sub sp, sp, %[size]
        
        stmia sp,{r0-r12}
        add r12, sp, %[size] + 4
        str r12, [sp, #52]
        str lr, [sp, #56]
        str lr, [sp, #60]
        
        add r12, sp, #64
        )"
#if defined(__ARM_FP) && __ARM_FP & 0x02
        "vstmia r12, {d0-d15}"
#endif
        R"(
        mov r1, sp
        b unwind_resume_impl
    )"
        :
        : [size] "i"(sizeof(_Unwind_Context)));
}

void _Unwind_DeleteException(_Unwind_Control_Block* ucbp) {
    if (ucbp->exception_cleanup) ucbp->exception_cleanup(_URC_FOREIGN_EXCEPTION_CAUGHT, ucbp);
}

_Unwind_VRS_Result _Unwind_VRS_Set(
    _Unwind_Context* context, _Unwind_VRS_RegClass regclass, uint32_t regno,
    _Unwind_VRS_DataRepresentation representation, void* valuep
) {
    switch (regclass) {
    case _UVRSC_CORE:
        if (regno > 15 || representation != _UVRSD_UINT32) return _UVRSR_FAILED;
        std::memcpy(&context->regs[regno], valuep, sizeof(uint32_t));
        break;
#if defined(__ARM_FP) && __ARM_FP & 0x02
    case _UVRSC_VFP:
        switch (representation) {
        case _UVRSD_UINT32:
        case _UVRSD_FLOAT:
            if (regno >= 32) return _UVRSR_FAILED;
            std::memcpy(&context->vfp[regno], valuep, sizeof(uint32_t));
            break;
        case _UVRSD_DOUBLE:
            if (regno >= 16) return _UVRSR_FAILED;
            std::memcpy(&context->vfp[regno * 2], valuep, sizeof(uint64_t));
            break;
        default: return _UVRSR_NOT_IMPLEMENTED;
        }
        break;
#endif
    default: return _UVRSR_NOT_IMPLEMENTED;
    }

    return _UVRSR_OK;
}

_Unwind_VRS_Result _Unwind_VRS_Get(
    _Unwind_Context* context, _Unwind_VRS_RegClass regclass, uint32_t regno,
    _Unwind_VRS_DataRepresentation representation, void* valuep
) {
    switch (regclass) {
    case _UVRSC_CORE:
        if (representation != _UVRSD_UINT32 || regno >= 16) return _UVRSR_FAILED;
        std::memcpy(valuep, &context->regs[regno], sizeof(uint32_t));
        break;
    case _UVRSC_VFP:
        switch (representation) {
        case _UVRSD_VFPX: return _UVRSR_NOT_IMPLEMENTED;
        case _UVRSD_DOUBLE:
            if (regno >= 16) return _UVRSR_FAILED;
            std::memcpy(valuep, &context->vfp[regno * 2], sizeof(uint64_t));
            break;
        default: return _UVRSR_NOT_IMPLEMENTED;
        }
        break;
    default: return _UVRSR_NOT_IMPLEMENTED;
    }
    return _UVRSR_OK;
}

_Unwind_VRS_Result _Unwind_VRS_Pop(
    _Unwind_Context* context, _Unwind_VRS_RegClass regclass, uint32_t discriminator,
    _Unwind_VRS_DataRepresentation representation
) {
    switch (regclass) {

    case _UVRSC_CORE: {
        if (discriminator & 0xffff0000 || representation != _UVRSD_UINT32) return _UVRSR_FAILED;
        uint32_t const* sp = reinterpret_cast<uint32_t const*>(context->regs[13]);
        for (uint32_t r = 0; r < 16; ++r) {
            if (discriminator & (1u << r)) { context->regs[r] = *sp++; }
        }

        // EHABI Rule: If R_SP (bit 13) was explicitly popped from the stack,
        // keep that popped value. Otherwise, update SP to reflect the final pointer.
        if (!(discriminator & (1u << 13))) { context->regs[13] = reinterpret_cast<uint32_t>(sp); }
        break;
    }
    case _UVRSC_VFP: {
        if (representation != _UVRSD_DOUBLE) return _UVRSR_FAILED;
        uint32_t const* sp = reinterpret_cast<uint32_t const*>(context->regs[13]);
        auto base          = discriminator >> 16;
        auto n             = discriminator & 0xffff;

        if (n == 0 || base + n > 16) return _UVRSR_FAILED;

        std::memcpy(&context->vfp[base * 2], sp, sizeof(uint64_t) * n);

        context->regs[13] = reinterpret_cast<uint32_t>(sp + n * 2);
        break;
    }

    default: return _UVRSR_NOT_IMPLEMENTED;
    }
    return _UVRSR_OK;
}

void _Unwind_Complete(_Unwind_Control_Block* ucpb) {
    ucpb->unwinder_cache.reserved1 = 0;
    ucpb->unwinder_cache.reserved2 = 0;
    // Needto preserve 4,5 for linked list
    ucpb->barrier_cache  = {};
    ucpb->cleanup_cache  = {};
    ucpb->barrier_cache  = {};
}

