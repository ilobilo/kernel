// Copyright (C) 2021  ilobilo

#pragma once

#define ATEXIT_MAX_FUNCS 128

struct atexit_func
{
    void (*destructor)(void *);
    void *obj_ptr;
    void *dso_handle;
};

extern "C" int __cxa_atexit(void (*t)(void*), void *objptr, void *dso);
extern "C" void __cxa_finalize(void *t);
