#include <cassert>
#include <cstdlib>

void __assert_fail(
    [[maybe_unused]] char const* file, [[maybe_unused]] char const* expr,
    [[maybe_unused]] char const* func, [[maybe_unused]] int line
) {
    std::_Exit(2);
}
