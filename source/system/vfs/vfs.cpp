// Copyright (C) 2021-2022  ilobilo

#include <system/sched/scheduler/scheduler.hpp>
#include <system/vfs/vfs.hpp>
#include <lib/lock.hpp>
#include <lib/log.hpp>

using namespace kernel::system::sched;

namespace kernel::system::vfs {

bool initialised = false;
bool debug = false;

fs_node_t *fs_root;
vector<filesystem_t*> filesystems;

new_lock(vfs_lock);

static uint64_t dev_id = 1;
uint64_t dev_new_id()
{
    return dev_id++;
}

int default_ioctl(void *handle, uint64_t request, void *argp)
{
    errno_set(EINVAL);
    return -1;
}

void install_fs(filesystem_t *fs)
{
    filesystems.push_back(fs);
}

filesystem_t *search_fs(std::string name)
{
    for (filesystem_t *fs : filesystems)
    {
        if (fs->name == name) return fs;
    }
    return nullptr;
}

std::string path2basename(std::string path)
{
    if (path.empty()) return "";

    const char *basename;
    cwk_path_get_basename(path.c_str(), &basename, 0);
    std::string ret(basename);

    return ret;
}

std::string path2absolute(std::string parent, std::string path)
{
    if (path.empty()) return "/";
    std::string ret;

    size_t length = cwk_path_get_absolute(parent.c_str(), path.c_str(), nullptr, 0);
    ret.resize(length);
    cwk_path_get_absolute(parent.c_str(), path.c_str(), ret.data(), length + 1);

    return ret;
}

std::string path2normal(std::string path)
{
    if (path.empty()) return "/";
    std::string ret;

    size_t length = cwk_path_normalize(path.c_str(), nullptr, 0);
    ret.resize(length);
    cwk_path_normalize(path.c_str(), ret.data(), length + 1);

    return ret;
}

std::string node2path(fs_node_t *node)
{
    if (node == nullptr) return "";
    if (node == fs_root) return "/";
    std::string ret;

    while (node != nullptr && node != fs_root && node != fs_root->mountpoint)
    {
        ret.insert("/" + node->name, 0);
        node = node->parent;
    }

    return ret;
}

fs_node_t *create_node(filesystem_t *fs, fs_node_t *parent, std::string name)
{
    fs_node_t *node = new fs_node_t;
    node->name = name;
    node->parent = parent;
    node->fs = fs;
    return node;
}

fs_node_t *node2reduced(fs_node_t *node, bool symlinks)
{
    if (node->redir) return node2reduced(node->redir, symlinks);
    if (node->mountpoint) return node2reduced(node->mountpoint, symlinks);
    if (node->target.empty() == false && symlinks == true)
    {
        fs_node_t *next_node = path2node(node->parent, node->target).node;
        if (next_node == nullptr) return nullptr;

        return node2reduced(next_node, symlinks);
    }
    return node;
}

fs_node_t *get_parent_dir(int dirfd, std::string path)
{
    scheduler::process_t *proc = this_proc();
    fs_node_t *parent = nullptr;
    if (cwk_path_is_absolute(path.c_str())) parent = fs_root;
    else
    {
        if (dirfd == at_fdcwd) parent = proc->current_dir;
        else
        {
            fd_t *dir_fd = fd_from_fdnum(proc, dirfd);
            if (!isdir(dir_fd->handle->res->stat.mode))
            {
                errno_set(ENOTDIR);
                return nullptr;
            }
            parent = dir_fd->handle->node;
        }
    }
    return parent;
}

fs_node_t *get_node(fs_node_t *parent, std::string path, bool links)
{
    fs_node_t *node = path2node(parent, path).node;
    if (node == nullptr) return nullptr;
    if (links == true) return node2reduced(node, true);
    return node;
}

void fs_node_t::dotentries(fs_node_t *parent)
{
    fs_node_t *dot = create_node(this->fs, this, ".");
    dot->redir = parent;

    fs_node_t *dotdot = create_node(this->fs, this, "..");

    if (parent == fs_root) dotdot->redir = parent;
    else dotdot->redir = this->parent;

    this->children.push_back(dot);
    this->children.push_back(dotdot);
}

fs_node_t *create(fs_node_t *parent, std::string name, int mode)
{
    lockit(vfs_lock);

    auto [tgt_parent, target_node, basename] = path2node(parent, name);
    if (target_node != nullptr)
    {
        errno_set(EEXIST);
        return nullptr;
    }
    if (tgt_parent == nullptr)
    {
        errno_set(ENOENT);
        return nullptr;
    }

    target_node = tgt_parent->fs->create(tgt_parent, basename, mode);
    tgt_parent->children.push_back(target_node);
    if (isdir(target_node->res->stat.mode))
    {
        target_node->dotentries(tgt_parent);
    }

    return target_node;
}

fs_node_t *symlink(fs_node_t *parent, std::string path, std::string target)
{
    lockit(vfs_lock);

    auto [src_parent, source_node, basename] = path2node(parent, path);
    if (source_node != nullptr || src_parent == nullptr)
    {
        errno_set(EEXIST);
        return nullptr;
    }

    fs_node_t *target_node = src_parent->fs->symlink(parent, basename, target);
    src_parent->children.push_back(target_node);

    return target_node;
}

bool unlink(fs_node_t *parent, std::string name, bool remdir)
{
    auto [tgt_parent, node, basename] = path2node(parent, name);
    if (node == nullptr) return false;

    if (isdir(node->res->stat.mode) && remdir == false)
    {
        errno_set(EISDIR);
        return false;
    }

    tgt_parent->children.remove(node);

    node->res->unlink(nullptr);
    node->res->unref(nullptr);

    return true;
}

bool mount(fs_node_t *parent, std::string source, std::string target, std::string fs)
{
    lockit(vfs_lock);

    filesystem_t *filesystem = search_fs(fs);
    if (filesystem == nullptr) return false;

    fs_node_t *source_node = nullptr;
    if (!source.empty())
    {
        source_node = path2node(parent, source).node;
        if (source_node == nullptr || isdir(source_node->res->stat.mode)) return false;
    }

    auto [tgt_parent, target_node, basename] = path2node(parent, target);
    bool isroot = target_node == fs_root;

    if (target_node == nullptr || (!isroot && !isdir(target_node->res->stat.mode)))
    {
        errno_set(ENOTDIR);
        return false;
    }


    fs_node_t *mountpoint = filesystem->mount(parent, source_node, basename);
    if (mountpoint == nullptr) return false;

    target_node->mountpoint = mountpoint;
    mountpoint->dotentries(tgt_parent);

    if (!source.empty()) log("VFS: Mounted %s on %s, type: %s", source.c_str(), target.c_str(), filesystem->name.c_str());
    else log("VFS: Mounted filesystem %s on %s", filesystem->name.c_str(), target.c_str());

    return true;
}

int fdnum_from_node(fs_node_t *node, int flags, int oldfd, bool specific)
{
    fd_t *fd = fd_from_res(node->res, flags);
    if (fd == nullptr) return -1;
    fd->handle->node = node;
    return fdnum_from_fd(this_proc(), fd, oldfd, specific);
}

int fdnum_from_fd(scheduler::process_t *proc, fd_t *fd, int oldfd, bool specific)
{
    if (proc == nullptr) proc = this_proc();
    lockit(proc->fd_lock);

    if (specific)
    {
        proc->fds[oldfd] = fd;
        return oldfd;
    }
    for (size_t i = 0; i < scheduler::max_fds; i++)
    {
        if (proc->fds[i] == nullptr)
        {
            proc->fds[i] = fd;
            return i;
        }
    }
    return -1;
}

int fdnum_from_res(scheduler::process_t *proc, resource_t *res, int flags, int oldfd, bool specific)
{
    fd_t *new_fd = fd_from_res(res, flags);
    if (new_fd == nullptr) return -1;
    return fdnum_from_fd(proc, new_fd, oldfd, specific);
}

fd_t *fd_from_fdnum(scheduler::process_t *proc, int fdnum)
{
    if (proc == nullptr) proc = this_proc();
    lockit(proc->fd_lock);

    if (static_cast<uint64_t>(fdnum) >= scheduler::max_fds || fdnum < 0)
    {
        errno_set(EBADF);
        return nullptr;
    }

    fd_t *ret = static_cast<fd_t*>(proc->fds[fdnum]);
    if (ret == nullptr)
    {
        errno_set(EBADF);
        return nullptr;
    }

    ret->handle->refcount++;
    return ret;
}

fd_t *fd_from_res(resource_t *res, int flags)
{
    res->refcount++;

    handle_t *handle = new handle_t;
    handle->res = res;
    handle->refcount = 1;
    handle->flags = flags & file_status_flags_mask;

    fd_t *fd = new fd_t;
    fd->handle = handle;
    fd->flags = flags & file_descriptor_flags_mask;

    return fd;
}

int fdnum_dup(scheduler::process_t *oldproc, int oldfdnum, scheduler::process_t *newproc, int newfdnum, int flags, bool specific, bool cloexec)
{
    if (oldproc == nullptr) oldproc = this_proc();
    if (newproc == nullptr) newproc = this_proc();

    if (specific && oldfdnum == newfdnum && oldproc == newproc)
    {
        errno_set(EINVAL);
        return -1;
    }

    fd_t *oldfd = fd_from_fdnum(oldproc, oldfdnum);
    if (oldfd == nullptr) return -1;

    fd_t *newfd = new fd_t;
    memcpy(newfd, oldfd, sizeof(fd_t));

    int new_fdnum = fdnum_from_fd(newproc, newfd, newfdnum, specific);
    if (new_fdnum == -1)
    {
        oldfd->unref();
        return -1;
    }

    newfd->flags = flags & file_descriptor_flags_mask;
    if (cloexec) newfd->flags &= o_cloexec;

    oldfd->handle->refcount++;
    oldfd->handle->res->refcount++;

    return new_fdnum;
}

bool fdnum_close(scheduler::process_t *proc, int fdnum)
{
    if (proc == nullptr) proc = this_proc();

    if (static_cast<uint64_t>(fdnum) >= scheduler::max_fds)
    {
        errno_set(EBADF);
        return false;
    }

    lockit(proc->fd_lock);

    fd_t *fd = static_cast<fd_t*>(proc->fds[fdnum]);
    if (fd == nullptr)
    {
        errno_set(EBADF);
        return false;
    }

    handle_t *handle = fd->handle;
    resource_t *res = handle->res;

    res->unref(handle);
    handle->refcount--;

    if (handle->refcount == 0) delete handle;
    delete fd;

    proc->fds[fdnum] = nullptr;

    return true;
}

void dump_vfs(fs_node_t *current_node)
{
    if (current_node == nullptr) return;

    current_node = node2reduced(current_node, false);
    for (fs_node_t *node : current_node->children)
    {
        if (node->name == "." || node->name == "..") continue;
        coutl << node2path(node);
        if (node->res && isdir(node->res->stat.mode)) dump_vfs(node);
    }
}

void init()
{
    log("Initialising VFS");

    if (initialised)
    {
        warn("VFS has already been initialised!\n");
        return;
    }

    fs_root = new fs_node_t;
    fs_root->res = new resource_t;

    serial::newline();
    initialised = true;
}
}