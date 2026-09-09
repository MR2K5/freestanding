## Platform hooks

- _start(), this should be marked CANTUNWIND
- __cxxabiv1::__cxa_guard_acquire() __cxa_guard_release() __cxa_guard_abort()
- _Exit()
- __libc_init_array() for statics
- atomic: __cpu_relax, sleep states for wait/notify?, atomic_flag if no other atomics exist

views:

- join_with
- lazy_split
- split
- common
- chunk
- chunk_by
- cartesian_product
- cache_latest
- as_input


