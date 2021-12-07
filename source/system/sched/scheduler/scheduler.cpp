// Copyright (C) 2021  ilobilo

#include <system/sched/scheduler/scheduler.hpp>
#include <drivers/display/serial/serial.hpp>
#include <system/cpu/smp/smp.hpp>
#include <lib/memory.hpp>

using namespace kernel::drivers::display;

namespace kernel::system::sched::scheduler {

bool initialised = false;
Vector<thread_t*> threads;
uint64_t next_id = 0;
DEFINE_LOCK(thread_lock)

extern "C" void Switch(cpu_context **oldregs, cpu_context *regs);

thread_t *alloc(uint64_t addr, void *args)
{
    thread_t *thread = new thread_t;

    acquire_lock(thread_lock);
    thread->stack = (uintptr_t)heap::malloc(TSTACK_SIZE);

    thread->regs = (cpu_context*)(thread->stack + TSTACK_SIZE - sizeof(cpu_context));
    memset(thread->regs, 0, sizeof(cpu_context));

    thread->regs->rip = addr;
    thread->regs->rdi = (uint64_t)args;
    thread->regs->rsp = (uint64_t)thread->stack;
    thread->regs->rflags = 0x202;

    thread->id = next_id++;
    thread->next = NULL;
    thread->state = READY;
    thread->pagemap = vmm::clonePagemap(vmm::kernel_pagemap);
    thread->current_dir = vfs::fs_root->ptr;
    release_lock(thread_lock);

    return thread;
}

thread_t *init(uint64_t addr, void *args);
thread_t *add(uint64_t addr, void *args)
{
    if (!threads.on) return init(addr, args);

    thread_t *thread = alloc(addr, args);

    acquire_lock(thread_lock);
    thread->next = threads[0];
    thread->last = threads.last();
    threads.last()->next = thread;

    threads.push_back(thread);
    release_lock(thread_lock);

    return thread;
}

thread_t *init(uint64_t addr, void *args)
{
    thread_t *thread;
    if (!threads.on)
    {
        threads.init();
        thread = alloc(addr, args);
        thread->next = thread;
        thread->last = thread;
        threads.push_back(thread);
    }
    else thread = add(addr, args);
    
    this_cpu->current_thread = thread;

    initialised = true;
    return thread;
}

void schedule()
{
    if (!initialised) return;

    thread_t *toswitch = this_cpu->current_thread->next;
    while (toswitch->next->state != READY) toswitch = toswitch->next;
    toswitch->state = RUNNING;

    Switch(&this_cpu->current_thread->regs, toswitch->regs);
    vmm::switchPagemap(toswitch->pagemap);

    this_cpu->current_thread->state = READY;
    this_cpu->current_thread = toswitch;
}

void block()
{
    this_cpu->current_thread->state = BLOCKED;
}

void unblock(thread_t *thread)
{
    thread->state = READY;
}
}