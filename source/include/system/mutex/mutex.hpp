#pragma once

#include <stddef.h>

#define DEFINE_MUTEX(name) static mutex name = {0};

struct mutex
{
    volatile bool locked;
};

void mutex_lock(mutex *mutex);
void mutex_unlock(mutex *mutex);
