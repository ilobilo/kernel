// Copyright (C) 2021-2022  ilobilo

#include <drivers/fs/devfs/streams/random.hpp>
#include <drivers/fs/devfs/devfs.hpp>
#include <lib/memory.hpp>
#include <lib/math.hpp>

namespace kernel::drivers::fs::streams::random {

bool initialised = false;

int64_t random_res::read(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
{
    if (size == 0) return 0;
    for (size_t i = 0; i < size; i++)
    {
        buffer[i] = rand() % 0xFF + 1;
    }
    return size;
}

int64_t random_res::write(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
{
    return size;
}

int random_res::ioctl(void *handle, uint64_t request, void *argp)
{
    return vfs::default_ioctl(handle, request, argp);
}

bool random_res::grow(void *handle, size_t new_size)
{
    return false;
}

void random_res::unref(void *handle)
{
    this->refcount--;
}

void random_res::link(void *handle)
{
    this->stat.nlink++;
}

void random_res::unlink(void *handle)
{
    this->stat.nlink--;
}

void *random_res::mmap(uint64_t page, int flags)
{
    return malloc(0x1000);
}

void init()
{
    if (initialised) return;

    random_res *res = new random_res;

    res->stat.size = 0;
    res->stat.blocks = 0;
    res->stat.blksize = 0x1000;
    res->stat.rdev = vfs::dev_new_id();
    res->stat.mode = 0666 | vfs::ifchr;
    res->can_mmap = true;

    devfs::add(res, "random");
    devfs::add(res, "urandom");

    initialised = true;
}
}