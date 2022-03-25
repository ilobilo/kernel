// Copyright (C) 2021-2022  ilobilo

#include <system/sched/scheduler/scheduler.hpp>
#include <system/sched/timer/timer.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/cpu/apic/apic.hpp>
#include <system/cpu/idt/idt.hpp>
#include <system/cpu/smp/smp.hpp>
#include <kernel/kernel.hpp>
#include <lib/string.hpp>
#include <lib/bitmap.hpp>
#include <lib/log.hpp>

using namespace kernel::system::cpu;

namespace kernel::system::sched::scheduler {

bool debug = false;
static bool die = false;

Bitmap pids;
static uint8_t sched_vector = 0;

vector<process_t*> proc_table;
process_t *initproc = nullptr;

size_t proc_count = 0;
size_t thread_count = 0;

new_lock(thread_lock);
new_lock(sched_lock);
new_lock(proc_lock);

int alloc_pid()
{
    if (pids.buffer == nullptr) pids.buffer = new uint8_t[(max_procs - 1) / 8 + 1];
    for (size_t i = 1; i < max_procs; i++)
    {
        if (pids[i] == false)
        {
            pids.Set(i, true);
            return i;
        }
    }
    return -1;
}

void yield(uint64_t ms)
{
    if (apic::initialised) apic::lapic_oneshot(sched_vector, ms);
    else pit::setfreq(MS2PIT(ms));
}

void idle()
{
    while (true) asm volatile ("hlt");
}

void func_wrapper(uint64_t addr, uint64_t args)
{
    reinterpret_cast<void (*)(uint64_t)>(addr)(args);
    this_thread()->exit();
}

thread_t::thread_t(uint64_t addr, uint64_t args, process_t *parent, priority_t priority, bool user)
{
    lockit(thread_lock);

    this->state = INITIAL;
    this->stack_phys = malloc<uint8_t*>(STACK_SIZE);
    this->stack = this->stack_phys + hhdm_tag->addr;

    this->fpu_storage = malloc<uint8_t*>(this_cpu->fpu_storage_size) + hhdm_tag->addr;
    this->fpu_storage_size = this_cpu->fpu_storage_size;

    this->regs.rflags = 0x202;
    this->regs.cs = (user ? (gdt::GDT_USER_CODE_64 | 0x03) : gdt::GDT_CODE_64);
    this->regs.ss = (user ? (gdt::GDT_USER_DATA_64 | 0x03) : gdt::GDT_DATA_64);

    this->regs.rip = reinterpret_cast<uint64_t>(func_wrapper);
    this->regs.rdi = reinterpret_cast<uint64_t>(addr);
    this->regs.rsi = reinterpret_cast<uint64_t>(args);
    this->regs.rsp = reinterpret_cast<uint64_t>(this->stack) + STACK_SIZE;

    this->priority = priority;
    this->parent = parent;

    this->user = user;
}

bool thread_t::map_user()
{
    if (this->parent == nullptr) return false;

    // TODO: Fix this if broken
    this->parent->thread_stack_top -= STACK_SIZE;
    uint64_t stack_bottom_vma = this->parent->thread_stack_top;
    this->parent->thread_stack_top -= 0x1000;

    this->parent->pagemap->mapMemRange(stack_bottom_vma, reinterpret_cast<uint64_t>(this->stack_phys), STACK_SIZE, vmm::Present | vmm::ReadWrite | vmm::UserSuper);
    this->stack = reinterpret_cast<uint8_t*>(stack_bottom_vma);

    this->regs.rsp = reinterpret_cast<uint64_t>(this->stack) + STACK_SIZE;

    return true;
}

thread_t *thread_t::fork(registers_t *regs)
{
    lockit(thread_lock);

    thread_t *newthread = new thread_t;

    newthread->state = INITIAL;
    newthread->stack_phys = malloc<uint8_t*>(STACK_SIZE);
    newthread->stack = newthread->stack_phys + hhdm_tag->addr;

    newthread->fpu_storage = malloc<uint8_t*>(this_cpu->fpu_storage_size) + hhdm_tag->addr;
    newthread->fpu_storage_size = this->fpu_storage_size;
    memcpy(newthread->fpu_storage, this->fpu_storage, this->fpu_storage_size);

    newthread->regs = *regs;
    newthread->regs.rax = 0;
    newthread->regs.rdx = 0;

    newthread->regs.rflags = 0x202;
    newthread->regs.cs = (this->user ? (gdt::GDT_USER_CODE_64 | 0x03) : gdt::GDT_CODE_64);
    newthread->regs.ss = (this->user ? (gdt::GDT_USER_DATA_64 | 0x03) : gdt::GDT_DATA_64);\

    uint64_t offset = reinterpret_cast<uint64_t>(newthread->stack) - reinterpret_cast<uint64_t>(this->stack);
    newthread->regs.rsp += offset;
    newthread->regs.rbp += offset;

    memcpy(newthread->stack, this->stack, STACK_SIZE);

    newthread->priority = this->priority;
    newthread->parent = this->parent;

    newthread->user = this->user;

    return newthread;
}

thread_t *process_t::add_thread(uint64_t addr, uint64_t args, priority_t priority, bool user)
{
    lockit(proc_lock);

    thread_t *thread = new thread_t(addr, args, this, priority, user);
    if (user == true) thread->map_user();

    thread->tid = this->next_tid++;
    thread_count++;

    this->threads.push_back(thread);
    thread->state = READY;

    return thread;
}

thread_t *process_t::add_thread(thread_t *thread)
{
    lockit(proc_lock);

    if (thread->parent != this) thread->parent = this;
    if (thread->user == true) thread->map_user();

    thread->tid = this->next_tid++;
    thread_count++;

    this->threads.push_back(thread);
    thread->state = READY;

    return thread;
}

bool process_t::table_add()
{
    if (this->in_table) return false;
    lockit(proc_lock);

    if (initproc == nullptr) initproc = this;
    proc_table.push_back(this);
    proc_count++;

    this->state = READY;
    this->in_table = true;

    return true;
}

process_t::process_t(string name, uint64_t addr, uint64_t args, priority_t priority, bool user)
{
    lockit(proc_lock);

    this->name = name;
    this->pid = alloc_pid();
    this->state = INITIAL;

    this->pagemap = vmm::newPagemap();
    this->current_dir = vfs::fs_root;
    this->parent = nullptr;

    proc_lock.unlock();
    this->add_thread(addr, args, priority, user);
}

process_t::process_t(string name)
{
    lockit(proc_lock);

    this->name = name;
    this->pid = alloc_pid();
    this->state = INITIAL;

    this->pagemap = vmm::newPagemap();
    this->current_dir = vfs::fs_root;
    this->parent = nullptr;
}

void process_t::block()
{
    if (this->state != READY && this->state != RUNNING) return;
    if (this == initproc)
    {
        error("Can not block init process!");
        return;
    }

    asm volatile ("cli");

    this->state = BLOCKED;
    if (debug) log("Blocking process with PID: %d", this->pid);

    asm volatile ("sti");
    yield();
    if (this == this_proc())
    {
        yield();
        asm volatile ("hlt");
    }
}

void process_t::unblock()
{
    if (this->state != BLOCKED) return;

    asm volatile ("cli");

    this->state = READY;
    if (debug) log("Unblocking process with PID: %d", this->pid);

    asm volatile ("sti");
}

void process_t::exit()
{
    if (this == initproc)
    {
        error("Can not kill init process!");
        return;
    }

    asm volatile ("cli");

    this->state = KILLED;
    if (debug) log("Exiting process with PID: %d", this->pid);

    asm volatile ("sti");
    yield();
    asm volatile ("hlt");
}

void thread_t::block()
{
    if (this->state != READY && this->state != RUNNING) return;
    asm volatile ("cli");

    this->state = BLOCKED;
    if (debug) log("Blocking thread with TID: %d and PID: %d", this->tid, this->parent->pid);

    asm volatile ("sti");
    if (this == this_thread())
    {
        yield();
        asm volatile ("hlt");
    }
}

void thread_t::unblock()
{
    if (this->state != BLOCKED)
    asm volatile ("cli");

    this->state = READY;
    if (debug) log("Unblocking thread with TID: %d and PID: %d", this->tid, this->parent->pid);

    asm volatile ("sti");
}

void thread_t::exit()
{
    if (this->parent == initproc && this->parent->threads.size() == 1 && this->parent->children.size() == 0)
    {
        error("Can not kill init thread!");
        return;
    }

    asm volatile ("cli");

    this->state = KILLED;
    if (debug) log("Exiting thread with TID: %d and PID: %d", this->tid, this->parent->pid);

    asm volatile ("sti");
    yield();
    while (true) asm volatile ("hlt");
}

void clean_proc(process_t *proc)
{
    if (proc == nullptr || proc == this_cpu->idle_proc) return;
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
            free(thread->fpu_storage - hhdm_tag->addr);
            // TODO: Fix this
            // free(thread->stack_phys); // Trple fault
            free(thread);
            thread_count--;
        }
        for (size_t i = 0; i < max_fds; i++)
        {
            if (proc->fds[i] == nullptr) continue;
            vfs::fdnum_close(proc, i);
        }

        process_t *parentproc = proc->parent;
        if (parentproc != nullptr)
        {
            parentproc->children.remove(parentproc->children.find(proc));
            if (parentproc->children.size() == 0 && proc->threads.size() == 0)
            {
                parentproc->state = KILLED;
                clean_proc(parentproc);
            }
        }
        pids.Set(proc->pid, false);
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
                free(thread->fpu_storage - hhdm_tag->addr);
                // TODO: Fix this
                // free(thread->stack_phys); // Trple fault
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

size_t switchThread(registers_t *regs, thread_t *thread)
{
    if (this_proc() && this_thread())
    {
        this_thread()->regs = *regs;
        this_cpu->fpu_save(this_thread()->fpu_storage);

        if (this_thread()->state == RUNNING) this_thread()->state = READY;
    }

    this_cpu->current_thread = thread;
    this_cpu->current_proc = thread->parent;

    *regs = thread->regs;
    this_cpu->fpu_restore(thread->fpu_storage);
    vmm::switchPagemap(thread->parent->pagemap);

    thread->state = RUNNING;

    return thread->priority;
}

void schedule(registers_t *regs)
{
    if (die) while (true) asm volatile ("cli; hlt");
    if (initproc == nullptr)
    {
        yield();
        return;
    }
    lockit(sched_lock);

    uint64_t timeslice = MID;

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

                timeslice = switchThread(regs, thread);
                goto success;
            }
        }
        goto idle;
    }
    else
    {
        size_t this_proc_id = proc_table.find(this_proc());
        size_t this_thread_id = this_proc()->threads.find(this_thread());

        for (size_t t = this_thread_id + 1; t < this_proc()->threads.size(); t++)
        {
            thread_t *thread = this_proc()->threads[t];

            if (this_proc()->state != READY) break;
            if (thread->state != READY) continue;

            clean_proc(this_proc());

            timeslice = switchThread(regs, thread);
            goto success;
        }
        for (size_t p = this_proc_id + 1; p < proc_table.size(); p++)
        {
            process_t *proc = proc_table[p];
            if (proc->state != READY) continue;

            for (size_t t = 0; t < proc->threads.size(); t++)
            {
                thread_t *thread = proc->threads[t];
                if (thread->state != READY) continue;

                clean_proc(this_proc());

                timeslice = switchThread(regs, thread);
                goto success;
            }
        }
        for (size_t p = 0; p < this_proc_id + 1; p++)
        {
            process_t *proc = proc_table[p];
            if (proc->state != READY) continue;

            for (size_t t = 0; t < proc->threads.size(); t++)
            {
                thread_t *thread = proc->threads[t];
                if (thread->state != READY) continue;

                clean_proc(this_proc());

                timeslice = switchThread(regs, thread);
                goto success;
            }
        }
    }
    goto idle;

    success:
    if (debug) log("Running process[%d]->thread[%d] on CPU core %zu with timeslice: %zu", this_proc()->pid - 1, this_thread()->tid - 1, this_cpu->id, timeslice);

    yield(timeslice);
    return;

    idle:
    clean_proc(this_proc());

    if (this_cpu->idle_proc == nullptr)
    {
        this_cpu->idle_proc = new process_t("Idle Process", reinterpret_cast<uint64_t>(idle), 0, LOW, false);
        thread_count--;
    }

    timeslice = switchThread(regs, this_cpu->idle_proc->threads.front());
    if (debug) log("Running Idle process on CPU core %zu", this_cpu->id);

    yield(timeslice);
}

void kill()
{
    asm volatile ("cli");
    die = true;
    asm volatile ("sti");
}

bool idt_init = false;
void init()
{
    if (apic::initialised)
    {
        if (sched_vector == 0) sched_vector = idt::alloc_vector();
        if (!idt_init)
        {
            idt::register_interrupt_handler(sched_vector, schedule, true);
            idt::idt_set_descriptor(sched_vector, idt::int_table[sched_vector], 0x8E, 1);
            idt_init = true;
        }
        apic::lapic_oneshot(sched_vector);
    }
    else
    {
        if (!idt_init)
        {
            idt::idt_set_descriptor(idt::IRQ0, idt::int_table[idt::IRQ0], 0x8E, 1);
            idt_init = true;
        }
        if (initproc != nullptr) pit::schedule = true;
    }
    while (true) asm volatile ("hlt");
}
}

scheduler::thread_t *this_thread()
{
    asm volatile ("cli");
    scheduler::thread_t *thread = this_cpu->current_thread;
    asm volatile ("sti");
    return thread;
}

scheduler::process_t *this_proc()
{
    asm volatile ("cli");
    scheduler::process_t *proc = this_cpu->current_proc;
    asm volatile ("sti");
    return proc;
}