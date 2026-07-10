#include <cassert>
#include <cstdlib>

void __assert_fail(const char *file, const char *expr, const char *func, int line) {
    std::_Exit(2);
}