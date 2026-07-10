#include <cstdint>
#include <cstdlib>
#include <cxxabi.h>

namespace __cxxabiv1 {

// A structural look at how GCC's libsupc++/guard.cc implements the single-thread track
int __cxa_guard_acquire(std::int64_t* guard_object) {
    char* bytes           = reinterpret_cast<char*>(guard_object);
    char initialized      = bytes[0];
    char init_in_progress = bytes[1];

    // 1. If already fully initialized, tell the compiler to bypass the constructor
    if (initialized) { return 0; }

    // 2. Detect recursive initialization loops!
    if (init_in_progress) {
        // Triggers a call to __throw_runtime_error() or an internal abort trap
        // __throw_recursive_init_exception();
        std::_exit();
    }

    // 3. Lock the gate: Mark that initialization has officially started
    bytes[1] = 1;

    // Return 1 signals the current execution thread to execute the constructor code
    return 1;
}

void __cxa_guard_release(std::int64_t* guard_object) {
    char* bytes = reinterpret_cast<char*>(guard_object);

    bytes[0] = 1;  // Mark as fully initialized (Fast-path gate opens forever)
    bytes[1] = 0;  // Clear the initialization lock flag
}

void __cxa_guard_abort(std::int64_t* guard_object) {
    char* bytes = reinterpret_cast<char*>(guard_object);

    // If the constructor fails or throws, reset the lock byte so someone else can try
    bytes[1] = 0;
}
}  // namespace __cxxabiv1
