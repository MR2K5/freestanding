
#include <cstring>
#include <cxxabi.h>
#include <expected>

#include <avr/io.hpp>



[[gnu::used, gnu::retain]] int main() {
    avr::regfile[70] = std::byte(8);
}
