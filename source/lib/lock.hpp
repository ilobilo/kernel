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

#define DEFINE_LOCK(name) static lock_t name;