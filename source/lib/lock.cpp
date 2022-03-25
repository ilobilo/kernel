// Copyright (C) 2021-2022  ilobilo

#include <lib/log.hpp>
#include <lib/lock.hpp>

void lock_t::lock()
{
    while (__atomic_test_and_set(&this->locked, __ATOMIC_ACQUIRE));
}

void lock_t::unlock()
{
    __atomic_clear(&this->locked, __ATOMIC_RELEASE);
}

bool lock_t::test()
{
    return this->locked;
}