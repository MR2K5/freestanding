
#include <algorithm>
#include <bit>
#include <cstring>
#include <cxxabi.h>
#include <expected>

#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include <variant>

char* str;

struct A {
    int v;
};

[[gnu::used, gnu::retain]] int main() {
    std::array a{-2, 99, 0, -743, INT_MAX, 2, INT_MIN, 4};
    std::ranges::sort(a); 
    return 0;
}

extern "C" void _start() {
    main();
}
