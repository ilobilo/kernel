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

DEFINE_LOCK(thread_lock)
DEFINE_LOCK(sched_lock)
DEFINE_LOCK(proc_lock)

thread_t *thread_create(uint64_t addr, uint64_t args, process_t *parent)
{
    thread_lock.lock();
    thread_t *thread = new thread_t;
    if (parent == nullptr) parent = this_proc();

    thread->tid = parent->next_tid++;;
    thread->state = READY;
    thread->stack = static_cast<uint8_t*>(malloc(STACK_SIZE));

    thread->regs.rflags = 0x202;
    thread->regs.cs = 0x28;
    thread->regs.ss = 0x30;

    thread->regs.rip = addr;
    thread->regs.rdi = reinterpret_cast<uint64_t>(args);
    thread->regs.rsp = reinterpret_cast<uint64_t>(thread->stack) + STACK_SIZE;

    thread->parent = parent;
    if (parent) parent->threads.push_back(thread);
    thread_lock.unlock();

    return thread;
}

process_t *proc_create(const char *name, uint64_t addr, uint64_t args)
{
    process_t *proc = new process_t;
    
    proc_lock.lock();
    strncpy(proc->name, name, (strlen(name) < PROC_NAME_LENGTH) ? strlen(name) : PROC_NAME_LENGTH);
    proc->pid = next_pid++;
    proc->state = READY;
    proc->pagemap = vmm::newPagemap();
    proc->current_dir = vfs::fs_root->ptr;
    proc->parent = nullptr;

    if (addr) thread_create(addr, args, proc);

    if (!initialised)
    {
        initproc = proc;
        initialised = true;
    }

    proc_table.push_back(proc);
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

uint64_t counter = 0;

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
        this_cpu->current_proc->pagemap->TOPLVL = vmm::getPagemap();
        this_cpu->current_thread->regs = *regs;

        this_cpu->current_proc->state = READY;
        this_cpu->current_thread->state = READY;

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
            process_t *proc = proc_table[p];

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

    this_cpu->current_proc->state = RUNNING;
    this_cpu->current_thread->state = RUNNING;

    *regs = this_cpu->current_thread->regs;
    vmm::switchPagemap(this_cpu->current_proc->pagemap);

    if (debug) log("Running: process[%d]->thread[%d]: CPU core %zu", this_proc()->pid - 1, this_thread()->tid - 1, this_cpu->lapic_id);

    nofree:;
    sched_lock.unlock();

    if (apic::initialised) apic::lapic_periodic(sched_vector, timeslice);
    else pit::setfreq(MS2PIT(timeslice));
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