// Copyright (C) 2021  ilobilo

#pragma once

#include <drivers/fs/vfs/vfs.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <lib/lock.hpp>
#include <lib/cpu.hpp>
#include <stdint.h>

using namespace kernel::drivers::fs;
using namespace kernel::system::mm;

namespace kernel::system::sched::scheduler {

#define PROC_NAME_LENGTH 128
#define DEFAULT_TIMESLICE 1

enum state_t
{
    INITIAL,
    READY,
    RUNNING,
    BLOCKED,
    SLEEPING,
    KILLED
};

struct process_t;
struct thread_t
{
    int tid = 1;
    state_t state;
    uint8_t *stack;
    registers_t regs;
    process_t *parent;
    size_t timeslice = DEFAULT_TIMESLICE;
};

struct process_t
{
    char name[PROC_NAME_LENGTH];
    int pid = 0;
    int next_tid = 1;
    state_t state;
    vmm::Pagemap *pagemap;
    vfs::fs_node_t *current_dir;
    vector<thread_t*> threads;
    vector<process_t*> children;
    process_t *parent;
};

extern bool debug;
extern process_t *initproc;

extern vector<process_t*> proc_table;

extern size_t proc_count;
extern size_t thread_count;

thread_t *thread_alloc(uint64_t addr, uint64_t args);
thread_t *thread_create(uint64_t addr, uint64_t args, process_t *parent = nullptr);

process_t *proc_alloc(const char *name);
process_t *proc_create(const char *name, uint64_t addr, uint64_t args);

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

void init();
}