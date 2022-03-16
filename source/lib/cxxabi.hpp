#pragma once

#define ATEXIT_MAX_FUNCS 128

extern "C" {

using uarch_t = unsigned;

struct atexit_func_entry_t
{
    void (*destructor_func)(void*);
    void *obj_ptr;
    void *dso_handle;
};

}