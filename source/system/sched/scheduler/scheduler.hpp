// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <system/mm/vmm/vmm.hpp>
#include <system/vfs/vfs.hpp>
#include <lib/lock.hpp>
#include <lib/cpu.hpp>
#include <stdint.h>

using namespace kernel::system::mm;

namespace kernel::system::sched::scheduler {

constexpr uint64_t max_fds = 256;

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
    int tid = 1;
    state_t state;
    uint8_t *stack;
    uint8_t *stack_phys;
    uint8_t *fpu_storage;
    registers_t regs;
    process_t *parent;
    priority_t priority;
};

struct process_t
{
    string name;
    int pid = 0;
    int next_tid = 1;
    state_t state;
    vmm::Pagemap *pagemap;
    lock_t fd_lock;
    vfs::fs_node_t *current_dir;
    void *fds[max_fds];
    vector<thread_t*> threads;
    vector<process_t*> children;
    process_t *parent;
    uint64_t thread_stack_top = 0x70000000000;
};

extern bool debug;
extern process_t *initproc;

extern vector<process_t*> proc_table;

extern size_t proc_count;
extern size_t thread_count;

thread_t *thread_create(uint64_t addr, uint64_t args, process_t *parent = nullptr, priority_t priority = MID, bool user = false);
process_t *proc_create(string name, uint64_t addr, uint64_t args, priority_t priority = MID, bool user = false);

thread_t *this_thread();
process_t *this_proc();

void thread_block();
void thread_block(thread_t *thread);

void proc_block();
void proc_block(process_t *proc);

void thread_unblock(thread_t *thread);
void proc_unblock(process_t *proc);

void thread_exit();
void proc_exit();

static inline int getpid()
{
    return this_proc()->pid;
}

static inline int gettid()
{
    return this_thread()->tid;
}

void yield(uint64_t ms = 1);
void switchTask(registers_t *regs);

void kill();
void init();
}