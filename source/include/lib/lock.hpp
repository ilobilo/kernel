// Copyright (C) 2021  ilobilo

#pragma once

#include <stddef.h>

using lock_t = volatile bool;
#define DEFINE_LOCK(name) static lock_t name;

void acquire_lock(lock_t &lock);
void release_lock(lock_t &lock);