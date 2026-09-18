#include "arm_eh.hpp"

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
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

static std::optional<uint32_t> read_uleb(std::span<uint8_t const>& ops) noexcept {
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

[[nodiscard]] constexpr std::optional<intptr_t> read_sleb(std::span<uint8_t const>& sp) noexcept {
    intptr_t result = 0;
    unsigned shift  = 0;

    constexpr unsigned max_bits = sizeof(intptr_t) * 8;

    while (!sp.empty()) {
        uint8_t const byte = sp.front();
        sp                 = sp.subspan(1);

        // Extract 7 data bits and accumulate
        result |= static_cast<intptr_t>(byte & 0x7F) << shift;
        shift  += 7;

        // Bit 7 indicates whether more bytes follow
        if ((byte & 0x80) == 0) {
            // Sign-extend if the most significant bit of the last 7-bit payload is set
            if ((shift < max_bits) && (byte & 0x40)) {
                result |= -(static_cast<intptr_t>(1) << shift);
            }
            return result;
        }

        // Prevent infinite loops / UB shift overflow on malformed input
        if (shift >= max_bits) { return std::nullopt; }
    }

    // Buffer ran out before terminal byte (byte & 0x80 == 0)
    return std::nullopt;
}

// Execute a single instruction from
static exec_result
exec_instr(_Unwind_Context* ctx, std::span<uint8_t const>& ops, bool& pc_updated) noexcept {
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

static _Unwind_Reason_Code arm_exec_instr(_Unwind_Context* context, std::span<uint8_t const> ops) noexcept {
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
__aeabi_unwind_cpp_pr0(_Unwind_State, _Unwind_Control_Block* ucbp, _Unwind_Context* context) noexcept {
    uint32_t const* datap = ucbp->pr_cache.ehtp;

    auto data = std::byteswap(*datap);
    auto ops  = std::span(reinterpret_cast<uint8_t const*>(&data) + 1, 3);

    return arm_exec_instr(context, ops);
}

_Unwind_Reason_Code
__aeabi_unwind_cpp_pr1(_Unwind_State, _Unwind_Control_Block* ucbp, _Unwind_Context* context) noexcept {

    uint32_t const* datap = ucbp->pr_cache.ehtp;
    assert((ucbp->pr_cache.additional & 1) == 0);

    auto words = ((*datap >> 16) & 0xff) + 1;

    uint32_t* store = (uint32_t*)alloca(words * sizeof(uint32_t));

    for (auto i = 0uz; i < words; ++i) { store[i] = std::byteswap(datap[i]); }

    auto ops = std::span(reinterpret_cast<uint8_t const*>(store) + 2, words * 4 - 2);
    return arm_exec_instr(context, ops);
}

_Unwind_Reason_Code
__aeabi_unwind_cpp_pr2(_Unwind_State, _Unwind_Control_Block*, _Unwind_Context*) noexcept {
    return _URC_FAILURE;
}

enum encoding : uint8_t {
    DW_EH_PE_absptr  = 0x00,
    DW_EH_PE_udata2  = 0x02,
    DW_EH_PE_uleb128 = 0x01,
    DW_EH_PE_udata4  = 0x03,
    DW_EH_PE_udata8  = 0x04,
    DW_EH_PE_sleb128 = 0x09,
    DW_EH_PE_sdata2  = 0x0A,
    DW_EH_PE_sdata4  = 0x0B,
    DW_EH_PE_sdata8  = 0x0C,

    DW_EH_PE_pcrel   = 0x10,
    DW_EH_PE_textrel = 0x20,
    DW_EH_PE_datarel = 0x30,
    DW_EH_PE_funcrel = 0x40,
    DW_EH_PE_aligned = 0x50,

    DW_EH_PE_omit = 0xff,
};

static abi::__cxa_exception* get_cxa_except(_Unwind_Control_Block* ucbp) noexcept {
    return reinterpret_cast<abi::__cxa_exception*>(ucbp->unwinder_cache.reserved3);
}

static void const* read_target2(void const* ptr) noexcept {
    uintptr_t off;
    std::memcpy(&off, ptr, sizeof(uintptr_t));
    if (!off) return 0;
    return reinterpret_cast<void const*>(reinterpret_cast<uintptr_t>(ptr) + off);
}

static _Unwind_Reason_Code read_lsda(
    _Unwind_State st, _Unwind_Control_Block* ucbp, _Unwind_Context* ctx, uint8_t const* lsda
) noexcept {
    auto sp = std::span<uint8_t const>(lsda, 0xffff);

    if (st == _US_UNWIND_FRAME_STARTING && ctx->regs[13] == ucbp->barrier_cache.sp) {
        ctx->regs[0]  = reinterpret_cast<uint32_t>(ucbp);
        ctx->regs[1]  = ucbp->barrier_cache.bitpattern[1];
        ctx->regs[15] = ucbp->barrier_cache.bitpattern[2];
        return _URC_INSTALL_CONTEXT;
    }

    auto read_u8 = [&sp]() -> uint8_t {
        uint8_t val = sp.front();
        sp          = sp.subspan(1);
        return val;
    };

    // 1. LPStart Encoding & Base
    uint8_t lpstart_enc = read_u8();
    uintptr_t lpstart   = 0;

    if (lpstart_enc == DW_EH_PE_omit) {
        // Defaults to function start on ARM EHABI
        lpstart = ucbp->pr_cache.fnstart;
    } else {
        // If not omitted, read address according to encoding
        return _URC_FAILURE;  // (or handle explicit LPStart)
    }

    // 2. Type Table Encoding & Base
    uint8_t ttable_enc         = read_u8();
    uint8_t const* ttable_base = nullptr;

    if (ttable_enc != DW_EH_PE_omit) {
        auto off_opt = read_uleb(sp);  // advances 'sp' past the ULEB bytes
        if (!off_opt) return _URC_FAILURE;

        // Base is calculated from the byte immediately following the ULEB
        ttable_base = sp.data() + *off_opt;
    }

    // 3. Call Site Table Encoding & Length
    uint8_t callsite_enc  = read_u8();
    auto callsite_len_opt = read_uleb(sp);  // advances 'sp'
    if (!callsite_len_opt) return _URC_FAILURE;

    uint32_t callsite_len = *callsite_len_opt;
    auto callsite_table   = sp.first(callsite_len);
    sp                    = sp.subspan(callsite_len);

    // 'sp.data()' now points to the Action Table
    uint8_t const* action_table = sp.data();

    auto pc  = ctx->regs[15];
    pc      -= 2;
    pc      -= lpstart;

    struct {
        uintptr_t landing_pad = 0;
        uint32_t act_off      = 0;
        bool found            = false;
    } match;

    while (!callsite_table.empty()) {
        uint32_t cs_start  = read_uleb(callsite_table).value_or(0);
        uint32_t cs_len    = read_uleb(callsite_table).value_or(0);
        uint32_t cs_lp     = read_uleb(callsite_table).value_or(0);
        uint32_t cs_action = read_uleb(callsite_table).value_or(0);

        if (pc >= cs_start && pc < cs_start + cs_len) {
            match.landing_pad  = cs_lp == 0 ? 0 : lpstart + cs_lp;
            match.landing_pad |= (ctx->regs[15] & 1);
            match.act_off      = cs_action;
            match.found        = true;
            break;
        }
    }

    if (!match.found) return _URC_CONTINUE_UNWIND;

    if (match.act_off == 0) {
        // Cleanups
        if (st == _US_VIRTUAL_UNWIND_FRAME) return _URC_CONTINUE_UNWIND;
        ctx->regs[0]  = reinterpret_cast<uint32_t>(ucbp);
        ctx->regs[1]  = 0;
        ctx->regs[15] = match.landing_pad;
        return _URC_INSTALL_CONTEXT;
    }

    // Need to walk actions
    // In phase 2 we have catch stored in barrier_cache
    // if (st == _US_UNWIND_FRAME_STARTING) { return _URC_CONTINUE_UNWIND; }

    std::span action_sp(action_table + (match.act_off - 1), 0xffff);
    while (1) {
        auto type_filter    = *read_sleb(action_sp);
        uint8_t const* save = action_sp.data();
        auto action_next    = *read_sleb(action_sp);

        if (type_filter == 0) {
            // Cleanup
            ctx->regs[0]  = reinterpret_cast<uint32_t>(ucbp);
            ctx->regs[1]  = 0;
            ctx->regs[15] = match.landing_pad;
            return _URC_INSTALL_CONTEXT;
        }

        if (st == _US_UNWIND_FRAME_STARTING) continue;
        if (type_filter < 0) return _URC_FAILURE;

        auto thrown              = get_cxa_except(ucbp);
        std::type_info const* tp = static_cast<std::type_info const*>(
            read_target2(reinterpret_cast<void* const*>(ttable_base) - type_filter)
        );
        void* ptr = thrown->exception;

        if (tp == nullptr
            || __cxxabiv1::__cxa_type_match(ucbp, tp, false, &ptr) != abi::ctm_failed) {
            ucbp->barrier_cache.sp            = ctx->regs[13];
            ucbp->barrier_cache.bitpattern[0] = reinterpret_cast<uint32_t>(ptr);
            ucbp->barrier_cache.bitpattern[1] = static_cast<uint32_t>(type_filter);
            ucbp->barrier_cache.bitpattern[2] = match.landing_pad;
            return _URC_HANDLER_FOUND;
        }

        if (action_next == 0) break;
        uint8_t const* next_rec = save + action_next;
        action_sp               = {next_rec, 0xffff};
    }
    return _URC_CONTINUE_UNWIND;
}

extern "C" _Unwind_Reason_Code __gxx_personality_v0(
    _Unwind_State state, _Unwind_Control_Block* ucbp, _Unwind_Context* context
) noexcept {
    uint8_t const* lsda = reinterpret_cast<uint8_t const*>(ucbp->pr_cache.ehtp);
    auto ops_type       = lsda[3];
    size_t opcode_words = 1;
    if (ops_type == 1) opcode_words = lsda[2] + 1;
    lsda += opcode_words * sizeof(uint32_t);

    switch (state) {
    case _US_VIRTUAL_UNWIND_FRAME:
    case _US_UNWIND_FRAME_STARTING: {
        auto r = read_lsda(state, ucbp, context, lsda);
        if (r != _URC_CONTINUE_UNWIND) return r;
    }
        [[fallthrough]];
    case _US_UNWIND_FRAME_RESUME:
        return ops_type == 1 ? __aeabi_unwind_cpp_pr1(state, ucbp, context)
                             : __aeabi_unwind_cpp_pr0(state, ucbp, context);
        break;
    default: return _URC_FAILURE;
    }
}

constinit static char const exception_class[8] = "FREEC++";

static bool is_cpp_exception(_Unwind_Control_Block* ucbp) noexcept {
    return std::string_view(ucbp->exception_class, 8) == std::string_view(exception_class, 8);
}

using abi::__cxa_exception;
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

static void exception_cleanup(_Unwind_Reason_Code code, _Unwind_Control_Block* ucbp) noexcept {
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

void abi::__cxa_call_terminate(_Unwind_Control_Block* ucbp) noexcept {
    if (ucbp) abi::__cxa_begin_catch(ucbp);
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

static __cxa_exception* get_or_create_exception(_Unwind_Control_Block* ucbp) noexcept {
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

void* abi::__cxa_begin_catch(_Unwind_Control_Block* ucbp) noexcept {
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

void abi::__cxa_end_catch() noexcept {
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
            auto f                      = get_ucb(cxa);
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

/*
1. LSDA Header

The header defines the pointer encoding formats (using DWARF DW_EH_PE_* values) and the boundary
offsets of the other three tables. Plaintext

[LPStart Encoding]       : 1 byte  (DW_EH_PE_* format)
[LPStart Base]           : Optional (present only if LPStart Encoding != DW_EH_PE_omit)
[Type Table Encoding]    : 1 byte  (DW_EH_PE_* format)
[Type Table Offset]      : ULEB128 (present only if Type Table Encoding != DW_EH_PE_omit)
[Call Site Encoding]     : 1 byte  (DW_EH_PE_* format)
[Call Site Table Length] : ULEB128

    LPStart (Landing Pad Start): Defines the base address for landing pad offsets in the Call Site
Table. If omitted (0xFF), it defaults to the function's start address (from the .eh_frame FDE).

    Type Table Offset: A ULEB128 integer measuring the byte distance from immediately after this
ULEB128 field to the base/end of the Type Table.

    Call Site Encoding: Describes how integer fields in the Call Site Table are stored (commonly
0x01 for uleb128 or 0x03 for udata4).

    Call Site Table Length: The total byte length of the Call Site Table.

2. Call Site Table

A linear array of records that partitions the function's code into protected and unprotected ranges.

Each entry contains four fields:
Plaintext

[cs_start]  : <Call Site Encoding> (Offset from function start / LPStart)
[cs_length] : <Call Site Encoding> (Length of this monitored region)
[cs_lp]     : <Call Site Encoding> (Offset to Landing Pad code; 0 if none)
[cs_action] : ULEB128                (1-based byte index into Action Table; 0 = cleanup only)

How the Unwinder Uses It:

When an exception occurs at instruction pointer IP:

    The unwinder calculates the offset: offset = IP - Function_Start.

    It linearly scans the Call Site Table until it finds an entry where:
    cs_start≤offset<cs_start+cs_length

    It evaluates cs_lp and cs_action:

        cs_lp == 0 && cs_action == 0: Unprotected code. No catch or cleanup in this frame; keep
unwinding up the stack.

        cs_lp != 0 && cs_action == 0: Cleanups/destructors only. Jump to cs_lp to run RAII
destructors, then resume unwinding (_Unwind_Resume).

        cs_lp != 0 && cs_action != 0: There is at least one catch clause (or an exception
specification). Jump to Action_Table + (cs_action - 1) to inspect candidate types.

3. Action Table

The Action Table is a singly-linked list serialized into a compact byte stream. It chains together
all candidate catch clauses protecting a specific call site.

Each record consists of two signed LEB128 numbers:
Plaintext

[Type Filter] : SLEB128 (Index pointing to a type in the Type Table)
[Next Action] : SLEB128 (Relative signed byte offset to the next action record; 0 terminates)

Interpreting the Type Filter:

    Type Filter > 0 (Catch Handler):

    A positive 1-based index into the Type Table. The personality routine fetches the corresponding
std::type_info* and checks if the thrown exception can be converted to it.

    Type Filter == 0 (Cleanup):

    Indicates that RAII cleanup/destructors must execute.

    Type Filter < 0 (Exception Specification):

    A negative offset into the exception specification list (filters for legacy throw(T1, T2)
clauses or noexcept).

4. Type Table

The Type Table contains pointers to the RTTI type descriptor structures (std::type_info) referenced
by the catch clauses.

A critical design quirk of the Type Table is that it is indexed backwards:

    The Type Table Offset in the LSDA header points to the end (the base boundary) of the Type
Table.

    If an action record has Type Filter = 1, its descriptor is located at:
    Address=Type Table Base−(1×pointer_size)

    If Type Filter = 2, its descriptor is at:
    Address=Type Table Base−(2×pointer_size)

    catch (...) (Catch-All): Encoded as a null pointer entry (0x0) in the Type Table. When the
personality routine encounters a null type pointer, it matches unconditionally.

Execution Walkthrough: What Happens During throw
C++

void foo() {
    try {
        bar(); // Throws std::runtime_error
    } catch (const std::exception& e) {
        handle();
    }
}

    bar() throws. The unwinder walks to foo()'s frame and consults foo's LSDA in .gcc_except_table.

    Call Site Match: The call to bar() matches a call-site entry with cs_lp != 0 and cs_action = 1.

    Action Table Scan: The unwinder reads Action Record 0 (offset cs_action - 1 = 0). It finds Type
Filter = 1.

    Type Lookup: It reads Type_Table_Base - sizeof(void*) to get the pointer to _ZTISt9exception
(typeinfo for std::exception).

    Type Match Check: It asks __cxa_can_catch() if std::runtime_error can be caught as
std::exception. The check succeeds.

    Execution Phase: The unwinder sets the CPU instruction pointer to the landing pad (cs_lp) and
resumes user execution inside the catch block.
*/
