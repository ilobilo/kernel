// Copyright (C) 2021  ilobilo

#include <system/sched/scheduler/scheduler.hpp>
#include <drivers/display/serial/serial.hpp>
#include <lib/memory.hpp>

using namespace kernel::drivers::display;

namespace kernel::system::sched::scheduler {

bool initialised = false;
Vector<thread_t*> threads;
thread_t *current_thread;
uint64_t next_id = 0;
DEFINE_LOCK(thread_lock)

extern "C" void context_switch(cpu_context **old_ctx, cpu_context *new_ctx);

static thread_t *alloc()
{
    thread_t *thread = new thread_t;
    acquire_lock(thread_lock);
    thread->stack = (uintptr_t)heap::malloc(TSTACK_SIZE);
    thread->id = next_id++;
    thread->state = INITIAL;
    thread->next = NULL;
    release_lock(thread_lock);
    uint64_t sp = (uintptr_t)thread->stack + TSTACK_SIZE;
    sp -= sizeof(cpu_context);
    thread->ctx = (cpu_context*)sp;
    memset(thread->ctx, 0, sizeof(cpu_context));
    return thread;
}

thread_t *create(uint64_t addr, void *args)
{
    thread_t *thread = alloc();
    thread->ctx->rip = addr;
    thread->ctx->rdi = (uint64_t)args;
    acquire_lock(thread_lock);
    thread->state = READY;
    thread->pagemap = vmm::clonePagemap((current_thread) ? current_thread->pagemap : vmm::kernel_pagemap);
    thread->next = threads[0];
    if (initialised) threads.last()->next = thread;
    else thread->next = thread;
    release_lock(thread_lock);
    threads.push_back(thread);
    return thread;
}

static void switch_thread(thread_t *new_thrd)
{
    context_switch(&current_thread->ctx, new_thrd->ctx);
    current_thread = new_thrd;
    vmm::switchPagemap(current_thread->pagemap);
}

void schedule()
{
    if (!initialised) return;
    thread_t *toswitch = current_thread->next;
    while (toswitch->next->state != READY) toswitch = toswitch->next;
    switch_thread(toswitch->next);
}

void init(uint64_t addr, void *args)
{
    threads.init();
    current_thread = create(addr, args);
    initialised = true;
}
}