

#include <array>
#include <ranges>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

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

extern "C" int main() {
    auto arr = std::array{1, 2, 3, 4};
    auto v   = std::ranges::views::cartesian_product(arr, arr);

    for (auto [a, b]: std::views::cartesian_product(arr, arr)) { write = a + b; }

    print("hello");

    return 0;
}

void test2() {
    main();
}
