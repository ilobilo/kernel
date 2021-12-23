// Copyright (C) 2021  ilobilo

#include <lib/cxxabi.hpp>

atexit_func __atexit_funcs[ATEXIT_MAX_FUNCS];
unsigned __atexit_func_count = 0;
void *__dso_handle = 0;

int __cxa_atexit(void (*t)(void*), void *objptr, void *dso)
{
    if (__atexit_func_count >= ATEXIT_MAX_FUNCS) return -1;
    __atexit_funcs[__atexit_func_count].destructor = t;
    __atexit_funcs[__atexit_func_count].obj_ptr = objptr;
    __atexit_funcs[__atexit_func_count].dso_handle = dso;
    __atexit_func_count++;
    return 0;
}

void __cxa_finalize(void *t)
{
    unsigned i = __atexit_func_count;
    if (!t)
    {
        while (i--)
        {
            if (__atexit_funcs[i].destructor)
            {
                (*__atexit_funcs[i].destructor)(__atexit_funcs[i].obj_ptr);
            };
        };
        return;
    };

    while (i--)
    {
        if (__atexit_funcs[i].destructor == t)
        {
            (*__atexit_funcs[i].destructor)(__atexit_funcs[i].obj_ptr);
            __atexit_funcs[i].destructor = 0;
        };
    };
}