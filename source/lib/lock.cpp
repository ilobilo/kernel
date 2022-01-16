// Copyright (C) 2021  ilobilo

#include <lib/lock.hpp>

void lock_t::lock()
{
    // while (!__sync_bool_compare_and_swap(&lock, 0, 1)) while (lock) asm volatile ("pause");
    while (__atomic_test_and_set(&this->locked, __ATOMIC_ACQUIRE));
}

void lock_t::unlock()
{
    // __sync_lock_release(&lock);
    __atomic_clear(&this->locked, __ATOMIC_RELEASE);
}

bool lock_t::test()
{
    return this->locked;
}