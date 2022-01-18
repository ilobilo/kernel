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

void thread_exit()
{
    asm volatile ("cli");
    if (this_proc() == initproc && this_proc()->threads.size() == 1 && this_proc()->children.size() == 0)
    {
        error("Can not kill init process!");
    }
    this_thread()->state = KILLED;
    asm volatile ("sti");
    yield();
    while (true) asm volatile ("hlt");
}

void proc_exit()
{
    asm volatile ("cli");
    if (this_proc() == initproc)
    {
        error("Can not kill init process!");
    }
    this_proc()->state = KILLED;
    asm volatile ("sti");
    yield();
    while (true) asm volatile ("hlt");
}

void clean_proc(process_t *proc)
{
    if (proc == nullptr) return;
    if (proc->state == KILLED)
    {
        for (size_t i = 0; i < proc->children.size(); i++)
        {
            process_t *childproc = proc->children[i];
            childproc->state = KILLED;
            clean_proc(childproc);
        }
        for (size_t i = 0; i < proc->threads.size(); i++)
        {
            thread_t *thread = proc->threads[i];
            proc->threads.remove(proc->threads.find(thread));
            free(thread->stack);
            free(thread);
            thread_count--;
        }
        process_t *parentproc = proc->parent;
        if (parentproc != nullptr)
        {
            parentproc->children.remove(parentproc->children.find(proc));
            if (parentproc->children.size() == 0 && proc->threads.size() == 0 )
            {
                parentproc->state = KILLED;
                clean_proc(parentproc);
            }
        }
        free(proc->pagemap);
        free(proc);
        proc_count--;
    }
    else
    {
        for (size_t i = 0; i < proc->threads.size(); i++)
        {
            thread_t *thread = proc->threads[i];
            if (thread->state == KILLED)
            {
                proc->threads.remove(proc->threads.find(thread));
                free(thread->stack);
                free(thread);
                thread_count--;
            }
        }
        if (proc->children.size() == 0 && proc->threads.size() == 0)
        {
            proc->state = KILLED;
            clean_proc(proc);
        }
    }
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
            if (proc->state != READY)
            {
                clean_proc(proc);
                continue;
            }

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
    else
    {
        this_thread()->regs = *regs;

        if (this_thread()->state == RUNNING) this_thread()->state = READY;

        for (size_t t = this_proc()->threads.find(this_thread()) + 1; t < this_proc()->threads.size(); t++)
        {
            thread_t *thread = this_proc()->threads[t];

            if (this_proc()->state != READY) break;
            if (thread->state != READY) continue;

            this_cpu->current_proc = this_proc();
            this_cpu->current_thread = thread;
            timeslice = this_thread()->timeslice;
            goto success;
        }
        for (size_t p = proc_table.find(this_proc()) + 1; p < proc_table.size(); p++)
        {
            process_t *proc = proc_table[p];
            if (proc->state != READY) continue;

            for (size_t t = 0; t < proc->threads.size(); t++)
            {
                thread_t *thread = proc->threads[t];
                if (thread->state != READY) continue;

                clean_proc(this_proc());

                this_cpu->current_proc = proc;
                this_cpu->current_thread = thread;
                timeslice = this_thread()->timeslice;
                goto success;
            }
        }
        for (size_t p = 0; p < proc_table.find(this_proc()) + 1; p++)
        {
            process_t *proc = proc_table[p];
            if (proc->state != READY) continue;

            for (size_t t = 0; t < proc->threads.size(); t++)
            {
                thread_t *thread = proc->threads[t];
                if (thread->state != READY) continue;

                clean_proc(this_proc());

                this_cpu->current_proc = proc;
                this_cpu->current_thread = thread;
                timeslice = this_thread()->timeslice;
                goto success;
            }
        }
    }
    goto nofree;

    success:;
    this_thread()->state = RUNNING;
    *regs = this_thread()->regs;
    vmm::switchPagemap(this_proc()->pagemap);

    if (debug) log("Running process[%d]->thread[%d] on CPU core %zu", this_proc()->pid - 1, this_thread()->tid - 1, this_cpu->lapic_id);

    sched_lock.unlock();
    yield(timeslice);
    return;

    nofree:
    clean_proc(this_proc());

    if (this_cpu->idle_proc == nullptr)
    {
        this_cpu->idle_proc = proc_alloc("Idle", reinterpret_cast<uint64_t>(idle), 0);
        thread_count--;
    }

    this_cpu->current_proc = this_cpu->idle_proc;
    this_cpu->current_thread = this_cpu->idle_proc->threads[0];
    timeslice = this_thread()->timeslice;

    this_thread()->state = RUNNING;
    *regs = this_thread()->regs;
    vmm::switchPagemap(this_proc()->pagemap);

    if (debug) log("Running Idle process on CPU core %zu", this_cpu->lapic_id);

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
            idt::idt_set_descriptor(sched_vector, idt::int_table[sched_vector], 0x8E, 1);
        }
        apic::lapic_periodic(sched_vector);
    }
    else
    {
        idt::idt_set_descriptor(sched_vector, idt::int_table[idt::IRQ0], 0x8E, 1);
        pit::schedule = true;
    }
    while (true) asm volatile ("hlt");
}
}