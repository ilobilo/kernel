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

bool initialised = false;
static bool die = false;
bool debug = false;

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

thread_t::thread_t(uint64_t addr, uint64_t args, process_t *parent, priority_t priority)
{
    lockit(thread_lock);

    this->state = INITIAL;
    this->stack_phys = malloc<uint8_t*>(STACK_SIZE);
    this->stack = this->stack_phys + hhdm_offset;

    this->fpu_storage = malloc<uint8_t*>(this_cpu->fpu_storage_size) + hhdm_offset;
    this->fpu_storage_size = this_cpu->fpu_storage_size;
    this_cpu->fpu_save(this->fpu_storage);

    this->regs.rflags = 0x202;
    this->regs.cs = gdt::GDT_CODE_64;
    this->regs.ss = gdt::GDT_DATA_64;

    this->regs.rip = reinterpret_cast<uint64_t>(func_wrapper);
    this->regs.rdi = addr;
    this->regs.rsi = args;
    this->regs.rsp = reinterpret_cast<uint64_t>(this->stack) + STACK_SIZE;

    this->priority = priority;
    this->parent = parent;
    this->user = false;

    this->gsbase = reinterpret_cast<uint64_t>(this);
    this->fsbase = 0;
}

thread_t::thread_t(process_t *parent, priority_t priority, Auxval auxval, vector<std::string> argv, vector<std::string> envp)
{
    if (parent == nullptr)
    {
        error("Parent can not be null!");
        return;
    }

    lockit(thread_lock);

    this->priority = priority;
    this->parent = parent;
    this->user = true;

    this->state = INITIAL;
    this->stack_phys = malloc<uint8_t*>(STACK_SIZE);
    this->stack = this->stack_phys + hhdm_offset;

    this->kstack_phys = malloc<uint8_t*>(STACK_SIZE);
    this->kstack = this->kstack_phys + hhdm_offset;

    uint64_t stack_vma = this->parent->thread_stack_top;
    this->parent->thread_stack_top -= STACK_SIZE;

    uint64_t stack_bottom_vma = this->parent->thread_stack_top;
    this->parent->thread_stack_top -= vmm::page_size;

    this->parent->pagemap->mapRange(stack_bottom_vma, reinterpret_cast<uint64_t>(this->stack_phys), STACK_SIZE, vmm::ProtRead | vmm::ProtWrite, vmm::MapAnon);
    this->parent->pagemap->switchTo();
    this->stack = reinterpret_cast<uint8_t*>(stack_bottom_vma);

    this->fpu_storage = malloc<uint8_t*>(this_cpu->fpu_storage_size) + hhdm_offset;
    this->fpu_storage_size = this_cpu->fpu_storage_size;
    this_cpu->fpu_save(this->fpu_storage);

    this->regs.rflags = 0x202;
    this->regs.cs = gdt::GDT_USER_CODE_64 | 0x03;
    this->regs.ss = gdt::GDT_USER_DATA_64 | 0x03;

    this->regs.rip = reinterpret_cast<uint64_t>(func_wrapper);
    this->regs.rdi = auxval.entry;
    this->regs.rsp = reinterpret_cast<uint64_t>(this->stack + STACK_SIZE);

    this->gsbase = 0;
    this->fsbase = 0;

    uint8_t *tmpstack = reinterpret_cast<uint8_t*>(this->regs.rsp);
    uint64_t orig_stack_vma = stack_vma;

    for (std::string seg : envp)
    {
        tmpstack = reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(tmpstack) - (seg.length() + 1));
        strcpy(reinterpret_cast<char*>(tmpstack), seg.c_str());
    }
    for (std::string seg : argv)
    {
        tmpstack = reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(tmpstack) - (seg.length() + 1));
        strcpy(reinterpret_cast<char*>(tmpstack), seg.c_str());
    }
    tmpstack = reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(tmpstack) - (reinterpret_cast<uint64_t>(tmpstack) & 0x0F));

    if ((argv.size() + envp.size() + 1) & 1) tmpstack--;

    *(--tmpstack) = 0; *(--tmpstack) = 0;
    tmpstack -= 2; *tmpstack = AT_ENTRY; *(tmpstack + 1) = auxval.entry;
    tmpstack -= 2; *tmpstack = AT_PHDR; *(tmpstack + 1) = auxval.phdr;
    tmpstack -= 2; *tmpstack = AT_PHENT; *(tmpstack + 1) = auxval.phent;
    tmpstack -= 2; *tmpstack = AT_PHNUM; *(tmpstack + 1) = auxval.phnum;

    *(--tmpstack) = 0;

    tmpstack -= envp.size();
    for (size_t i = 0; i < envp.size(); i++)
    {
        orig_stack_vma -= envp[i].length() + 1;
        tmpstack[i] = orig_stack_vma;
    }

    *(--tmpstack) = 0;

    tmpstack -= argv.size();
    for (size_t i = 0; i < argv.size(); i++)
    {
        orig_stack_vma -= argv[i].length() + 1;
        tmpstack[i] = orig_stack_vma;
    }

    *(--tmpstack) = argv.size();
    this->regs.rsp -= reinterpret_cast<uint64_t>(this->stack) + STACK_SIZE - reinterpret_cast<uint64_t>(tmpstack);
}

thread_t *thread_t::fork(registers_t *regs)
{
    lockit(thread_lock);

    auto newthread = new thread_t;

    newthread->state = INITIAL;
    newthread->stack_phys = malloc<uint8_t*>(STACK_SIZE);
    newthread->stack = newthread->stack_phys + hhdm_offset;

    if (user)
    {
        newthread->kstack_phys = malloc<uint8_t*>(STACK_SIZE);
        newthread->kstack = newthread->kstack_phys + hhdm_offset;
    }

    newthread->fpu_storage = malloc<uint8_t*>(this_cpu->fpu_storage_size) + hhdm_offset;
    newthread->fpu_storage_size = this->fpu_storage_size;
    memcpy(newthread->fpu_storage, this->fpu_storage, this->fpu_storage_size);

    newthread->regs = *regs;

    uint64_t offset = reinterpret_cast<uint64_t>(newthread->stack) - reinterpret_cast<uint64_t>(this->stack);
    newthread->regs.rsp += offset;
    newthread->regs.rbp += offset;

    memcpy(newthread->stack, this->stack, STACK_SIZE);

    newthread->priority = this->priority;
    newthread->parent = this->parent;
    newthread->user = this->user;

    newthread->gsbase = (this->user ? this->gsbase : reinterpret_cast<uint64_t>(newthread));
    newthread->fsbase = this->fsbase;

    return newthread;
}

thread_t *process_t::add_user_thread(uint64_t addr, uint64_t args, priority_t priority, Auxval auxval, vector<std::string> argv, vector<std::string> envp)
{
    lockit(proc_lock);

    auto thread = new thread_t(this, priority, auxval, argv, envp);

    thread->tid = this->next_tid++;
    thread_count++;

    this->threads.push_back(thread);
    thread->state = READY;

    return thread;
}

thread_t *process_t::add_thread(uint64_t addr, uint64_t args, priority_t priority)
{
    lockit(proc_lock);

    auto thread = new thread_t(addr, args, this, priority);

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

    thread->tid = this->next_tid++;
    thread_count++;

    this->threads.push_back(thread);
    thread->state = READY;

    return thread;
}

bool process_t::enqueue()
{
    if (this->in_table || (this->children.size() == 0 && this->threads.size() == 0)) return false;
    lockit(proc_lock);

    if (initproc == nullptr) initproc = this;

    proc_table.push_back(this);
    proc_count++;

    this->state = READY;
    this->in_table = true;

    return true;
}

process_t::process_t(std::string name, uint64_t addr, uint64_t args, priority_t priority)
{
    lockit(proc_lock);

    this->name = name;
    this->pid = alloc_pid();
    this->state = INITIAL;

    this->pagemap = vmm::newPagemap();
    this->current_dir = vfs::fs_root;
    this->parent = nullptr;

    proc_lock.unlock();
    this->add_thread(addr, args, priority);
}

process_t::process_t(std::string name)
{
    lockit(proc_lock);

    this->name = name;
    this->pid = alloc_pid();
    this->state = INITIAL;

    this->pagemap = vmm::newPagemap();
    this->current_dir = vfs::fs_root;
    this->parent = nullptr;
}

process_t *start_program(vfs::fs_node_t *dir, std::string path, vector<std::string> argv, vector<std::string> envp, std::string stdin, std::string stdout, std::string stderr, std::string procname)
{
    auto prog = vfs::get_node(dir, path, true);
    if (prog == nullptr || prog->res == nullptr)
    {
        error("No such file or directory!");
        return nullptr;
    }

    // TODO: Shebang

    auto proc = new process_t(procname);
    auto [auxval, ld_path] = elf_load(proc->pagemap, prog->res, 0);
    uint64_t entry = 0;

    if (ld_path.empty()) entry = auxval.entry;
    else
    {
        auto ld_node = vfs::get_node(nullptr, ld_path, true);
        if (ld_node == nullptr || ld_node->res == nullptr)
        {
            error("Could not find dynamic linker!");
            delete proc;
            return nullptr;
        }
        entry = elf_load(proc->pagemap, ld_node->res, 0x40000000).auxval.entry;
    }

    auto stdin_node = vfs::get_node(nullptr, stdin, true);
    auto stdin_handle = new vfs::handle_t
    {
        .res = stdin_node->res,
        .node = stdin_node,
        .refcount = 1
    };
    auto stdin_fd = new vfs::fd_t { .handle = stdin_handle };
    proc->fds[0] = stdin_fd;

    auto stdout_node = vfs::get_node(nullptr, stdout, true);
    auto stdout_handle = new vfs::handle_t
    {
        .res = stdout_node->res,
        .node = stdout_node,
        .refcount = 1
    };
    auto stdout_fd = new vfs::fd_t { .handle = stdout_handle };
    proc->fds[1] = stdout_fd;

    auto stderr_node = vfs::get_node(nullptr, stderr, true);
    auto stderr_handle = new vfs::handle_t
    {
        .res = stderr_node->res,
        .node = stderr_node,
        .refcount = 1
    };
    auto stderr_fd = new vfs::fd_t { .handle = stderr_handle };
    proc->fds[2] = stderr_fd;

    proc->add_user_thread(entry, 0, MID, auxval, argv, envp);

    return proc;
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

void process_t::exit(bool halt)
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
    if (halt)
    {
        yield();
        while (true) asm volatile ("hlt");
    }
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

void thread_t::exit(bool halt)
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
    if (halt)
    {
        yield();
        while (true) asm volatile ("hlt");
    }
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
            free(thread->fpu_storage - hhdm_offset);
            // TODO: Fix this: Triple fault
            // free(thread->stack_phys);
            // if (thread->kstack_phys) free(thread->kstack_phys);
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
        proc->pagemap->deleteThis();
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
                free(thread->fpu_storage - hhdm_offset);
                // TODO: Fix this: Triple fault
                // free(thread->stack_phys);
                // if (thread->kstack_phys) free(thread->kstack_phys);
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
        this_proc()->pagemap->save();

        this_thread()->gsbase = get_kernel_gs();
        this_thread()->fsbase = get_fs();

        if (this_thread()->state == RUNNING) this_thread()->state = READY;
    }

    this_cpu->current_thread = thread;
    this_cpu->current_proc = thread->parent;

    *regs = this_thread()->regs;
    this_thread()->cpu = this_cpu->id;
    this_cpu->fpu_restore(this_thread()->fpu_storage);
    this_proc()->pagemap->switchTo();

    set_gs(reinterpret_cast<uint64_t>(thread));
    set_kernel_gs(this_thread()->user ? this_thread()->gsbase : reinterpret_cast<uint64_t>(thread));
    set_fs(this_thread()->fsbase);

    this_thread()->state = RUNNING;

    return thread->priority;
}

void schedule(registers_t *regs)
{
    if (die) while (true) asm volatile ("cli; hlt");
    if (initproc == nullptr || initialised == false)
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
            auto proc = proc_table[i];
            if (proc->state != READY)
            {
                clean_proc(proc);
                continue;
            }

            for (size_t t = 0; t < proc->threads.size(); t++)
            {
                auto thread = proc->threads[t];
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
            auto thread = this_proc()->threads[t];

            if (this_proc()->state != READY) break;
            if (thread->state != READY) continue;

            clean_proc(this_proc());

            timeslice = switchThread(regs, thread);
            goto success;
        }
        for (size_t p = this_proc_id + 1; p < proc_table.size(); p++)
        {
            auto proc = proc_table[p];
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
            auto proc = proc_table[p];
            if (proc->state != READY) continue;

            for (size_t t = 0; t < proc->threads.size(); t++)
            {
                auto thread = proc->threads[t];
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
        this_cpu->idle_proc = new process_t("Idle Process", reinterpret_cast<uint64_t>(idle), 0, LOW);
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
void init(bool last)
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
    if (last) initialised = true;
    while (true) asm volatile ("hlt");
}
}

scheduler::thread_t *this_thread()
{
    asm volatile ("cli");
    auto thread = this_cpu->current_thread;
    asm volatile ("sti");
    return thread;
}

scheduler::process_t *this_proc()
{
    asm volatile ("cli");
    auto proc = this_cpu->current_proc;
    asm volatile ("sti");
    return proc;
}