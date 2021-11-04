// Copyright (C) 2021  ilobilo

#include <system/sched/lock/lock.hpp>

void acquire_lock(lock_t *lock)
{
    while (__sync_lock_test_and_set(lock, __ATOMIC_ACQUIRE));
}

void release_lock(lock_t *lock)
{
    __sync_lock_release(lock);
}