// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/vector.hpp>
#include <lib/string.hpp>
#include <lib/errno.hpp>
#include <stdint.h>

namespace kernel::system::vfs {

enum stats
{
    ifmt = 0xF000,
    ifblk = 0x6000,
    ifchr = 0x2000,
    ififo = 0x1000,
    ifreg = 0x8000,
    ifdir = 0x4000,
    iflnk = 0xA000,
    ifsock = 0xC000,
    ifpipe = 0x3000
};

enum dtypes
{
    dt_unknown = 0,
    dt_fifo = 1,
    dt_chr = 2,
    dt_dir = 4,
    dt_blk = 6,
    dt_reg = 8,
    dt_lnk = 10,
    dt_sock = 12,
    dt_wht = 14
};

enum oflags
{
    o_accmode = 0x0007,
	o_exec = 1,
	o_rdonly = 2,
	o_rdwr = 3,
	o_search = 4,
	o_wronly = 5,
	o_append = 0x0008,
	o_creat = 0x0010,
	o_directory = 0x0020,
	o_excl = 0x0040,
	o_noctty = 0x0080,
	o_nofollow = 0x0100,
	o_trunc = 0x0200,
	o_nonblock = 0x0400,
	o_dsync = 0x0800,
	o_rsync = 0x1000,
	o_sync = 0x2000,
	o_cloexec = 0x4000,

    file_creation_flags_mask = o_creat | o_directory | o_excl | o_noctty | o_nofollow | o_trunc,
	file_descriptor_flags_mask = o_cloexec,
	file_status_flags_mask = ~(file_creation_flags_mask | file_descriptor_flags_mask)
};

enum fcntls
{
	f_dupfd = 1,
	f_dupfd_cloexec = 2,
	f_getfd = 3,
	f_setfd = 4,
	f_getfl = 5,
	f_setfl = 6,
	f_getlk = 7,
	f_setlk = 8,
	f_setlkw = 9,
	f_getown = 10,
	f_setown = 11,
	fd_cloexec = 1
};

struct timespec_t
{
    int64_t tv_sec;
    int64_t tv_nsec;
};

struct stat_t
{
    uint64_t dev;
    uint64_t inode;
    int mode;
    int nlink;
    int uid;
    int gid;
    int rdev;
    int size;
    timespec_t atime;
    timespec_t mtime;
    timespec_t ctime;
    uint64_t blksize;
    uint64_t blocks;
};

static inline bool isblk(int mode)
{
    return (mode & stats::ifmt) == stats::ifblk;
}
static inline bool ischr(int mode)
{
    return (mode & stats::ifmt) == stats::ifchr;
}
static inline bool isifo(int mode)
{
    return (mode & stats::ifmt) == stats::ififo;
}
static inline bool isreg(int mode)
{
    return (mode & stats::ifmt) == stats::ifreg;
}
static inline bool isdir(int mode)
{
    return (mode & stats::ifmt) == stats::ifdir;
}
static inline bool islnk(int mode)
{
    return (mode & stats::ifmt) == stats::iflnk;
}
static inline bool issock(int mode)
{
    return (mode & stats::ifmt) == stats::ifsock;
}

struct dirent_t
{
    uint64_t inode;
    uint64_t offset;
    uint16_t reclen;
    uint8_t type;
    char name[1024];
};

struct resource_t;
int default_ioctl(void *handle, uint64_t request, void *argp);

struct resource_t
{
    stat_t stat;
    int refcount;
    lock_t lock;
    bool can_mmap;

    virtual int64_t read(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
    {
        errno_set(EINVAL);
        return -1;
    }
    virtual int64_t write(void *handle, uint8_t *buffer, uint64_t offset, uint64_t size)
    {
        errno_set(EINVAL);
        return -1;
    }
    virtual int ioctl(void *handle, uint64_t request, void *argp)
    {
        return default_ioctl(this, request, argp);
    }
    virtual bool grow(void *handle, size_t new_size)
    {
        errno_set(EINVAL);
        return false;
    }
    virtual void unref(void *handle)
    {
        errno_set(EINVAL);
        return;
    }
    virtual void link(void *handle)
    {
        errno_set(EINVAL);
        return;
    }
    virtual void unlink(void *handle)
    {
        errno_set(EINVAL);
        return;
    }
    virtual void *mmap(uint64_t page, int flags)
    {
        errno_set(EINVAL);
        return nullptr;
    }
};

struct fs_node_t;
struct handle_t
{
    lock_t lock;
    resource_t *res;
    fs_node_t *node;
    int refcount;
    int64_t offset;
    int flags;
    bool dirlist_valid;
    vector<dirent_t*> dirlist;
};

struct fd_t
{
    handle_t *handle;
    int flags;

    void unref()
    {
        this->handle->refcount--;
    }
};

struct filesystem_t
{
    string name;

    virtual void init()
    {
        errno_set(EINVAL);
        return;
    }
    virtual void populate(fs_node_t *node)
    {
        errno_set(EINVAL);
        return;
    }
    virtual fs_node_t *mount(fs_node_t *parent, fs_node_t *source, string dest)
    {
        errno_set(EINVAL);
        return nullptr;
    }
    virtual fs_node_t *symlink(fs_node_t *parent, string source, string dest)
    {
        errno_set(EINVAL);
        return nullptr;
    }
    virtual fs_node_t *create(fs_node_t *parent, string name, int mode)
    {
        errno_set(EINVAL);
        return nullptr;
    }
    virtual fs_node_t *link(fs_node_t *parent, string name, fs_node_t *old)
    {
        errno_set(EINVAL);
        return nullptr;
    }
};

struct fs_node_t
{
    string name;
    string target;
    resource_t *res;
    filesystem_t *fs;
    fs_node_t *mountpoint;
    fs_node_t *parent;
    vector<fs_node_t*> children;
    fs_node_t *redir;

    void dotentries(fs_node_t *parent);
};

extern bool initialised;

extern fs_node_t *fs_root;
extern vector<filesystem_t*> filesystems;

uint64_t dev_new_id();

void install_fs(filesystem_t *fs);
filesystem_t *search_fs(string name);

string path2basename(string path);
string path2absolute(string parent, string path);
string path2normal(string path);
string node2path(fs_node_t *node);

fs_node_t *create_node(filesystem_t *fs, fs_node_t *parent, string name);
auto path2node(fs_node_t *parent, string path);
fs_node_t *node2reduced(fs_node_t *node, bool symlinks);

fs_node_t *get_parent_dir(int dirfd, string path);
fs_node_t *get_node(fs_node_t *parent, string path, bool links);

fs_node_t *create(fs_node_t *parent, string name, int mode);
fs_node_t *symlink(fs_node_t *parent, string path, string target);
bool unlink(fs_node_t *parent, string name, bool remdir);
bool mount(fs_node_t *parent, string source, string target, filesystem_t *filesystem);

struct process_t;
int fdnum_from_fd(process_t *proc, fd_t *fd, int oldfd, bool specific);
int fdnum_from_res(process_t *proc, resource_t *res, int flags, int oldfd, bool specific);

fd_t *fd_from_fdnum(process_t *proc, int fdnum);
fd_t *fd_from_res(resource_t *res, int flags);

int fdnum_dup(process_t *oldproc, int oldfdnum, process_t *newproc, int newfdnum, int flags, bool specific, bool cloexec);
bool fdnum_close(process_t *proc, int fdnum);

void init();
}