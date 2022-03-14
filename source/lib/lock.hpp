// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <stddef.h>

class lock_t
{
    private:
    volatile bool locked = false;

    public:
    void lock();
    void unlock();
    bool test();
};

class lockit
{
    private:
    lock_t *lock;
    public:
    lockit(lock_t &lock)
    {
        this->lock = &lock;
        lock.lock();
    }
    ~lockit()
    {
        lock->unlock();
    }
};

#define new_lock(name) static lock_t name;
#define lockit(name) lockit lock(name);