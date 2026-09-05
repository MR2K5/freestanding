#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>

#include <__algorithm/sort.hpp>
#include <iterator>
#include <numeric>
#include <type_traits>

using std::byte;

extern "C" {

// 1. Core Exit Functions
void abort() noexcept {
    _Exit(1);
}

using handler = void (*)();
static constinit handler quick_handlers[16]{};
static constinit unsigned char qh_count = 0;

// 3. Register Handler
int at_quick_exit(void (*func)()) noexcept {
    if (qh_count < 16) {
        quick_handlers[qh_count] = func;
        qh_count++;
        return 0;  // Success
    }
    return 1;
}

// 4. Run Handlers and Exit
void quick_exit(int status) noexcept {
    (void)status;

    // LIFO order
    while (qh_count > 0) {
        qh_count--;
        if (quick_handlers[qh_count]) { quick_handlers[qh_count](); }
    }

    _Exit(status);
}

int atexit(void (*func)()) noexcept {
    // TODO is this cast ok?
    __cxxabiv1::__cxa_atexit(reinterpret_cast<void (*)(void*)>(func), nullptr, nullptr);
    return 1;
}

void exit(int status) noexcept {
    __cxxabiv1::__cxa_finalize(nullptr);
    _Exit(status);
}

size_t memalignment(void const* p) noexcept {
    // FIXME Assumes a common platform. check for ARV
    return std::saturating_cast<size_t>(
        reinterpret_cast<uintptr_t>(p) & -reinterpret_cast<uintptr_t>(p)
    );
}

int abs(int i) noexcept {
    return std::abs(i);
}
long labs(long i) noexcept {
    return std::labs(i);
}
long long llabs(long long i) noexcept {
    return std::llabs(i);
}

div_t div(int numer, int denom) noexcept {
    return std::div(numer, denom);
}
ldiv_t ldiv(long int numer, long int denom) noexcept {
    return std::ldiv(numer, denom);
}
lldiv_t lldiv(long long int numer, long long int denom) noexcept {
    return std::lldiv(numer, denom);
}

extern "C++" struct [[clang::internal_linkage]] memory_block_iter {
    byte* ptr_;
    ptrdiff_t stride_;

    memory_block_iter& operator++() {
        ptr_ += stride_;
        return *this;
    }
    memory_block_iter& operator--() {
        ptr_ -= stride_;
        return *this;
    }
    memory_block_iter operator++(int) {
        auto tmp = *this;
        ++*this;
        return tmp;
    }
    memory_block_iter operator--(int) {
        auto tmp = *this;
        --*this;
        return tmp;
    }
    memory_block_iter& operator+=(ptrdiff_t n) {
        ptr_ += stride_ * n;
        return *this;
    }
    memory_block_iter& operator-=(ptrdiff_t n) {
        ptr_ -= stride_ * n;
        return *this;
    }

    friend memory_block_iter operator+(memory_block_iter i, ptrdiff_t n) { return i += n; }
    [[maybe_unused]] friend memory_block_iter operator+(ptrdiff_t n, memory_block_iter i) {
        return i += n;
    }
    friend memory_block_iter operator-(memory_block_iter i, ptrdiff_t n) { return i -= n; }
    friend ptrdiff_t operator-(memory_block_iter a, memory_block_iter b) {
        assert(a.stride_ == b.stride_);
        return (a.ptr_ - b.ptr_) / a.stride_;
    }

    static constexpr size_t max_storage_size = 256;
    struct save {
        alignas(std::max_align_t) byte store[max_storage_size];
        void const* ptr() const { return store; }
    };

    struct proxy {
        byte* ptr_;
        ptrdiff_t n;

        void* ptr() const { return ptr_; }

        operator save() const {
            save r;
            std::memcpy(r.store, ptr_, n);
            return r;
        }
        proxy const& operator=(save s) const {
            std::memcpy(ptr_, s.store, n);
            return *this;
        }
        proxy& operator=(save s) {
            std::memcpy(ptr_, s.store, n);
            return *this;
        }
    };

    proxy operator*() const { return {ptr_, stride_}; }
    auto operator->() const {
        struct arrow {
            proxy p;
            proxy const* operator->() const { return &p; }
        };
        return arrow{.p = {**this}};
    }
    proxy operator[](ptrdiff_t n) const { return *(*this + n); }

    using value_type       = save;
    using reference        = proxy;
    using difference_type  = ptrdiff_t;
    using iterator_concept = std::random_access_iterator_tag;

    friend void iter_swap(memory_block_iter a, memory_block_iter b) {
        auto storage = alloca(a.stride_);
        std::memcpy(storage, a.ptr_, a.stride_);
        std::memcpy(a.ptr_, b.ptr_, a.stride_);
        std::memcpy(b.ptr_, storage, a.stride_);
    }
    friend save iter_move(memory_block_iter x) { return save(*x); }

    friend bool operator==(memory_block_iter a, memory_block_iter b) { return a.ptr_ == b.ptr_; }
    friend auto operator<=>(memory_block_iter a, memory_block_iter b) { return a.ptr_ <=> b.ptr_; }
};

void qsort(void* ptr, size_t count, size_t size, int (*comp)(void const*, void const*)) {
    assert(size <= memory_block_iter::max_storage_size);

    auto beg = memory_block_iter{(byte*)ptr, ptrdiff_t(size)};
    auto end = memory_block_iter{((byte*)ptr) + count * size, ptrdiff_t(size)};

    std::ranges::sort(beg, end, [comp](auto x, auto y) { return comp(x.ptr(), y.ptr()) < 0; });
}
void* bsearch(
    void const* key, void const* ptr, size_t count, size_t size,
    int (*comp)(void const*, void const*)
) {
    auto first =
        memory_block_iter{static_cast<std::byte*>(const_cast<void*>(ptr)), ptrdiff_t(size)};
    auto last = first + static_cast<std::ptrdiff_t>(count);

    auto it = std::ranges::partition_point(first, last, [comp, key](auto elem) {
        return comp(elem.ptr(), key) < 0;
    });
    if (it != last && comp(key, it->ptr()) == 0) return it->ptr();
    return nullptr;
}

}  // extern "C"
