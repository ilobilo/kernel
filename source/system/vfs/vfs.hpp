// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/vector.hpp>
#include <lib/string.hpp>
#include <lib/errno.hpp>
#include <lib/cwalk.hpp>
#include <cstdint>

namespace kernel::system::sched::scheduler
{
    struct process_t;
}
using namespace kernel::system::sched;

namespace kernel::system::vfs {

static constexpr int64_t at_fdcwd = -100;
static constexpr uint64_t at_empty_path = 1;
static constexpr uint64_t at_symlink_follow = 2;
static constexpr uint64_t at_symlink_nofollow = 4;
static constexpr uint64_t at_removedir = 8;
static constexpr uint64_t at_eaccess = 512;
static constexpr uint64_t seek_cur = 1;
static constexpr uint64_t seek_end = 2;
static constexpr uint64_t seek_set = 3;

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

    int64_t read(uint8_t *buffer, uint64_t size)
    {
        lockit(this->lock);
        uint64_t ret = this->res->read(this, buffer, this->offset, size);
        this->offset += ret;
        return ret;
    }
    int64_t write(uint8_t *buffer, uint64_t size)
    {
        lockit(this->lock);
        uint64_t ret = this->res->write(this, buffer, this->offset, size);
        this->offset += ret;
        return ret;
    }
    int ioctl(uint64_t request, void *argp)
    {
        return this->res->ioctl(this, request, argp);
    }
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

struct [[gnu::aligned(16)]] filesystem_t
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
fs_node_t *node2reduced(fs_node_t *node, bool symlinks);

fs_node_t *get_parent_dir(int dirfd, string path);
fs_node_t *get_node(fs_node_t *parent, string path, bool links = false);

fs_node_t *create(fs_node_t *parent, string name, int mode);
fs_node_t *symlink(fs_node_t *parent, string path, string target);
bool unlink(fs_node_t *parent, string name, bool remdir);
bool mount(fs_node_t *parent, string source, string target, filesystem_t *filesystem);

int fdnum_from_node(fs_node_t *node, int flags, int oldfd, bool specific);

int fdnum_from_fd(scheduler::process_t *proc, fd_t *fd, int oldfd, bool specific);
int fdnum_from_res(scheduler::process_t *proc, resource_t *res, int flags, int oldfd, bool specific);

fd_t *fd_from_fdnum(scheduler::process_t *proc, int fdnum);
fd_t *fd_from_res(resource_t *res, int flags);

int fdnum_dup(scheduler::process_t *oldproc, int oldfdnum, scheduler::process_t *newproc, int newfdnum, int flags, bool specific, bool cloexec);
bool fdnum_close(scheduler::process_t *proc, int fdnum);

void dump_vfs(fs_node_t *current_node = fs_root);

void init();

static inline auto path2node(fs_node_t *parent, string path)
{
    struct ret { fs_node_t *parent; fs_node_t *node; string basename; };
    ret null = { nullptr, nullptr, "" };

    if (path.first() == '/' || parent == nullptr) parent = fs_root;
    path = path2normal(path);

    fs_node_t *curr_node = node2reduced(parent, false);
    if (path == "/") return ret { curr_node, curr_node, "/" };
    if (path.empty()) return ret { parent->parent, parent, parent->name };

    bool last = false;

    cwk_segment segment;
    cwk_path_get_first_segment(path.c_str(), &segment);

    cwk_segment lastsegment;
    cwk_path_get_last_segment(path.c_str(), &lastsegment);
    string lastseg(lastsegment.begin, lastsegment.size);

    do {
        string seg(segment.begin, segment.size);
        if (seg == lastseg) last = true;

        curr_node = node2reduced(curr_node, false);

        for (fs_node_t *child : curr_node->children)
        {
            if (child->name == seg)
            {
                fs_node_t *node = node2reduced(child, false);
                if (last == true) return ret { curr_node, node, seg };

                curr_node = node;

                if (islnk(curr_node->res->stat.mode))
                {
                    curr_node = path2node(curr_node->parent, curr_node->target).node;
                    if (curr_node == nullptr) return null;
                    continue;
                }
                if (!isdir(curr_node->res->stat.mode))
                {
                    errno_set(ENOTDIR);
                    return null;
                }
                goto next;
            }
        }

        errno_set(ENOENT);
        if (last == true) return ret { curr_node, nullptr, seg };
        return null;

        next:;
    }
    while (cwk_path_get_next_segment(&segment));

    errno_set(ENOENT);
    return null;
}
}