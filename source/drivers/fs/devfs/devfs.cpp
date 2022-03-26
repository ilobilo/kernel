// Copyright (C) 2021-2022  ilobilo

#include <drivers/fs/devfs/streams/random.hpp>
#include <drivers/fs/devfs/streams/null.hpp>
#include <drivers/fs/devfs/streams/zero.hpp>
#include <drivers/fs/devfs/devfs.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <system/vfs/vfs.hpp>
#include <kernel/kernel.hpp>
#include <lib/memory.hpp>
#include <lib/math.hpp>
#include <lib/log.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::mm;

namespace kernel::drivers::fs::devfs {

bool initialised = false;
vfs::fs_node_t *devfs_root = nullptr;
devfs_fs *devfs = new devfs_fs;

uint64_t inode_counter = 0;
uint64_t dev_id = 0;

int64_t devfs_res::read(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
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

int64_t devfs_res::write(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
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

int devfs_res::ioctl(void *handle, uint64_t request, void *argp)
{
    return vfs::default_ioctl(handle, request, argp);
}

bool devfs_res::grow(void *handle, size_t new_size)
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

void devfs_res::unref(void *handle)
{
    lockit(this->lock);

    this->refcount--;
}

void devfs_res::link(void *handle)
{
    lockit(this->lock);

    this->stat.nlink++;
}

void devfs_res::unlink(void *handle)
{
    lockit(this->lock);

    this->stat.nlink--;
}

void *devfs_res::mmap(uint64_t page, int flags)
{
    lockit(this->lock);

    if (flags & vmm::MapShared)
    {
        return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(&this->storage[page * vmm::page_size]) - vmm::hhdm_offset);
    }

    void *copy = pmm::alloc();
    memcpy(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(copy) + vmm::hhdm_offset), &this->storage[page * vmm::page_size], vmm::page_size);

    return copy;
}

void devfs_fs::init() { }
void devfs_fs::populate(vfs::fs_node_t *node) { }

vfs::fs_node_t *devfs_fs::symlink(vfs::fs_node_t *parent, string source, string dest)
{
    vfs::fs_node_t *node = vfs::create_node(this, parent, source);

    devfs_res *res = new devfs_res;
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

vfs::fs_node_t *devfs_fs::create(vfs::fs_node_t *parent, string name, int mode)
{
    vfs::fs_node_t *node = vfs::create_node(this, parent, name);

    devfs_res *res = new devfs_res;
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

vfs::fs_node_t *devfs_fs::link(vfs::fs_node_t *parent, string name, vfs::fs_node_t *old)
{
    vfs::fs_node_t *node = vfs::create_node(this, parent, name);

    old->res->refcount++;

    node->res = old->res;
    node->children.copyfrom(old->children);

    return node;
}

vfs::fs_node_t *devfs_fs::mount(vfs::fs_node_t *parent, vfs::fs_node_t *source, string dest)
{
    if (dev_id == 0) dev_id = vfs::dev_new_id();
    if (devfs_root == nullptr) devfs_root = create(parent, dest, 0644 | vfs::ifdir);
    return devfs_root;
}

bool add(vfs::resource_t *res, string name)
{
    vfs::fs_node_t *node = vfs::create_node(devfs, devfs_root, name);
    if (node == nullptr) return false;

    node->res = res;
    node->res->stat.dev = dev_id;
    node->res->stat.inode = inode_counter++;
    node->res->stat.nlink = 1;

    // res->stat.atime = ;
    // res->stat.mtime = ;
    // res->stat.ctime = ;

    devfs_root->children.push_back(node);

    return true;
}

void init()
{
    log("Initialising DEVFS");

    if (initialised)
    {
        warn("DEVFS has already been initialised!\n");
        return;
    }

    devfs->name = "devfs";

    vfs::install_fs(devfs);

    vfs::create(vfs::fs_root, "/dev", 0644 | vfs::ifdir);
    vfs::mount(vfs::fs_root, "", "/dev", devfs);

    streams::random::init();
    streams::null::init();
    streams::zero::init();

    serial::newline();
    initialised = true;
}
}