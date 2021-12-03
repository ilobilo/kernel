// Copyright (C) 2021  ilobilo

#include <lib/lock.hpp>

void acquire_lock(lock_t &lock)
{
    while (!__sync_bool_compare_and_swap(&lock, 0, 1)) while (lock) asm volatile ("pause");
}

void release_lock(lock_t &lock)
{
    __sync_lock_release(&lock);
}