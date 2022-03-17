// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <system/vfs/vfs.hpp>

using namespace kernel::system;

namespace kernel::drivers::fs::streams::zero {

struct zero_res : vfs::resource_t
{
    int64_t read(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size);
    int64_t write(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size);
    int ioctl(void *handle, uint64_t request, void *argp);
    bool grow(void *handle, size_t new_size);
    void unref(void *handle);
    void link(void *handle);
    void unlink(void *handle);
    void *mmap(uint64_t page, int flags);
};

extern bool initialised;
void init();
}