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

#define TSTACK_SIZE 32768

enum state_t
{
    INITIAL,
    READY,
    RUNNING,
    BLOCKED,
    KILLED
};

struct thread_t
{
    int id;
    state_t state;
    cpu_context *ctx;
    vmm::Pagemap *pagemap;
    vfs::fs_node_t *current_dir;
    uintptr_t stack;
    thread_t *next;
};

extern bool initialised;
extern Vector<thread_t*> threads;

thread_t *create(uint64_t addr, void *args);
void schedule();

void init(uint64_t addr, void *args);
}