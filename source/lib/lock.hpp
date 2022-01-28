// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <stddef.h>

struct lock_t
{
    volatile bool locked = false;

    void lock();
    void unlock();
    bool test();
};

#define DEFINE_LOCK(name) static lock_t name;