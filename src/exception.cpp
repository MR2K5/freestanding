#include <__config.hpp>
#include <atomic>
#include <cstdlib>
#include <exception>

namespace std {

exception::~exception()         = default;
bad_exception::~bad_exception() = default;

// TODO Should we pass a message?
constinit static std::atomic<terminate_handler> handler_ = std::abort;

void __terminate(terminate_handler h) noexcept {
    _TRY {
        if (h)
            h();
        else
            std::abort();
    }
    _CATCHALL {
        std::abort();
    }
    std::abort();
}

void terminate() noexcept {
    __terminate(handler_.load(memory_order::acquire));
}

terminate_handler get_terminate() noexcept {
    return handler_.load(memory_order::acquire);
}

terminate_handler set_terminate(terminate_handler h) noexcept {
    return handler_.exchange(h, memory_order::acq_rel);
}

}  // namespace std
