// Copyright (C) 2021  ilobilo

#include <system/sched/scheduler/scheduler.hpp>
#include <system/sched/timer/timer.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/cpu/apic/apic.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/cpu/smp/smp.hpp>
#include <kernel/kernel.hpp>
#include <lib/string.hpp>
#include <lib/log.hpp>

using namespace kernel::system::cpu;

namespace kernel::system::sched::scheduler {

bool initialised = false;
bool debug = false;
static uint64_t next_pid = 1;
static uint8_t sched_vector = 0;

static vector<process_t*> proc_table;
process_t *initproc = nullptr;

size_t proc_count = 0;
size_t thread_count = 0;

DEFINE_LOCK(thread_lock)
DEFINE_LOCK(sched_lock)
DEFINE_LOCK(proc_lock)

thread_t *thread_alloc(uint64_t addr, uint64_t args)
{
    thread_lock.lock();
    thread_t *thread = new thread_t;

    thread->state = INITIAL;
    thread->stack = static_cast<uint8_t*>(malloc(STACK_SIZE));

    thread->regs.rflags = 0x202;
    thread->regs.cs = 0x28;
    thread->regs.ss = 0x30;

    thread->regs.rip = addr;
    thread->regs.rdi = reinterpret_cast<uint64_t>(args);
    thread->regs.rsp = reinterpret_cast<uint64_t>(thread->stack) + STACK_SIZE;

    thread->parent = nullptr;
    thread_lock.unlock();

    return thread;
}

thread_t *thread_create(uint64_t addr, uint64_t args, process_t *parent)
{
    thread_t *thread = thread_alloc(addr, args);
    if (parent)
    {
        thread->tid = parent->next_tid++;
        thread->parent = parent;
        parent->threads.push_back(thread);
    }
    thread_count++;
    thread_lock.lock();
    thread->state = READY;
    thread_lock.unlock();

    return thread;
}

void idle()
{
    while (true) asm volatile ("hlt");
}

process_t *proc_alloc(const char *name, uint64_t addr, uint64_t args)
{
    process_t *proc = new process_t;
    
    proc_lock.lock();
    strncpy(proc->name, name, (strlen(name) < PROC_NAME_LENGTH) ? strlen(name) : PROC_NAME_LENGTH);
    proc->pid = next_pid++;
    proc->state = INITIAL;
    proc->pagemap = vmm::newPagemap();
    proc->current_dir = vfs::open(NULL, "/");
    proc->parent = nullptr;

    if (addr) thread_create(addr, args, proc);

    if (!initialised)
    {
        initproc = proc;
        initialised = true;
    }
    proc_lock.unlock();

    return proc;
}

process_t *proc_create(const char *name, uint64_t addr, uint64_t args)
{
    process_t *proc = proc_alloc(name, addr, args);
    proc_table.push_back(proc);
    proc_count++;
    proc_lock.lock();
    proc->state = READY;
    proc_lock.unlock();

    return proc;
}

thread_t *this_thread()
{
    asm volatile ("cli");
    thread_t *thread = this_cpu->current_thread;
    asm volatile ("sti");
    return thread;
}

process_t *this_proc()
{
    asm volatile ("cli");
    process_t *proc = this_cpu->current_proc;
    asm volatile ("sti");
    return proc;
}

void yield(uint64_t ms)
{
    if (apic::initialised) apic::lapic_oneshot(sched_vector, ms);
    else pit::setfreq(MS2PIT(ms));
}

void thread_block()
{
    asm volatile ("cli");
    if (this_thread()->state == READY || this_thread()->state == RUNNING) this_thread()->state = BLOCKED;
    asm volatile ("sti");
}

void thread_block(thread_t *thread)
{
    asm volatile ("cli");
    if (this_thread()->state == READY || this_thread()->state == RUNNING) thread->state = BLOCKED;
    asm volatile ("sti");
}

void proc_block()
{
    asm volatile ("cli");
    if (this_proc() == initproc)
    {
        error("Can not block init process!");
        asm volatile ("sti");
        return;
    }
    if (this_proc()->state == READY || this_proc()->state == RUNNING) this_proc()->state = BLOCKED;
    asm volatile ("sti");
}

void proc_block(process_t *proc)
{
    asm volatile ("cli");
    if (this_proc()->state == READY || this_proc()->state == RUNNING) proc->state = BLOCKED;
    asm volatile ("sti");
}

void thread_unblock()
{
    asm volatile ("cli");
    if (this_thread()->state == BLOCKED) this_thread()->state = READY;
    asm volatile ("sti");
}

void proc_unblock()
{
    asm volatile ("cli");
    if (this_proc()->state == BLOCKED) this_proc()->state = READY;
    asm volatile ("sti");
}

void switchTask(registers_t *regs)
{
    if (!initialised) return;

    sched_lock.lock();
    uint64_t timeslice = DEFAULT_TIMESLICE;

    if (!this_proc() || !this_thread())
    {
        for (size_t i = 0; i < proc_table.size(); i++)
        {
            process_t *proc = proc_table[i];
            if (proc->state != READY) continue;

            for (size_t t = 0; t < proc->threads.size(); t++)
            {
                thread_t *thread = proc->threads[t];
                if (thread->state != READY) continue;

                this_cpu->current_proc = proc;
                this_cpu->current_thread = thread;
                timeslice = this_thread()->timeslice;
                goto success;
            }
            goto nofree;
        }
        goto nofree;
    }
    else
    {
        this_proc()->pagemap->TOPLVL = vmm::getPagemap();
        this_thread()->regs = *regs;

        if (this_thread()->state == RUNNING) this_thread()->state = READY;

        for (size_t t = this_thread()->tid; t < this_proc()->threads.size(); t++)
        {
            thread_t *thread = this_proc()->threads[t];
            if (thread->state != READY) continue;

            this_cpu->current_proc = this_proc();
            this_cpu->current_thread = thread;
            timeslice = this_thread()->timeslice;
            goto success;
        }
        bool first = true;
        for (size_t p = this_proc()->pid; p <= proc_table.size(); p++)
        {
            if (p == proc_table.size() && first)
            {
                p = 0;
                first = false;
            }
            else break;
            process_t *proc = proc_table[p];
            if (proc->state != READY) continue;

            for (size_t t = 0; t < proc->threads.size(); t++)
            {
                thread_t *thread = proc->threads[t];
                if (thread->state != READY) continue;

                this_cpu->current_proc = proc;
                this_cpu->current_thread = thread;
                timeslice = this_thread()->timeslice;
                goto success;
            }
        }
        goto nofree;
    }
    goto nofree;

    success:;

    this_thread()->state = RUNNING;
    *regs = this_thread()->regs;
    vmm::switchPagemap(this_proc()->pagemap);

    if (debug) log("Running: process[%d]->thread[%d]: CPU core %zu", this_proc()->pid - 1, this_thread()->tid - 1, this_cpu->lapic_id);

    sched_lock.unlock();
    yield(timeslice);
    return;

    nofree:
    if (this_cpu->idle_thread == nullptr) this_cpu->idle_thread = thread_alloc(reinterpret_cast<uint64_t>(idle), 0);
    *regs = this_cpu->idle_thread->regs;
    sched_lock.unlock();
    yield();
}

void init()
{
    if (apic::initialised)
    {
        if (sched_vector == 0)
        {
            sched_vector = idt::alloc_vector();
            idt::register_interrupt_handler(sched_vector, switchTask);
        }
        apic::lapic_periodic(sched_vector);
    }
    else pit::schedule = true;
    while (true) asm volatile ("hlt");
}
}