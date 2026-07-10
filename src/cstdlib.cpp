#include <cstdlib>
#include <cxxabi.h>

// 1. Core Exit Functions
void abort() noexcept {
    _Exit(1);
}

void _Exit(int status) noexcept {
    _exit();
}

// 2. Quick Exit Handler Storage
// Using uint8_t is highly optimized for AVR's 8-bit registers
using handler = void (*)();
static constinit handler quick_handlers[16]{};
static constinit unsigned char qh_count = 0;

// 3. Register Handler
int at_quick_exit(void (*func)()) noexcept {
    if (qh_count < 16) {
        quick_handlers[qh_count] = func;
        qh_count++;
        return 0;  // Success
    }
    return 1;  // Failure (No space remaining)
}

// 4. Run Handlers and Exit
void quick_exit(int status) noexcept {
    (void)status;

    // Execute in strict LIFO (Last In, First Out) order
    while (qh_count > 0) {
        qh_count--;
        if (quick_handlers[qh_count]) { quick_handlers[qh_count](); }
    }

    _Exit(status);
}

int atexit(void (*func)()) noexcept {
    // TODO is this cast ok?
    __cxxabiv1::__cxa_atexit(reinterpret_cast<void (*)(void*)>(func), nullptr, nullptr);
    return 1;
}

void exit(int status) noexcept {
    // Run standard exit handlers in LIFO (Last In, First Out) order
    __cxxabiv1::__cxa_finalize(nullptr);
    _Exit(status);
}
