// Copyright (C) 2021-2022  ilobilo

#include <drivers/fs/devfs/streams/null.hpp>
#include <drivers/fs/devfs/devfs.hpp>

namespace kernel::drivers::fs::streams::null {

bool initialised = false;

int64_t null_res::read(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
{
    return 0;
}

int64_t null_res::write(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
{
    return size;
}

int null_res::ioctl(void *handle, uint64_t request, void *argp)
{
    return vfs::default_ioctl(handle, request, argp);
}

bool null_res::grow(void *handle, size_t new_size)
{
    return false;
}

void null_res::unref(void *handle)
{
    this->refcount--;
}

void null_res::link(void *handle)
{
    this->stat.nlink++;
}

void null_res::unlink(void *handle)
{
    this->stat.nlink--;
}

void *null_res::mmap(uint64_t page, int flags)
{
    return nullptr;
}

void init()
{
    if (initialised) return;

    null_res *res = new null_res;

    res->stat.size = 0;
    res->stat.blocks = 0;
    res->stat.blksize = 0x1000;
    res->stat.rdev = vfs::dev_new_id();
    res->stat.mode = 0666 | vfs::ifchr;
    res->can_mmap = true;

    devfs::add(res, "null");

    initialised = true;
}
}