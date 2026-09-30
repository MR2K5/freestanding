

#include <array>
#include <memory>
#include <ranges>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <string>

long long volatile write = 0;
long long volatile r2    = 65;
long long volatile r1    = 45;

static void print(char const* s) {
    register auto r0 asm("r0") = 0x04;
    register auto r1 asm("r1") = s;
    asm volatile("bkpt 0xAB" : : "r"(r0), "r"(r1));
}

struct A {
    ~A() { write = 1; }
};

[[gnu::used]] extern long long test(long long a, long long b) {
    return a / b;
}

constexpr size_t test2() {
    std::string a("aa");
    a.assign("Hello");
    a.insert(1, "moi");
    return a.size();
}


extern "C" int main() {
    static constexpr auto x = test2();

    int arr[x];

    constexpr auto u = std::bit_expand(0b1101u, 0b1011u);

    return 0;
}

