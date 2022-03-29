// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <system/mm/vmm/vmm.hpp>
#include <system/vfs/vfs.hpp>
#include <lib/lock.hpp>
#include <lib/cpu.hpp>
#include <lib/elf.hpp>
#include <cstdint>

using namespace kernel::system::mm;

namespace kernel::system::sched::scheduler {

static constexpr uint64_t max_procs = 65536;
static constexpr uint64_t max_fds = 256;
static constexpr uint64_t MMAP_ANON_BASE = 0x80000000000;
static constexpr uint64_t THREAD_STACK_TOP = 0x70000000000;

enum state_t
{
    INITIAL,
    READY,
    RUNNING,
    BLOCKED,
    SLEEPING,
    KILLED
};

enum priority_t
{
    LOW = 3,
    MID = 5,
    HIGH = 7,
};

struct process_t;
struct thread_t
{
    uint64_t cpu;
    uint8_t *stack;
    uint8_t *kstack;

    int tid = 1;
    errno_t err;
    state_t state;
    uint8_t *stack_phys;
    uint8_t *kstack_phys;
    uint8_t *fpu_storage;
    size_t fpu_storage_size;
    uint64_t gsbase;
    uint64_t fsbase;
    registers_t regs;
    process_t *parent;
    priority_t priority;

    bool user;

    thread_t(uint64_t addr, uint64_t args, process_t *parent, priority_t priority, Auxval auxval, vector<string> argv, vector<string> envp, bool iself);
    thread_t(uint64_t addr, uint64_t args, process_t *parent, priority_t priority);
    thread_t() { };

    bool map_user();

    thread_t *fork(registers_t *regs);

    void block();
    void unblock();
    void exit(bool halt = true);
};

struct process_t
{
    string name;
    int pid = 0;
    int next_tid = 1;
    state_t state;
    vmm::Pagemap *pagemap;
    uint64_t mmap_anon_base = MMAP_ANON_BASE;
    lock_t fd_lock;
    vfs::fs_node_t *current_dir;
    void *fds[max_fds];
    vector<thread_t*> threads;
    vector<process_t*> children;
    process_t *parent;
    uint64_t thread_stack_top = THREAD_STACK_TOP;

    bool in_table = false;

    thread_t *add_user_thread(uint64_t addr, uint64_t args, priority_t priority, Auxval auxval, vector<string> argv, vector<string> envp, bool iself);
    thread_t *add_thread(uint64_t addr, uint64_t args, priority_t priority = MID);
    thread_t *add_thread(thread_t *thread);
    bool table_add();

    process_t(string name, uint64_t addr, uint64_t args, priority_t priority = MID);
    process_t(string name);
    process_t() { };

    void block();
    void unblock();
    void exit(bool halt = true);
};

extern bool debug;
extern process_t *initproc;

extern vector<process_t*> proc_table;

extern size_t proc_count;
extern size_t thread_count;

int alloc_pid();
process_t *start_program(vfs::fs_node_t *dir, string path, vector<string> argv, vector<string> envp, string stdin, string stdout, string stderr, bool execve, string procname = "");

void yield(uint64_t ms = 1);
void schedule(registers_t *regs);

void kill();
void init();
}

scheduler::thread_t *this_thread();
scheduler::process_t *this_proc();

static inline int getpid()
{
    if (this_proc() == nullptr) return -1;
    return this_proc()->pid;
}

static inline int getppid()
{
    if (this_proc() == nullptr) return -1;
    if (this_proc()->parent == nullptr) return -1;
    return this_proc()->parent->pid;
}

static inline int gettid()
{
    if (this_thread() == nullptr) return -1;
    return this_thread()->tid;
}