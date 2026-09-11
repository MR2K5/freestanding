// startup.cpp
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>

extern "C" {

// Linker script symbols
extern uint32_t __stack_top;
extern std::byte __data_load[];
extern std::byte __data_start[];
extern std::byte __data_end[];
extern std::byte __bss_start[];
extern std::byte __bss_end[];
extern void (*__init_array_start[])();
extern void (*__init_array_end[])();

extern std::byte __tdata_load[];
extern std::byte __tdata_start[];
extern std::byte __tdata_end[];
extern std::byte __tbss_start[];
extern std::byte __tbss_end[];

extern std::byte __heap_start[];
extern std::byte __heap_end[];

int main();

[[gnu::naked]] void reset_handler();
[[gnu::naked]] void default_handler();

// Fault & System Exception Aliases
void NMI_Handler() __attribute__((weak, alias("default_handler")));
void HardFault_Handler() __attribute__((weak, alias("default_handler")));
void MemManage_Handler() __attribute__((weak, alias("default_handler")));
void BusFault_Handler() __attribute__((weak, alias("default_handler")));
void UsageFault_Handler() __attribute__((weak, alias("default_handler")));
void SVC_Handler() __attribute__((weak, alias("default_handler")));
void DebugMon_Handler() __attribute__((weak, alias("default_handler")));
void PendSV_Handler() __attribute__((weak, alias("default_handler")));
void SysTick_Handler() __attribute__((weak, alias("default_handler")));
}

// Trap handler for faults and unhandled interrupts
extern "C" [[gnu::naked]] void default_handler() {
    __asm__ volatile(
        R"(bkpt #0
        b default_handler)"
    );
}

// Cortex-M4 Reset Handler
// Marked naked so GCC does not generate a function prologue that could touch
// uninitialized stack or emit VFP instructions before the FPU is active.
extern "C" [[gnu::naked]] void reset_handler() {
    __asm__ volatile(
        R"(.syntax unified
        .thumb

        @ 1. Enable FPU (CP10 and CP11 Full Access via SCB->CPACR: 0xE000ED88)
        ldr r0, =0xE000ED88
        ldr r1, [r0]
        orr r1, r1, #(0xF << 20)
        str r1, [r0]
        dsb
        isb
        eor lr,lr,lr

        b _start2
    )"
    );
}

extern "C" [[gnu::used]] void _start2() noexcept {
    std::memcpy(__data_start, __data_load, __data_end - __data_start);
    std::memset(__bss_start, 0, __bss_end - __bss_start);

    std::memcpy(__tdata_start, __tdata_load, __tdata_end - __tdata_start);
    std::memset(__tbss_start, 0, __tbss_end - __tbss_start);

    for (auto x = __init_array_start; x != __init_array_end; ++x) { (*x)(); }

    __std_init_heap(__heap_start, __heap_end - __heap_start);

    main();

    __cxxabiv1::__cxa_finalize(nullptr);
}

extern "C" [[gnu::naked]] void* __aeabi_read_tp() noexcept {
    asm volatile(R"(
        ldr r0, =__main_tcb
        bx lr
    )");
}

// Vector Table definition placed at Flash address 0x00000000
using isr_t = void (*)();

[[gnu::section(".vectors"), gnu::used]] isr_t const vector_table[] = {
    reinterpret_cast<isr_t>(&__stack_top),  // 0x00: Initial Main Stack Pointer (MSP)
    reset_handler,                          // 0x04: Reset Handler
    NMI_Handler,                            // 0x08: NMI
    HardFault_Handler,                      // 0x0C: HardFault
    MemManage_Handler,                      // 0x10: MemManage Fault
    BusFault_Handler,                       // 0x14: BusFault
    UsageFault_Handler,                     // 0x18: UsageFault
    nullptr,                                // 0x1C: Reserved
    nullptr,                                // 0x20: Reserved
    nullptr,                                // 0x24: Reserved
    nullptr,                                // 0x28: Reserved
    SVC_Handler,                            // 0x2C: SVCall
    DebugMon_Handler,                       // 0x30: Debug Monitor
    nullptr,                                // 0x34: Reserved
    PendSV_Handler,                         // 0x38: PendSV
    SysTick_Handler                         // 0x3C: SysTick
};
