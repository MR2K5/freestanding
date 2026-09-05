#include <__config.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>

#include <atomic>
#include <stddef.h>

#if _STD_HAS_HEAP
#  include <tlsf.h>
#endif

namespace std {

static constinit atomic<new_handler> new_handler_;

new_handler set_new_handler(new_handler h) noexcept {
    return new_handler_.exchange(h, memory_order_acq_rel);
}
new_handler get_new_handler() noexcept {
    return new_handler_.load(memory_order::acquire);
}

#if _STD_HAS_HEAP

extern "C" {

static tlsf_t tlsf_;

void __std_init_heap(void* ptr, size_t size) noexcept {
    tlsf_ = tlsf_create_with_pool(ptr, size);
}
void* aligned_alloc(size_t alignment, size_t size) noexcept {
    return tlsf_memalign(tlsf_, alignment, size);
}
void* calloc(size_t nmemb, size_t size) noexcept {
    auto p = malloc(size * nmemb);
    if (p) memset(p, 0, nmemb * size);
    return p;
}
void free(void* ptr) noexcept {
    tlsf_free(tlsf_, ptr);
}
void free_sized(void* ptr, [[maybe_unused]] size_t size) noexcept {
    free(ptr);
}
void free_aligned_sized(
    void* ptr, [[maybe_unused]] size_t alignment, [[maybe_unused]] size_t size
) noexcept {
    return free(ptr);
}
void* malloc(size_t size) noexcept {
    return tlsf_memalign(tlsf_, alignof(max_align_t), size);
}
void* realloc(void* ptr, size_t size) noexcept {
    void* nw = malloc(size);
    if (nw) memcpy(nw, ptr, tlsf_block_size(ptr));
    return nw;
}
}

#endif
}  // namespace std

[[gnu::weak]] void* operator new(size_t sz) {
    while (1) {
#if _STD_HAS_HEAP
        void* p = malloc(sz);
#else
        void* p = nullptr;
#endif

        if (p) return p;
        auto handler = std::get_new_handler();
        if (!handler) {
#if _STD_HAS_EH
            throw std::bad_alloc();
#else
            std::abort();
#endif
        }
    }
}

[[gnu::weak]] void* operator new(size_t sz, std::align_val_t align) {
    while (1) {
#if _STD_HAS_HEAP
        void* p = aligned_alloc(size_t(align), sz);
#else
        void* p = nullptr;
#endif

        if (p) return p;
        auto handler = std::get_new_handler();
        if (!handler) {
#if _STD_HAS_EH
            throw std::bad_alloc();
#else
            std::abort();
#endif
        }
    }
}

void* operator new(std::size_t size, std::nothrow_t const&) noexcept {
    _TRY {
        return ::operator new(size);
    }
    _CATCHALL {
        return nullptr;
    }
}
void* operator new(std::size_t size, std::align_val_t alignment, std::nothrow_t const&) noexcept {
    _TRY {
        return ::operator new(size, alignment);
    }
    _CATCHALL {
        return nullptr;
    }
}

void operator delete(void* ptr) noexcept {
    if (ptr) free(ptr);
}
void operator delete(void* ptr, std::size_t) noexcept {
    ::operator delete(ptr);
}
void operator delete(void* ptr, std::align_val_t) noexcept {
    ::operator delete(ptr);
}
void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept {
    ::operator delete(ptr);
}
void operator delete(void* ptr, std::nothrow_t const&) noexcept {
    ::operator delete(ptr);
}
void operator delete(void* ptr, std::align_val_t alignment, std::nothrow_t const&) noexcept {
    ::operator delete(ptr, alignment);
}

void* operator new[](std::size_t size) {
    return ::operator new(size);
}
void* operator new[](std::size_t size, std::align_val_t alignment) {
    return ::operator new(size, alignment);
}
void* operator new[](std::size_t size, std::nothrow_t const&) noexcept {
    _TRY {
        return ::operator new(size);
    }
    _CATCHALL {
        return nullptr;
    }
}
void* operator new[](std::size_t size, std::align_val_t alignment, std::nothrow_t const&) noexcept {
    _TRY {
        return ::operator new(size, alignment);
    }
    _CATCHALL {
        return nullptr;
    }
}

void operator delete[](void* ptr) noexcept {
    ::operator delete(ptr);
}
void operator delete[](void* ptr, std::size_t) noexcept {
    ::operator delete[](ptr);
}
void operator delete[](void* ptr, std::align_val_t alignment) noexcept {
    ::operator delete(ptr, alignment);
}
void operator delete[](void* ptr, std::size_t, std::align_val_t alignment) noexcept {
    ::operator delete[](ptr, alignment);
}
void operator delete[](void* ptr, std::nothrow_t const&) noexcept {
    ::operator delete[](ptr);
}
void operator delete[](void* ptr, std::align_val_t alignment, std::nothrow_t const&) noexcept {
    ::operator delete[](ptr, alignment);
}
