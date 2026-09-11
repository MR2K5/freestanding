#include "arm_eh.hpp"

#include <atomic>
#include <bit>
#include <cstdint>
#include <cstring>
#include <cxxabi.h>
#include <exception>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <string_view>
#include <typeinfo>
#include <unwind.h>
#include <utility>

enum class exec_result { ok, finish, fail };

static std::optional<uint32_t> read_uleb(std::span<uint8_t const>& ops) {
    uint32_t res   = 0;
    uint32_t shift = 0;

    for (int i = 0; i < 5; ++i) {
        if (ops.empty()) return std::nullopt;

        uint8_t byte = ops[0];
        ops          = ops.subspan(1);

        // Too big
        if (shift == 28 && (byte & 0x70) != 0) return std::nullopt;

        res |= uint32_t(byte & 0x7f) << shift;

        if ((byte & 0x80) == 0) return res;
        shift += 7;
    }
    return std::nullopt;
}

// Execute a single instruction from
static exec_result
exec_instr(_Unwind_Context* ctx, std::span<uint8_t const>& ops, bool& pc_updated) {
    using enum exec_result;

    auto read = [&]() -> std::optional<uint8_t> {
        if (ops.empty()) return std::nullopt;
        auto r = ops.front();
        ops    = ops.subspan(1);
        return r;
    };

    auto op = read();
    if (!op) return finish;  // Implicit finish

    switch (*op >> 6) {
    case 0b00: ctx->regs[13] += ((*op & 0x3f) << 2) + 4; return ok;
    case 0b01: ctx->regs[13] -= ((*op & 0x3f) << 2) + 4; return ok;
    case 0b10:
        switch ((*op >> 4) & 0b11) {
        case 0b00: {
            auto op2 = read();
            if (!op2) return fail;
            uint32_t mask = uint32_t(*op2) << 4 | uint32_t(*op & 0x0f) << 12;
            if (mask == 0) return fail;

            auto r = _Unwind_VRS_Pop(ctx, _UVRSC_CORE, mask, _UVRSD_UINT32);
            if (r != _UVRSR_OK) return fail;

            if (mask & (1u << 15)) pc_updated = true;

            return ok;
        }
        case 0b01:
            switch (*op & 0x0f) {
            case 13:
            case 15: return fail;
            default: ctx->regs[13] = ctx->regs[*op & 0x0f]; return ok;
            }
        case 0b10: {
            int n         = *op & 0x7;
            uint32_t mask = ((1u << (n + 1)) - 1) << 4;
            if (*op & 0x08) mask |= (1u << 14);

            auto r = _Unwind_VRS_Pop(ctx, _UVRSC_CORE, mask, _UVRSD_UINT32);
            if (r != _UVRSR_OK) return fail;

            return ok;
        }
        case 0b11:
            switch (*op & 0x0f) {
            case 0b0000:
                // Finish
                return finish;
            case 0b0001: {
                auto op2 = read();
                if (!op2 || *op2 == 0 || ((*op2 & 0xf0) != 0)) return fail;
                uint32_t mask = *op2 & 0x0f;

                auto r = _Unwind_VRS_Pop(ctx, _UVRSC_CORE, mask, _UVRSD_UINT32);
                if (r != _UVRSR_OK) return fail;

                return ok;
            }
            case 0b0010: {
                auto uleb = read_uleb(ops);
                if (!uleb) return fail;

                constexpr uint32_t max_uleb = (0xFFFFFFFFu - 0x204u) >> 2;
                if (*uleb > max_uleb) return fail;

                ctx->regs[13] += 0x204 + (*uleb << 2);
                return ok;
            }
            default: return fail;  // Not implemented or spare
            }
        }

    case 0b11:
        switch ((*op >> 3) & 0x07) {
        case 0b000: return fail;  // Intel wmmx. Not implemented
        case 0b001: {
            auto yyy = *op & 0x07;
            if (yyy > 1) return fail;  // spare
            auto op2 = read();
            if (!op2) return fail;

            uint32_t base  = (*op2 & 0xf0) >> 4;
            uint32_t count = (*op2 & 0x0f) + 1;

            if ((*op & 1) == 0) base += 16;
            uint32_t mask = base << 16 | count;

            auto r = _Unwind_VRS_Pop(ctx, _UVRSC_VFP, mask, _UVRSD_DOUBLE);
            if (r != _UVRSR_OK) return fail;

            return ok;
        }
        case 0b010: {
            auto nnn  = *op & 0x07;
            auto mask = uint32_t(8) << 16 | (nnn + 1);
            auto r    = _Unwind_VRS_Pop(ctx, _UVRSC_VFP, mask, _UVRSD_DOUBLE);
            if (r != _UVRSR_OK) return fail;
            return ok;
        }
        default: return fail;
        }
    }
    return fail;
}

static _Unwind_Reason_Code handle_descriptors(
    _Unwind_State st, _Unwind_Control_Block* ucbp, _Unwind_Context* ctx, uint32_t const* start,
    bool is_16
) {
    auto fnstart = ucbp->pr_cache.fnstart & ~1u;
    auto pc      = (ctx->regs[15] & ~1u) - 2;
    auto rel_pc  = pc - fnstart;

    while (*start != 0) {
        uint32_t raw_l, raw_s;
        if (is_16) {
            raw_l = *start & 0xffff;
            raw_s = *start++ >> 16;
        } else {
            raw_l = *start++;
            raw_s = *start++;
        }

        uint32_t kind = ((raw_l & 1u) << 1) | (raw_s & 1u);
        uint32_t l    = raw_l & ~1u;
        uint32_t s    = raw_s & ~1u;

        bool in_scope = (rel_pc >= s) && (rel_pc < s + l);

        switch (kind) {
        case 0b00: {
            uint32_t const* lp_ptr = start++;

            if (in_scope && st != _US_VIRTUAL_UNWIND_FRAME) {
                ucbp->cleanup_cache.bitpattern[0] = reinterpret_cast<uint32_t>(start);
                abi::__cxa_begin_cleanup(ucbp);
                ctx->regs[15] = prel31(lp_ptr);
                return _URC_INSTALL_CONTEXT;
            }
            break;
        }
        case 0b10: {
            uint32_t const* lp_ptr   = start++;
            uint32_t const* type_ptr = start++;

            if (in_scope) {
                bool is_ref = (*lp_ptr & (1u << 31));

                if (st == _US_VIRTUAL_UNWIND_FRAME) {
                    void* match = abi::__get_std_except_ptr(ucbp);

                    if (*type_ptr == 0xfffffffe) return _URC_FAILURE;

                    if (*type_ptr != 0xffffffff) {
                        auto const* rtti = reinterpret_cast<std::type_info const*>(*type_ptr);
                        if (abi::__cxa_type_match(ucbp, rtti, is_ref, &match) == abi::ctm_failed)
                            break;
                    }

                    ucbp->barrier_cache.sp            = ctx->regs[13];
                    ucbp->barrier_cache.bitpattern[0] = reinterpret_cast<uint32_t>(match);
                    ucbp->barrier_cache.bitpattern[1] = reinterpret_cast<uint32_t>(lp_ptr);
                    ucbp->barrier_cache.bitpattern[2] = 0;
                    return _URC_HANDLER_FOUND;
                } else if (
                    reinterpret_cast<uint32_t>(lp_ptr) == ucbp->barrier_cache.bitpattern[1]
                ) {
                    ctx->regs[0]  = reinterpret_cast<uint32_t>(ucbp);
                    ctx->regs[15] = prel31(lp_ptr) & ~(1u << 31);
                    return _URC_INSTALL_CONTEXT;
                }
            }
            break;
        }
        default: return _URC_FAILURE;
        }
    }

    return _URC_CONTINUE_UNWIND;
}

static _Unwind_Reason_Code personality(
    _Unwind_State state, _Unwind_Control_Block* ucbp, _Unwind_Context* context,
    std::span<uint8_t const> ops, uint32_t const* descriptors, bool is_16
) {
    if (state == _US_UNWIND_FRAME_RESUME) {
        descriptors = reinterpret_cast<uint32_t const*>(ucbp->cleanup_cache.bitpattern[0]);
    }

    if (descriptors) {
        auto r = handle_descriptors(state, ucbp, context, descriptors, is_16);
        if (r != _URC_CONTINUE_UNWIND) return r;
    }
    bool pc_updated = false;

    while (1) {
        auto r = exec_instr(context, ops, pc_updated);

        if (r == exec_result::fail) return _URC_FAILURE;
        if (r == exec_result::finish) {
            if (!pc_updated) context->regs[15] = context->regs[14];
            return _URC_CONTINUE_UNWIND;
        }
    }
}

_Unwind_Reason_Code
__aeabi_unwind_cpp_pr0(_Unwind_State state, _Unwind_Control_Block* ucbp, _Unwind_Context* context) {

    uint32_t const* datap = ucbp->pr_cache.ehtp;
    uint32_t const* desc  = nullptr;

    if ((ucbp->pr_cache.additional & 1) == 0) { desc = datap + 1; }

    auto data = std::byteswap(*datap);
    auto ops  = std::span(reinterpret_cast<uint8_t const*>(&data) + 1, 3);

    return personality(state, ucbp, context, ops, desc, true);
}

_Unwind_Reason_Code
__aeabi_unwind_cpp_pr1(_Unwind_State state, _Unwind_Control_Block* ucbp, _Unwind_Context* context) {

    uint32_t const* datap = ucbp->pr_cache.ehtp;
    assert((ucbp->pr_cache.additional & 1) == 0);

    auto words = ((*datap >> 16) & 0xff) + 1;

    uint32_t* store = (uint32_t*)alloca(words * sizeof(uint32_t));

    for (auto i = 0uz; i < words; ++i) { store[i] = std::byteswap(datap[i]); }

    auto ops = std::span(reinterpret_cast<uint8_t const*>(store) + 2, words * 4 - 2);
    return personality(state, ucbp, context, ops, datap + words, true);
}

_Unwind_Reason_Code
__aeabi_unwind_cpp_pr2(_Unwind_State state, _Unwind_Control_Block* ucbp, _Unwind_Context* context) {

    uint32_t const* datap = ucbp->pr_cache.ehtp;
    assert((ucbp->pr_cache.additional & 1) == 0);

    auto words = ((*datap >> 16) & 0xff) + 1;

    uint32_t* store = (uint32_t*)alloca(words * sizeof(uint32_t));

    for (auto i = 0uz; i < words; ++i) { store[i] = std::byteswap(datap[i]); }

    auto ops = std::span(reinterpret_cast<uint8_t const*>(store) + 2, words * 4 - 2);
    return personality(state, ucbp, context, ops, datap + words, false);
}

extern "C" _Unwind_Reason_Code __gxx_personality_v0(
    _Unwind_State state,
    _Unwind_Control_Block* ucbp,
    _Unwind_Context* context
) {
    return __aeabi_unwind_cpp_pr1(state, ucbp, context);
}

constinit static char const exception_class[8] = "FREEC++";

static bool is_cpp_exception(_Unwind_Control_Block* ucbp) noexcept {
    return std::string_view(ucbp->exception_class, 8) == std::string_view(exception_class, 8);
}

using abi::__cxa_exception;

static abi::__cxa_exception* get_cxa_except(_Unwind_Control_Block* ucbp) noexcept {
    return reinterpret_cast<__cxa_exception*>(ucbp->unwinder_cache.reserved3);
}

abi::__cxa_type_match_result abi::__cxa_type_match(
    _Unwind_Control_Block* ucbp, std::type_info const* rttip, bool is_reference_type,
    void** matched_object
) noexcept {
    if (!is_cpp_exception(ucbp)) return abi::ctm_failed;

    auto cxa           = get_cxa_except(ucbp);
    auto const* thrown = cxa->exceptionType;

    if (!rttip || !thrown) return abi::ctm_failed;

    auto old = *matched_object;
    if (rttip->__can_catch(thrown, is_reference_type, *matched_object))
        return *matched_object == old ? ctm_succeeded : ctm_succeeded_with_ptr_to_base;
    return ctm_failed;
}

void* abi::__get_std_except_ptr(_Unwind_Control_Block* ucbp) noexcept {
    if (is_cpp_exception(ucbp))
        return std::launder(reinterpret_cast<abi::__cxa_exception*>(ucbp))
            ->exception;  // Pointer interconvertibility
    return nullptr;
}

static void exception_cleanup(_Unwind_Reason_Code code, _Unwind_Control_Block* ucbp) {
    auto exc = get_cxa_except(ucbp);
    if (code != _URC_FOREIGN_EXCEPTION_CAUGHT) std::__terminate(exc->terminateHandler);

    if (exc->exceptionDestructor) exc->exceptionDestructor(exc->exception);
}

void* abi::__cxa_allocate_exception(size_t sz) {
    sz += sizeof(__cxa_exception);

    auto ptr = static_cast<__cxa_exception*>(operator new(sz, std::nothrow));
    if (!ptr) std::terminate();

    auto obj = std::construct_at(ptr);

    obj->terminateHandler = std::get_terminate();

    std::memcpy(obj->ucb.exception_class, exception_class, 8);
    obj->ucb.unwinder_cache.reserved1 = 0;
    obj->ucb.unwinder_cache.reserved3 = reinterpret_cast<uint32_t>(obj);

    obj->ucb.exception_cleanup = exception_cleanup;

    return obj->exception;
}

__cxa_exception* cxa_from_except_ptr(void* ptr) noexcept {
    return reinterpret_cast<__cxa_exception*>(
        reinterpret_cast<uint32_t>(ptr) - offsetof(__cxa_exception, exception)
    );
}

void abi::__cxa_free_exception(void* except) noexcept {
    auto cxa = cxa_from_except_ptr(except);
    std::destroy_at(cxa);
    operator delete(cxa);
}

void abi::__cxa_call_terminate(_Unwind_Control_Block* ucbp) {
    if (ucbp)
        abi::__cxa_begin_catch(ucbp);
    if (ucbp && is_cpp_exception(ucbp)) {
        std::__terminate(get_cxa_except(ucbp)->terminateHandler);
    } else {
        std::terminate();
    }
}

using abi::__cxa_exception;

static _Unwind_Control_Block* get_ucb(__cxa_exception* exc) noexcept {
    if (is_cpp_exception(&exc->ucb)) {
        return &exc->ucb;
    } else {
        // Need to get the original UCB, as exc was allocated separately. exc->ucb is not used for
        // unwinding ever. Use unwinder_cache.private1 as a pointer
        return reinterpret_cast<_Unwind_Control_Block*>(exc->ucb.unwinder_cache.reserved1);
    }
}

static __cxa_exception* get_or_create_exception(_Unwind_Control_Block* ucbp) {
    auto ptr = reinterpret_cast<__cxa_exception*>(ucbp->unwinder_cache.reserved3);
    if (ptr) return ptr;

    // Need to allocate a marker
    ptr                   = new (std::nothrow) __cxa_exception{};  // NO nee for tail object
    ptr->terminateHandler = std::get_terminate();
    if (!ptr) std::terminate();

    ptr->refcount                  = 1;
    ucbp->unwinder_cache.reserved3 = reinterpret_cast<uint32_t>(ptr);  // Ref to the marker
    std::memcpy(ptr->ucb.exception_class, ucbp->exception_class, 8);
    // Store the original UCB in the marker
    ptr->ucb.unwinder_cache.reserved1 = reinterpret_cast<uint32_t>(ucbp);

    return ptr;
}

void* abi::__cxa_begin_catch(_Unwind_Control_Block* ucbp) {
    __cxa_globals.uncaught_exceptions -= 1;

    auto cxa = get_or_create_exception(ucbp);

    // Un-negate if caught from rethrow (-1 -> 2), or increment if normal/reclaimed (0 -> 1)
    cxa->handlerCount = (cxa->handlerCount < 0) ? -cxa->handlerCount + 1 : cxa->handlerCount + 1;

    // Link only if not already at the head (avoids self-referencing loops on rethrow)
    if (abi::__cxa_globals.caught_exceptions != cxa) {
        cxa->nextCaughtException = std::exchange(abi::__cxa_globals.caught_exceptions, cxa);
    }

    auto* r = reinterpret_cast<void*>(ucbp->barrier_cache.bitpattern[0]);
    _Unwind_Complete(ucbp);
    return r;
}

void abi::__cxa_end_catch() {
    auto* top = abi::__cxa_globals.caught_exceptions;
    if (!top) return;

    if (top->handlerCount < 0) {
        if (!is_cpp_exception(&top->ucb)) {
            // We need to unconditionally free the marker and unlink
            __cxa_globals.caught_exceptions = top->nextCaughtException;
            auto ucb                        = get_ucb(top);
            ucb->unwinder_cache.reserved3   = 0;
            delete top;
            return;
        }

        // Exiting via rethrow (throw;): Increment count toward 0 (-1 -> 0), but keep on
        // caught_exceptions so outer catches reuse it
        top->handlerCount += 1;
        return;
    }

    // Normal exit (not rethrown)
    top->handlerCount -= 1;
    if (top->handlerCount == 0) {
        abi::__cxa_globals.caught_exceptions = top->nextCaughtException;
        __cxa_decrement_exception_refcount(top->exception);
    }
}

void abi::__cxa_rethrow() {
    auto* top = abi::__cxa_globals.caught_exceptions;
    if (!top) std::terminate();

    // Mark as in-flight rethrow
    if (top->handlerCount > 0) { top->handlerCount = -top->handlerCount; }
    abi::__cxa_globals.uncaught_exceptions += 1;

    _Unwind_Control_Block* ucbp = get_ucb(top);

    _Unwind_RaiseException(ucbp);
    std::__terminate(top->terminateHandler);
}

bool abi::__cxa_begin_cleanup(_Unwind_Control_Block* ucbp) {
    if (!ucbp) return false;

    if (__cxa_globals.propagating_exceptions != ucbp) {
        ucbp->unwinder_cache.reserved4 = 1;
        ucbp->unwinder_cache.reserved5 =
            reinterpret_cast<uint32_t>(__cxa_globals.propagating_exceptions);
        __cxa_globals.propagating_exceptions = ucbp;
    } else {
        ucbp->unwinder_cache.reserved4 += 1;
    }

    return true;
}

extern "C" _Unwind_Control_Block* __cxa_end_cleanup_impl() {
    auto* top = abi::__cxa_globals.propagating_exceptions;
    if (!top) std::terminate();

    top->unwinder_cache.reserved4 -= 1;
    if (top->unwinder_cache.reserved4 == 0) {
        abi::__cxa_globals.propagating_exceptions =
            reinterpret_cast<_Unwind_Control_Block*>(top->unwinder_cache.reserved5);
    }
    return top;
}

[[gnu::naked]] void abi::__cxa_end_cleanup() {
    asm(
        R"(
        push {r0-r3, r12, lr}
        bl __cxa_end_cleanup_impl
        mov r12, r0
        pop {r0-r3}
        mov r0, r12
        pop {r12, lr}
        b _Unwind_Resume
        )"
    );
}

void abi::__cxa_increment_exception_refcount(void* exc) noexcept {
    if (!exc) return;
    auto cxa = cxa_from_except_ptr(exc);

    std::atomic_ref refs(cxa->refcount);
    refs.store_add(1, std::memory_order::relaxed);
}

void abi::__cxa_decrement_exception_refcount(void* exc) noexcept {
    if (!exc) return;
    auto cxa = cxa_from_except_ptr(exc);

    std::atomic_ref refs(cxa->refcount);
    if (refs.fetch_sub(1, std::memory_order::acq_rel) == 1) {
        if (is_cpp_exception(&cxa->ucb)) {
            if (cxa->exceptionDestructor) cxa->exceptionDestructor(cxa->exception);
            abi::__cxa_free_exception(cxa->exception);
        } else {
            auto f = get_ucb(cxa);
            f->unwinder_cache.reserved3 = 0;
            _Unwind_DeleteException(f);
            std::destroy_at(cxa);
            ::operator delete(cxa);
        }
    }
}

std::exception_ptr std::current_exception() noexcept {
    auto top = abi::__cxa_globals.caught_exceptions;
    if (!top) return nullptr;

    if (!is_cpp_exception(&top->ucb)) {
        return make_exception_ptr(__foreign_exception(top->ucb.exception_class));
    }

    abi::__cxa_increment_exception_refcount(top->exception);
    return exception_ptr(top->exception);
}

void std::rethrow_exception(exception_ptr p) {
    // TODO threading?
    void* obj = p.__get();
    if (!obj) std::terminate();

    auto cxa = cxa_from_except_ptr(obj);
    abi::__cxa_increment_exception_refcount(obj);

    abi::__cxa_globals.uncaught_exceptions += 1;
    auto ucbp                               = get_ucb(cxa);

    ucbp->barrier_cache.sp            = 0;
    ucbp->barrier_cache.bitpattern[0] = 0;
    ucbp->barrier_cache.bitpattern[1] = 0;
    ucbp->barrier_cache.bitpattern[2] = 0;

    _Unwind_RaiseException(ucbp);
    abi::__cxa_call_terminate(ucbp);
}

// // FIXME will we now throw the inner or outer block of foreign exceptions?

// // BUG foreign exceptions are unlinked in end_catch, which means another marker is allocated on
// rethrow
