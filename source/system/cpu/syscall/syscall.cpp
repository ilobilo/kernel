// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <system/sched/scheduler/scheduler.hpp>
#include <system/cpu/syscall/syscall.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <drivers/fs/vfs/vfs.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/acpi/acpi.hpp>
#include <lib/memory.hpp>
#include <lib/string.hpp>
#include <linux/reboot.h>
#include <lib/buddy.hpp>
#include <lib/log.hpp>

using namespace kernel::drivers::display;
using namespace kernel::drivers::fs;
using namespace kernel::system::sched;
using namespace kernel::system::cpu;
using namespace kernel::system::mm;

namespace kernel::system::cpu::syscall {

bool initialised = false;

static void syscall_read(registers_t *regs)
{
    char *str = (char*)"Read currently not working!\n";
    RAX = reinterpret_cast<uint64_t>(str);
}

static void syscall_write(registers_t *regs)
{
    switch (RDI_ARG0)
    {
        case 0:
        {
            printf("%.*s", static_cast<int>(RDX_ARG2), reinterpret_cast<char*>(RSI_ARG1));
            RAX = RDX_ARG2 * sizeof(char);
            break;
        }
        case 1:
            break;
        case 2:
        {
            error("%.*s", static_cast<int>(RDX_ARG2), reinterpret_cast<char*>(RSI_ARG1));
            RAX = RDX_ARG2 * sizeof(char);
            break;
        }
        default:
            break;
    }
}

static void syscall_getpid(registers_t *regs)
{
    RAX = scheduler::getpid();
}

static void syscall_exit(registers_t *regs)
{
    scheduler::exit();
    RAX = 0;
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
    strcpy(buf->sysname, SYSNAME);
    strcpy(buf->nodename, NODENAME);
    strcpy(buf->release, RELEASE);
    strcpy(buf->version, VERSION);
    strcpy(buf->machine, MACHINE);
#ifdef  _GNU_SOURCE
    strcpy(buf->domainname, DOMAINNAME);
#endif
    RAX = 0;
}

static void syscall_chdir(registers_t *regs)
{
    vfs::fs_node_t *node = vfs::open(NULL, reinterpret_cast<const char*>(RDI_ARG0));
    if (node == nullptr || (node->flags & 0x07) != vfs::FS_DIRECTORY)
    {
        RAX = -1;
        return;
    }
    scheduler::running_thread()->current_dir = node;
    RAX = 0;
}

static void syscall_rename(registers_t *regs)
{
    vfs::fs_node_t *node = vfs::open(NULL, reinterpret_cast<const char*>(RDI_ARG0));
    if (node == nullptr)
    {
        RAX = -1;
        return;
    }
    size_t count = 0;
    char **name = strsplit_count(reinterpret_cast<const char*>(RSI_ARG1), "/", count);
    strcpy(node->name, name[count]);
    RAX = 0;
}

static void syscall_mkdir(registers_t *regs)
{
    vfs::fs_node_t *node = vfs::open_r(NULL, reinterpret_cast<const char*>(RDI_ARG0));
    node->mode = RSI_ARG1;
    node->flags = vfs::FS_DIRECTORY;
    RAX = 0;
}

static void syscall_rmdir(registers_t *regs)
{
    vfs::fs_node_t *node = vfs::open(NULL, reinterpret_cast<const char*>(RDI_ARG0));
    if (node == nullptr)
    {
        RAX = -1;
        return;
    }
    vfs::remove_child(node->parent, node->name);
    RAX = 0;
}

static void syscall_chmod(registers_t *regs)
{
    vfs::fs_node_t *node = vfs::open(NULL, reinterpret_cast<const char*>(RDI_ARG0));
    if (node == nullptr)
    {
        RAX = -1;
        return;
    }
    node->mode = RSI_ARG1;
    RAX = 0;
}

static void syscall_chown(registers_t *regs)
{
    vfs::fs_node_t *node = vfs::open(NULL, reinterpret_cast<const char*>(RDI_ARG0));
    if (node == nullptr)
    {
        RAX = -1;
        return;
    }
    node->uid = RSI_ARG1;
    node->gid = RDX_ARG2;
    RAX = 0;
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
    char _f[20-2*sizeof(long)-sizeof(int)];
};
static void syscall_sysinfo(registers_t *regs)
{
    sysinfo *sf = reinterpret_cast<sysinfo*>(RDI_ARG0);\
    memset(sf, 0, sizeof(sysinfo));
    sf->mem_unit = 1;
    sf->uptime = rtc::seconds_since_boot();
    sf->totalram = getmemsize();
    sf->freeram = pmm::freemem();
    RAX = 0;
}

static void syscall_reboot(registers_t *regs)
{
    if (RDI_ARG0 != LINUX_REBOOT_MAGIC1 || (RSI_ARG1 != LINUX_REBOOT_MAGIC2 && RSI_ARG1 != LINUX_REBOOT_MAGIC2A && RSI_ARG1 != LINUX_REBOOT_MAGIC2B && RSI_ARG1 != LINUX_REBOOT_MAGIC2C)) RAX = -1;
    else
    {
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
        }
        RAX = 0;
    }
}

static void syscall_time(registers_t *regs)
{
    long *tloc = reinterpret_cast<long*>(RDI_ARG0);
    *tloc = rtc::epoch();
    RAX = *tloc;
}

syscall_t syscalls[] = {
    [SYSCALL_READ] = syscall_read,
    [SYSCALL_WRITE] = syscall_write,
    [SYSCALL_GETPID] = syscall_getpid,
    [SYSCALL_EXIT] = syscall_exit,
    [SYSCALL_UNAME] = syscall_uname,
    [SYSCALL_CHDIR] = syscall_chdir,
    [SYSCALL_RENAME] = syscall_rename,
    [SYSCALL_MKDIR] = syscall_mkdir,
    [SYSCALL_RMDIR] = syscall_rmdir,
    [SYSCALL_CHMOD] = syscall_chmod,
    [SYSCALL_CHOWN] = syscall_chown,
    [SYSCALL_SYSINFO] = syscall_sysinfo,
    [SYSCALL_REBOOT] = syscall_reboot,
    [SYSCALL_TIME] = syscall_time
};

static void handler(registers_t *regs)
{
    if (RAX >= 0 && syscalls[RAX]) syscalls[RAX](regs);
}

const char *read(const char *string, int length)
{
    uint64_t ret;
    SYSCALL3(SYSCALL_READ, 1, string, length);
    return reinterpret_cast<const char*>(ret);
}
const char *write(const char *string, int length)
{
    uint64_t ret;
    SYSCALL3(SYSCALL_WRITE, 0, string, length);
    return reinterpret_cast<const char*>(ret);
}
const char *err(const char *string, int length)
{
    uint64_t ret;
    SYSCALL3(SYSCALL_WRITE, 2, string, length);
    return reinterpret_cast<const char*>(ret);
}

void init()
{
    log("Initialising System calls");

    if (initialised)
    {
        warn("System calls have already been initialised!\n");
        return;
    }

    idt::register_interrupt_handler(SYSCALL, handler);

    serial::newline();
    initialised = true;
}
}