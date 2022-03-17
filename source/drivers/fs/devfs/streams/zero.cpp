// Copyright (C) 2021-2022  ilobilo

#include <drivers/fs/devfs/streams/zero.hpp>
#include <drivers/fs/devfs/devfs.hpp>
#include <lib/memory.hpp>

namespace kernel::drivers::fs::streams::zero {

bool initialised = false;

int64_t zero_res::read(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
{
    memset(buffer, 0, size);
    return 0;
}

int64_t zero_res::write(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
{
    return size;
}

int zero_res::ioctl(void *handle, uint64_t request, void *argp)
{
    return vfs::default_ioctl(handle, request, argp);
}

bool zero_res::grow(void *handle, size_t new_size)
{
    return false;
}

void zero_res::unref(void *handle)
{
    this->refcount--;
}

void zero_res::link(void *handle)
{
    this->stat.nlink++;
}

void zero_res::unlink(void *handle)
{
    this->stat.nlink--;
}

void *zero_res::mmap(uint64_t page, int flags)
{
    return malloc(0x1000);
}

void init()
{
    if (initialised) return;

    zero_res *res = new zero_res;

    res->stat.size = 0;
    res->stat.blocks = 0;
    res->stat.blksize = 0x1000;
    res->stat.rdev = vfs::dev_new_id();
    res->stat.mode = 0666 | vfs::ifchr;

    devfs::add(res, "zero");

    initialised = true;
}
}