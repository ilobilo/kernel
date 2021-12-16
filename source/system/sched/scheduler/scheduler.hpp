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

#define TSTACK_SIZE 0x8000

enum state_t
{
    READY,
    RUNNING,
    BLOCKED,
    SLEEPING,
    KILLED
};

struct thread_t
{
    int pid;
    state_t state;
    uint8_t *stack;
    registers_t regs;
    vmm::Pagemap *pagemap;
    vfs::fs_node_t *current_dir;
    volatile bool killed = false;
};

struct threadentry_t
{
    thread_t *thread;
    threadentry_t *next;
};

extern bool initialised;

thread_t *alloc(uint64_t addr, void *args);
void add(thread_t *thread);

void schedule(registers_t *regs);

void block();
void block(thread_t *thread);
void unblock(thread_t *thread);

void exit();
void exit(thread_t *thread);

int getpid();

thread_t *running_thread();

void init();
}