// Copyright (C) 2021-2022  ilobilo

#include <drivers/fs/tmpfs/tmpfs.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <system/vfs/vfs.hpp>
#include <kernel/kernel.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/math.hpp>
#include <lib/log.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::mm;

namespace kernel::drivers::fs::tmpfs {

bool initialised = false;
tmpfs_fs *tmpfs = new tmpfs_fs;

int64_t tmpfs_res::read(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
{
    lockit(this->lock);

    uint64_t actual_size = size;
    if (offset + size > static_cast<uint64_t>(this->stat.size))
    {
        actual_size = size - ((offset + size) - this->stat.size);
    }
    memcpy(buffer, this->storage + offset, actual_size);

    return actual_size;
}

int64_t tmpfs_res::write(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
{
    lockit(this->lock);

    if (offset + size > this->cap)
    {
        uint64_t new_cap = this->cap;
        while (offset + size > new_cap) new_cap *= 2;

        uint8_t *new_storage = realloc<uint8_t*>(this->storage, new_cap);
        if (new_storage == nullptr || new_storage == this->storage) return 0;

        this->storage = new_storage;
        this->cap = new_cap;
    }

    memcpy(this->storage + offset, buffer, size);

    if (offset + size > static_cast<uint64_t>(this->stat.size))
    {
        this->stat.size = offset + size;
        this->stat.blocks = DIV_ROUNDUP(this->stat.size, this->stat.blksize);
    }

    return size;
}

int tmpfs_res::ioctl(void *handle, uint64_t request, void *argp)
{
    return vfs::default_ioctl(handle, request, argp);
}

bool tmpfs_res::grow(void *handle, size_t new_size)
{
    lockit(this->lock);

    uint64_t new_cap = this->cap;
    while (new_size > new_cap) new_cap *= 2;

    uint8_t *new_storage = realloc<uint8_t*>(this->storage, new_cap);
    if (new_storage == nullptr || new_storage == this->storage) return false;

    this->storage = new_storage;
    this->cap = new_cap;

    this->stat.size = new_size;
    this->stat.blocks = DIV_ROUNDUP(new_size, this->stat.blksize);

    return true;
}

void tmpfs_res::unref(void *handle)
{
    lockit(this->lock);

    this->refcount--;
    if (this->refcount == 0 && vfs::isreg(this->stat.mode))
    {
        free(this->storage);
        free(this);
    }
}

void tmpfs_res::link(void *handle)
{
    lockit(this->lock);

    this->stat.nlink++;
}

void tmpfs_res::unlink(void *handle)
{
    lockit(this->lock);

    this->stat.nlink--;
}

void *tmpfs_res::mmap(uint64_t page, int flags)
{
    lockit(this->lock);

    if (flags & vmm::MapShared)
    {
        return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(&this->storage[page * vmm::page_size]) - hhdm_offset);
    }

    void *copy = pmm::alloc();
    memcpy(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(copy) + hhdm_offset), &this->storage[page * vmm::page_size], vmm::page_size);

    return copy;
}

void tmpfs_fs::init() { }
void tmpfs_fs::populate(vfs::fs_node_t *node) { }

vfs::fs_node_t *tmpfs_fs::symlink(vfs::fs_node_t *parent, string source, string dest)
{
    vfs::fs_node_t *node = vfs::create_node(this, parent, source);

    tmpfs_res *res = new tmpfs_res;
    res->refcount = 1;

    res->stat.size = dest.length();
    res->stat.blocks = 0;
    res->stat.blksize = 512;
    res->stat.dev = dev_id;
    res->stat.inode = inode_counter++;
    res->stat.mode = 0777 | vfs::iflnk;
    res->stat.nlink = 1;

    // res->stat.atime = ;
    // res->stat.mtime = ;
    // res->stat.ctime = ;

    node->res = res;
    node->target = dest;

    return node;
}

vfs::fs_node_t *tmpfs_fs::create(vfs::fs_node_t *parent, string name, int mode)
{
    vfs::fs_node_t *node = vfs::create_node(this, parent, name);

    tmpfs_res *res = new tmpfs_res;
    res->refcount = 1;

    if (vfs::isreg(mode))
    {
        res->cap = 0x1000;
        res->storage = malloc<uint8_t*>(0x1000);
        res->can_mmap = true;
    }

    res->stat.size = 0;
    res->stat.blocks = 0;
    res->stat.blksize = 512;
    res->stat.dev = dev_id;
    res->stat.inode = inode_counter++;
    res->stat.mode = mode;
    res->stat.nlink = 1;

    // res->stat.atime = ;
    // res->stat.mtime = ;
    // res->stat.ctime = ;

    node->res = res;

    return node;
}

vfs::fs_node_t *tmpfs_fs::link(vfs::fs_node_t *parent, string name, vfs::fs_node_t *old)
{
    vfs::fs_node_t *node = vfs::create_node(this, parent, name);

    old->res->refcount++;

    node->res = old->res;
    node->children.copyfrom(old->children);

    return node;
}

vfs::fs_node_t *tmpfs_fs::mount(vfs::fs_node_t *parent, vfs::fs_node_t *source, string dest)
{
    if (this->dev_id == 0) this->dev_id = vfs::dev_new_id();
    return create(parent, dest, 0644 | vfs::ifdir);
}

void init()
{
    log("Initialising TMPFS");

    if (initialised)
    {
        warn("TMPFS has already been initialised!\n");
        return;
    }

    tmpfs->name = "tmpfs";
    vfs::install_fs(tmpfs);

    vfs::mount(nullptr, "", "/", "tmpfs");

    serial::newline();
    initialised = true;
}
}