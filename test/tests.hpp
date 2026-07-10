#pragma once

[[noreturn]] inline void test_exit(int code) {
    asm volatile("syscall" : : "a"(0x3C), "D"(code) : "memory");
    __builtin_unreachable();
}

inline void print(char const* str) {
    auto len = __builtin_strlen(str);
    asm volatile(
        "syscall"
        :                         // No outputs
        : "a"(1),                 // rax = 1 (sys_write)
          "D"(2),                 // rdi = 1 (stdout file descriptor)
          "S"(str),               // rsi = pointer to string
          "d"(len)                // rdx = length of string
        : "rcx", "r11", "memory"  // syscall instruction clobbers rcx and r11
    );
}

inline void __do_test(bool val, char const* str, int line, char const* file, char const* func) {
    if (!val) {

        print("Test failed in ");
        print(func);
        print(":");
        print(file);
        print(": ");
        print(str);

        test_exit(1);
    }
}

#define TEST_ASSERT(...)                                                                           \
    __do_test(__VA_ARGS__, #__VA_ARGS__, __LINE__, __FILE__, __PRETTY_FUNCTION__)
