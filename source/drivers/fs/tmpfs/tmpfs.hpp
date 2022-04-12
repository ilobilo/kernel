// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <system/vfs/vfs.hpp>

using namespace kernel::system;

namespace kernel::drivers::fs::tmpfs {

struct tmpfs_fs : vfs::filesystem_t
{
    uint64_t inode_counter = 0;
    uint64_t dev_id = 0;

    void init();
    void populate(vfs::fs_node_t *node);
    vfs::fs_node_t *mount(vfs::fs_node_t *parent, vfs::fs_node_t *source, std::string dest);
    vfs::fs_node_t *symlink(vfs::fs_node_t *parent, std::string source, std::string dest);
    vfs::fs_node_t *create(vfs::fs_node_t *parent, std::string name, int mode);
    vfs::fs_node_t *link(vfs::fs_node_t *parent, std::string name, vfs::fs_node_t *old);
};

struct tmpfs_res : vfs::resource_t
{
    uint8_t *storage;
    uint64_t cap;

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
extern tmpfs_fs *tmpfs;

void init();
}