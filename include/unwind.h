#pragma once

#include <stdint.h>

#if defined (__arm__)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    _URC_OK                       = 0, /* operation completed successfully */
    _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
    _URC_HANDLER_FOUND            = 6,
    _URC_INSTALL_CONTEXT          = 7,
    _URC_CONTINUE_UNWIND          = 8,
    _URC_FAILURE                  = 9 /* unspecified failure of some kind */
} _Unwind_Reason_Code;

typedef uint32_t _Unwind_State;

static _Unwind_State const _US_VIRTUAL_UNWIND_FRAME  = 0;
static _Unwind_State const _US_UNWIND_FRAME_STARTING = 1;
static _Unwind_State const _US_UNWIND_FRAME_RESUME   = 2;

typedef struct _Unwind_Control_Block _Unwind_Control_Block;
typedef struct _Unwind_Context _Unwind_Context;
typedef uint32_t _Unwind_EHT_Header;

typedef struct _Unwind_Control_Block {
    char exception_class[8];
    void (*exception_cleanup)(_Unwind_Reason_Code, _Unwind_Control_Block*);

    /* Unwinder cache, private fields for the unwinder's use */
    struct {
        uint32_t reserved1; /* init reserved1 to 0, then don't touch. We cache the pr when
                               installing a ctx and read it after resume */
        uint32_t reserved2; /* Stores VRS[15] (pc) in phase 2 */
        uint32_t reserved3;
        uint32_t reserved4;
        uint32_t reserved5;
    } unwinder_cache;

    /* Propagation barrier cache (valid after phase 1): */
    struct {
        uint32_t sp;
        uint32_t bitpattern[5];
    } barrier_cache;

    /* Cleanup cache (preserved over cleanup): */
    struct {
        uint32_t bitpattern[4];
    } cleanup_cache;

    /* Pr cache (for pr's benefit): */
    struct {
        uint32_t fnstart;         /* function start address */
        _Unwind_EHT_Header* ehtp; /* pointer to EHT entry header word */
        uint32_t additional;      /* additional data */
        uint32_t reserved1;
    } pr_cache;
    long long int : 0; /* Force alignment of next item to 8-byte boundary */
} _Unwind_Control_Block;

/* Unwinding functions */
[[gnu::nothrow]] _Unwind_Reason_Code _Unwind_RaiseException(_Unwind_Control_Block* ucbp);
[[noreturn]] void _Unwind_Resume(_Unwind_Control_Block* ucbp);
void _Unwind_Complete(_Unwind_Control_Block* ucbp);
void _Unwind_DeleteException(_Unwind_Control_Block* ucbp);

typedef enum {
    _UVRSC_CORE   = 0, /* integer register */
    _UVRSC_VFP    = 1, /* vfp */
    _UVRSC_WMMXD  = 3, /* Intel WMMX data register */
    _UVRSC_WMMXC  = 4, /* Intel WMMX control register */
    _UVRSC_PSEUDO = 5  /* Special purpose pseudo register */
} _Unwind_VRS_RegClass;

typedef enum {
    _UVRSD_UINT32 = 0,
    _UVRSD_VFPX   = 1,
    _UVRSD_UINT64 = 3,
    _UVRSD_FLOAT  = 4,
    _UVRSD_DOUBLE = 5
} _Unwind_VRS_DataRepresentation;

typedef enum { _UVRSR_OK = 0, _UVRSR_NOT_IMPLEMENTED = 1, _UVRSR_FAILED = 2 } _Unwind_VRS_Result;

_Unwind_VRS_Result _Unwind_VRS_Set(
    _Unwind_Context* context, _Unwind_VRS_RegClass regclass, uint32_t regno,
    _Unwind_VRS_DataRepresentation representation, void* valuep
);

_Unwind_VRS_Result _Unwind_VRS_Get(
    _Unwind_Context* context, _Unwind_VRS_RegClass regclass, uint32_t regno,
    _Unwind_VRS_DataRepresentation representation, void* valuep
);

_Unwind_VRS_Result _Unwind_VRS_Pop(
    _Unwind_Context* context, _Unwind_VRS_RegClass regclass, uint32_t discriminator,
    _Unwind_VRS_DataRepresentation representation
);

typedef _Unwind_Reason_Code (*PersonalityRoutine)(
    _Unwind_State, _Unwind_Control_Block*, _Unwind_Context*
);

extern "C" {
_Unwind_Reason_Code
__aeabi_unwind_cpp_pr0(_Unwind_State state, _Unwind_Control_Block* ucbp, _Unwind_Context* context) noexcept;
_Unwind_Reason_Code
__aeabi_unwind_cpp_pr1(_Unwind_State state, _Unwind_Control_Block* ucbp, _Unwind_Context* context) noexcept;
_Unwind_Reason_Code
__aeabi_unwind_cpp_pr2(_Unwind_State state, _Unwind_Control_Block* ucbp, _Unwind_Context* context) noexcept;
}

#endif

#ifdef __cplusplus
}
#endif