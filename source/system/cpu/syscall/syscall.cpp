// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <system/sched/scheduler/scheduler.hpp>
#include <system/cpu/syscall/syscall.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/acpi/acpi.hpp>
#include <system/vfs/vfs.hpp>
#include <lib/memory.hpp>
#include <lib/string.hpp>
#include <linux/reboot.h>
#include <lib/alloc.hpp>
#include <lib/log.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::sched;
using namespace kernel::system::cpu;
using namespace kernel::system::mm;

namespace kernel::system::cpu::syscall {

bool initialised = false;
extern syscall_t syscalls[];

static void syscall_read(registers_t *regs)
{
    vfs::fd_t *fd = vfs::fd_from_fdnum(nullptr, RDI_ARG0);
    if (fd == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }
    RAX_RET = fd->handle->read(reinterpret_cast<uint8_t*>(RSI_ARG1), RDX_ARG2);
    RDX_ERRNO = 0;
    fd->unref();
}

static void syscall_write(registers_t *regs)
{
    vfs::fd_t *fd = vfs::fd_from_fdnum(nullptr, RDI_ARG0);
    if (fd == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }
    RAX_RET = fd->handle->write(reinterpret_cast<uint8_t*>(RSI_ARG1), RDX_ARG2);
    RDX_ERRNO = 0;
    fd->unref();
}

static void syscall_open(registers_t *regs)
{
    uint64_t path = RDI_ARG0;
    uint64_t flags = RSI_ARG1;
    uint64_t fd = RDX_ARG2;

    RDI_ARG0 = vfs::at_fdcwd;
    RSI_ARG1 = path;
    RDX_ARG2 = flags;
    R10_ARG3 = fd;

    syscalls[SYSCALL_OPENAT](regs);
}

static void syscall_close(registers_t *regs)
{
    if (!vfs::fdnum_close(nullptr, RDI_ARG0))
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }
    RAX_RET = 0;
    RDX_ERRNO = 0;
}

static void syscall_ioctl(registers_t *regs)
{
    vfs::fd_t *fd = vfs::fd_from_fdnum(nullptr, RDI_ARG0);
    if (fd == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }
    int ret = fd->handle->ioctl(RSI_ARG1, reinterpret_cast<void*>(RDX_ARG2));
    if (ret == -1)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }
    RAX_RET = ret;
    RDX_ERRNO = 0;
    fd->unref();
}

static void syscall_access(registers_t *regs)
{
    uint64_t path = RDI_ARG0;
    uint64_t mode = RSI_ARG1;

    RDI_ARG0 = vfs::at_fdcwd;
    RSI_ARG1 = path;
    RDX_ARG2 = mode;
    R10_ARG3 = 0;

    syscalls[SYSCALL_FACCESAT](regs);
}

static void syscall_getpid(registers_t *regs)
{
    int pid = getpid();
    if (pid == -1)
    {
        RAX_RET = -1;
        RDX_ERRNO = EINVAL;
        return;
    }
    RAX_RET = pid;
    RDX_ERRNO = 0;
}

static void syscall_fork(registers_t *regs)
{
    auto *oldproc = this_proc();
    auto *newproc = new scheduler::process_t();

    newproc->name = oldproc->name;
    newproc->pid = scheduler::alloc_pid();
    newproc->state = scheduler::INITIAL;

    newproc->pagemap = vmm::clonePagemap(oldproc->pagemap);
    newproc->current_dir = oldproc->current_dir;
    newproc->parent = oldproc;

    for (size_t i = 0; i < scheduler::max_fds; i++)
    {
        if (oldproc->fds[i] == nullptr) continue;
        vfs::fdnum_dup(oldproc, i, newproc, i, 0, true, false);
    }

    newproc->add_thread(this_thread()->fork(regs));
    newproc->table_add();

    RAX_RET = newproc->pid;
    RDX_ERRNO = 0;
}

static void syscall_exit(registers_t *regs)
{
    this_proc()->exit();
    RAX_RET = 0;
    RDX_ERRNO = 0;
}

#define SYSNAME "Kernel"
#define NODENAME ""
#define RELEASE KERNEL_VERSION
#define VERSION GIT_VERSION
#define MACHINE "x86_64"
#define DOMAINNAME ""
struct utsname
{
    char *sysname;
    char *nodename;
    char *release;
    char *version;
    char *machine;
#ifdef  _GNU_SOURCE
    char *domainname;
#endif
};
static void syscall_uname(registers_t *regs)
{
    utsname *buf = reinterpret_cast<utsname*>(RDI_ARG0);
    if (buf == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = EFAULT;
        return;
    }

    strcpy(buf->sysname, SYSNAME);
    strcpy(buf->nodename, NODENAME);
    strcpy(buf->release, RELEASE);
    strcpy(buf->version, VERSION);
    strcpy(buf->machine, MACHINE);
#ifdef  _GNU_SOURCE
    strcpy(buf->domainname, DOMAINNAME);
#endif

    RAX_RET = 0;
    RDX_ERRNO = 0;
}

static void syscall_getcwd(registers_t *regs)
{
    char *buffer = reinterpret_cast<char*>(RDI_ARG0);
    if (buffer == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = EINVAL;
        return;
    }

    string cwd(vfs::node2path(this_proc()->current_dir));
    if (cwd.length() >= RSI_ARG1)
    {
        RAX_RET = -1;
        RDX_ERRNO = ERANGE;
        return;
    }
    strcpy(buffer, cwd.c_str());
    RAX_RET = 0;
    RDX_ERRNO = 0;
}

static void syscall_chdir(registers_t *regs)
{
    string path(reinterpret_cast<char*>(RDI_ARG0));
    if (path.empty())
    {
        RAX_RET = -1;
        RDX_ERRNO = ENOENT;
        return;
    }

    vfs::fs_node_t *node = vfs::get_node(this_proc()->current_dir, path, true);
    if (node == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }
    if (!vfs::isdir(node->res->stat.mode))
    {
        RAX_RET = -1;
        RDX_ERRNO = ENOTDIR;
        return;
    }
    this_proc()->current_dir = node;
    RAX_RET = 0;
    RDX_ERRNO = 0;
}

static void syscall_mkdir(registers_t *regs)
{
    RAX_RET = 0;
    RDX_ERRNO = 0;
}

static void syscall_rmdir(registers_t *regs)
{
    RAX_RET = 0;
    RDX_ERRNO = 0;
}

static void syscall_link(registers_t *regs)
{
    uint64_t oldpath = RDI_ARG0;
    uint64_t newpath = RSI_ARG1;

    RDI_ARG0 = vfs::at_fdcwd;
    RSI_ARG1 = oldpath;
    RDX_ARG2 = vfs::at_fdcwd;
    R10_ARG3 = newpath;
    R8_ARG4 = 0;

    syscalls[SYSCALL_LINKAT](regs);
}

static void syscall_chmod(registers_t *regs)
{
    string path(reinterpret_cast<char*>(RDI_ARG0));
    if (path.empty())
    {
        RAX_RET = -1;
        RDX_ERRNO = ENOENT;
        return;
    }

    vfs::fs_node_t *node = vfs::get_node(this_proc()->current_dir, path);
    if (node == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    node->res->stat.mode = RSI_ARG1;
    RAX_RET = 0;
    RDX_ERRNO = 0;
}

static void syscall_fchmod(registers_t *regs)
{
    vfs::fd_t *fd = vfs::fd_from_fdnum(nullptr, RDI_ARG0);
    if (fd == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    fd->handle->res->stat.mode = RSI_ARG1;
    RAX_RET = 0;
    RDX_ERRNO = 0;
    fd->unref();
}

static void syscall_chown(registers_t *regs)
{
    string path(reinterpret_cast<char*>(RDI_ARG0));
    if (path.empty())
    {
        RAX_RET = -1;
        RDX_ERRNO = ENOENT;
        return;
    }

    vfs::fs_node_t *node = vfs::get_node(this_proc()->current_dir, path, true);
    if (node == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    node->res->stat.uid = RSI_ARG1;
    node->res->stat.gid = RDX_ARG2;
    RAX_RET = 0;
    RDX_ERRNO = 0;
}

static void syscall_fchown(registers_t *regs)
{
    vfs::fd_t *fd = vfs::fd_from_fdnum(nullptr, RDI_ARG0);
    if (fd == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    fd->handle->res->stat.uid = RSI_ARG1;
    fd->handle->res->stat.gid = RDX_ARG2;

    RAX_RET = 0;
    RDX_ERRNO = 0;
    fd->unref();
}

static void syscall_lchown(registers_t *regs)
{
    string path(reinterpret_cast<char*>(RDI_ARG0));
    if (path.empty())
    {
        RAX_RET = -1;
        RDX_ERRNO = ENOENT;
        return;
    }

    vfs::fs_node_t *node = vfs::get_node(this_proc()->current_dir, path);
    if (node == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    node->res->stat.uid = RSI_ARG1;
    node->res->stat.gid = RDX_ARG2;

    RAX_RET = 0;
    RDX_ERRNO = 0;
}

struct sysinfo
{
    long uptime;
    unsigned long loads[3];
    unsigned long totalram;
    unsigned long freeram;
    unsigned long sharedram;
    unsigned long bufferram;
    unsigned long totalswap;
    unsigned long freeswap;
    unsigned short procs;
    unsigned long totalhigh;
    unsigned long freehigh;
    unsigned int mem_unit;
    char _f[20 - 2 * sizeof(long) - sizeof(int)];
};
static void syscall_sysinfo(registers_t *regs)
{
    sysinfo *sf = reinterpret_cast<sysinfo*>(RDI_ARG0);\
    memset(sf, 0, sizeof(sysinfo));
    sf->mem_unit = 1;
    sf->uptime = rtc::seconds_since_boot();
    sf->totalram = getmemsize();
    sf->freeram = pmm::freemem();
    RAX_RET = 0;
}

static void syscall_getppid(registers_t *regs)
{
    int ppid = getppid();
    if (ppid == -1)
    {
        RAX_RET = -1;
        RDX_ERRNO = EINVAL;
        return;
    }
    RAX_RET = ppid;
    RDX_ERRNO = 0;
}

static void syscall_reboot(registers_t *regs)
{
    if (RDI_ARG0 != LINUX_REBOOT_MAGIC1 || (RSI_ARG1 != LINUX_REBOOT_MAGIC2 && RSI_ARG1 != LINUX_REBOOT_MAGIC2A && RSI_ARG1 != LINUX_REBOOT_MAGIC2B && RSI_ARG1 != LINUX_REBOOT_MAGIC2C))
    {
        RAX_RET = -1;
        RDX_ERRNO = EINVAL;
        return;
    }

    switch (RDX_ARG2)
    {
        case LINUX_REBOOT_CMD_CAD_OFF: break;
        case LINUX_REBOOT_CMD_CAD_ON:
            acpi::reboot();
            break;
        case LINUX_REBOOT_CMD_HALT:
            printf("\nSystem halted.\n");
            log("System halted.");
            asm volatile ("cli; hlt");
            break;
        case LINUX_REBOOT_CMD_KEXEC: break;
        case LINUX_REBOOT_CMD_POWER_OFF:
            printf("\nPower down.\n");
            log("Power down.");
            acpi::shutdown();
            break;
        case LINUX_REBOOT_CMD_RESTART:
            printf("\nRestarting system.\n");
            log("Restarting system.");
            acpi::reboot();
            break;
        case LINUX_REBOOT_CMD_RESTART2:
            printf("\nRestarting system with command '%s'.\n", reinterpret_cast<char*>(R10_ARG3));
            log("Restarting system with command '%s'.", reinterpret_cast<char*>(R10_ARG3));
            acpi::reboot();
            break;
        case LINUX_REBOOT_CMD_SW_SUSPEND: break;
        default:
            RAX_RET = -1;
            RDX_ERRNO = EINVAL;
            return;
    }
    RAX_RET = 0;
    RDX_ERRNO = 0;
}

static void syscall_time(registers_t *regs)
{
    uint64_t *tloc = reinterpret_cast<uint64_t*>(RDI_ARG0);
    if (tloc == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = EINVAL;
        return;
    }

    *tloc = rtc::epoch();
    RAX_RET = *tloc;
    RDX_ERRNO = 0;
}

static void syscall_openat(registers_t *regs)
{
    string path(reinterpret_cast<char*>(RSI_ARG1));
    if (path.empty())
    {
        RAX_RET = -1;
        RDX_ERRNO = ENOENT;
        return;
    }

    vfs::fs_node_t *parent = vfs::get_parent_dir(RDI_ARG0, path);
    if (parent == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    vfs::fs_node_t *node = vfs::get_node(parent, path, !(RDX_ARG2 & vfs::o_nofollow));
    if (node == nullptr && (RDX_ARG2 & vfs::file_creation_flags_mask) & vfs::o_creat)
    {
        node = vfs::create(parent, path, vfs::ifreg | 0644);
        if (node == nullptr)
        {
            RAX_RET = -1;
            RDX_ERRNO = errno_get();
            return;
        }
    }
    if (node == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    if (vfs::islnk(node->res->stat.mode))
    {
        RAX_RET = -1;
        RDX_ERRNO = ELOOP;
        return;
    }

    node = vfs::node2reduced(node, true);
    if (node == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    if (!vfs::isdir(node->res->stat.mode) && RDX_ARG2 & vfs::o_directory)
    {
        RAX_RET = -1;
        RDX_ERRNO = ENOTDIR;
        return;
    }

    int fdnum = vfs::fdnum_from_node(node, RDX_ARG2, 0, false);
    if (fdnum == -1)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }
    RAX_RET = fdnum;
    RDX_ERRNO = 0;
}

static void syscall_mkdirat(registers_t *regs)
{
    string path(reinterpret_cast<char*>(RSI_ARG1));
    if (path.empty())
    {
        RAX_RET = -1;
        RDX_ERRNO = ENOENT;
        return;
    }

    vfs::fs_node_t *parent = vfs::get_parent_dir(RDI_ARG0, path);
    if (parent == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    auto [tgt_parent, target, basename] = vfs::path2node(parent, path);
    if (tgt_parent == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = ENOENT;
        return;
    }

    if (target != nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = EEXIST;
        return;
    }

    if (vfs::create(tgt_parent, basename, RDX_ARG2 | vfs::ifdir))
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }
    RAX_RET = 0;
    RDX_ERRNO = 0;
}

static void syscall_unlinkat(registers_t *regs)
{
    string path(reinterpret_cast<char*>(RSI_ARG1));
    if (path.empty())
    {
        RAX_RET = -1;
        RDX_ERRNO = ENOENT;
        return;
    }

    vfs::fs_node_t *parent = vfs::get_parent_dir(RDI_ARG0, path);
    if (parent == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }
    if (!vfs::unlink(parent, path, RDX_ARG2 & vfs::at_removedir))
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }
    RAX_RET = 0;
    RDX_ERRNO = 0;
}

static void syscall_linkat(registers_t *regs)
{
    string oldpath(reinterpret_cast<char*>(RSI_ARG1));
    if (oldpath.empty())
    {
        RAX_RET = -1;
        RDX_ERRNO = ENOENT;
        return;
    }

    string newpath(reinterpret_cast<char*>(R10_ARG3));

    vfs::fs_node_t *oldparent = vfs::get_parent_dir(RDI_ARG0, oldpath);
    if (oldparent == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    vfs::fs_node_t *_newparent = vfs::get_parent_dir(RDX_ARG2, newpath);
    if (_newparent == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    oldparent = vfs::path2node(oldparent, oldpath).parent;
    auto [newparent, _, basename] = vfs::path2node(_newparent, newpath);

    if (oldparent->fs != newparent->fs)
    {
        RAX_RET = -1;
        RDX_ERRNO = EXDEV;
        return;
    }

    vfs::fs_node_t *oldnode = vfs::get_node(oldparent, oldpath, !(R8_ARG4 & vfs::at_symlink_nofollow));
    if (oldnode == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    vfs::fs_node_t *node = newparent->fs->link(newparent, newpath, oldnode);
    if (node == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    node->res->link(nullptr);
    newparent->children.push_back(node);

    RAX_RET = 0;
    RDX_ERRNO = 0;
}

static void syscall_readlinkat(registers_t *regs)
{
    string path(reinterpret_cast<char*>(RSI_ARG1));
    if (path.empty())
    {
        RAX_RET = -1;
        RDX_ERRNO = ENOENT;
        return;
    }

    vfs::fs_node_t *parent = vfs::get_parent_dir(RDI_ARG0, path);
    if (parent == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }
    vfs::fs_node_t *node = vfs::get_node(parent, path);
    if (node == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    if (!vfs::islnk(node->res->stat.mode))
    {
        RAX_RET = -1;
        RDX_ERRNO = EINVAL;
        return;
    }

    uint64_t copy = node->target.length() + 1;
    if (copy > R10_ARG3) copy = R10_ARG3;
    memcpy(reinterpret_cast<uint8_t*>(RDX_ARG2), node->target.c_str(), copy);

    RAX_RET = copy;
    RDX_ERRNO = 0;
}

static void syscall_faccessat(registers_t *regs)
{
    string path(reinterpret_cast<char*>(RSI_ARG1));
    if (path.empty())
    {
        RAX_RET = -1;
        RDX_ERRNO = ENOENT;
        return;
    }

    vfs::fs_node_t *parent = vfs::get_parent_dir(RDI_ARG0, path);
    if (parent == nullptr)
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }

    if (!vfs::get_node(parent, path, !(R10_ARG3 & vfs::at_symlink_nofollow)))
    {
        RAX_RET = -1;
        RDX_ERRNO = errno_get();
        return;
    }
    RAX_RET = 0;
    RDX_ERRNO = 0;
}

syscall_t syscalls[] = {
    [SYSCALL_READ] = syscall_read,
    [SYSCALL_WRITE] = syscall_write,
    [SYSCALL_OPEN] = syscall_open,
    [SYSCALL_CLOSE] = syscall_close,
    [SYSCALL_IOCTL] = syscall_ioctl,
    [SYSCALL_ACCESS] = syscall_access,
    [SYSCALL_GETPID] = syscall_getpid,
    [SYSCALL_FORK] = syscall_fork,
    [SYSCALL_EXIT] = syscall_exit,
    [SYSCALL_UNAME] = syscall_uname,
    [SYSCALL_GETCWD] = syscall_getcwd,
    [SYSCALL_CHDIR] = syscall_chdir,
    [SYSCALL_MKDIR] = syscall_mkdir,
    [SYSCALL_RMDIR] = syscall_rmdir,
    [SYSCALL_LINK] = syscall_link,
    [SYSCALL_CHMOD] = syscall_chmod,
    [SYSCALL_FCHMOD] = syscall_fchmod,
    [SYSCALL_CHOWN] = syscall_chown,
    [SYSCALL_FCHOWN] = syscall_fchown,
    [SYSCALL_LCHOWN] = syscall_lchown,
    [SYSCALL_SYSINFO] = syscall_sysinfo,
    [SYSCALL_GETPPID] = syscall_getppid,
    [SYSCALL_REBOOT] = syscall_reboot,
    [SYSCALL_TIME] = syscall_time,
    [SYSCALL_OPENAT] = syscall_openat,
    [SYSCALL_MKDIRAT] = syscall_mkdirat,
    [SYSCALL_UNLINKAT] = syscall_unlinkat,
    [SYSCALL_LINKAT] = syscall_linkat,
    [SYSCALL_READLINKAT] = syscall_readlinkat,
    [SYSCALL_FACCESAT] = syscall_faccessat
};

static void handler(registers_t *regs)
{
    if (RAX_RET >= 0 && syscalls[RAX_RET]) syscalls[RAX_RET](regs);
}

void reboot(string message)
{
    syscall(SYSCALL_REBOOT, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART2, message.c_str());
}

void init()
{
    log("Initialising System calls");

    if (initialised)
    {
        warn("System calls have already been initialised!\n");
        return;
    }

    idt::register_interrupt_handler(idt::SYSCALL, handler, true);

    serial::newline();
    initialised = true;
}
}