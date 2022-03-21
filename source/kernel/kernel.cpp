// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/framebuffer/framebuffer.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <drivers/display/ssfn/ssfn.hpp>
#include <kernel/kernel.hpp>
#include <kernel/main.hpp>
#include <lib/string.hpp>
#include <lib/panic.hpp>
#include <lib/cpu.hpp>
#include <stivale2.h>
#include <cstddef>

using namespace kernel::drivers::display;

struct stivale2_struct_tag_framebuffer *frm_tag;
struct stivale2_struct_tag_terminal *term_tag;
struct stivale2_struct_tag_smp *smp_tag;
struct stivale2_struct_tag_memmap *mmap_tag;
struct stivale2_struct_tag_rsdp *rsdp_tag;
struct stivale2_struct_tag_modules *mod_tag;
struct stivale2_struct_tag_cmdline *cmd_tag;
struct stivale2_struct_tag_kernel_file_v2 *kfilev2_tag;
struct stivale2_struct_tag_epoch *epoch_tag;
struct stivale2_struct_tag_hhdm *hhdm_tag;
struct stivale2_struct_tag_pmrs *pmrs_tag;
struct stivale2_struct_tag_kernel_base_address *kbad_tag;

char *cmdline;

uint8_t kernel_stack[STACK_SIZE] = { [0 ... STACK_SIZE - 1] = 'A' };

static struct stivale2_header_tag_terminal terminal_hdr_tag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_TERMINAL_ID,
        .next = 0
    },
    .flags = 0,
    .callback = 0
};

static struct stivale2_header_tag_smp smp_hdr_tag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_SMP_ID,
        .next = reinterpret_cast<uint64_t>(&terminal_hdr_tag)
    },
    .flags = 0
};

static struct stivale2_header_tag_framebuffer framebuffer_hdr_tag = {
    .tag = {
        .identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
        .next = reinterpret_cast<uint64_t>(&smp_hdr_tag)
    },
    .framebuffer_width = 0,
    .framebuffer_height = 0,
    .framebuffer_bpp = 0,
    .unused = 0
};

static struct stivale2_tag unmap_null_tag = {
    .identifier = STIVALE2_HEADER_TAG_UNMAP_NULL_ID,
    .next = reinterpret_cast<uint64_t>(&framebuffer_hdr_tag)
};

#if (LVL5_PAGING != 0)
static struct stivale2_tag lvl5_hdr_tag = {
    .identifier = STIVALE2_HEADER_TAG_5LV_PAGING_ID,
    .next = reinterpret_cast<uint64_t>(&unmap_null_tag)
};
#endif

[[gnu::section(".stivale2hdr"), gnu::used]]
static struct stivale2_header stivale_hdr = {
    .entry_point = 0,
    .stack = reinterpret_cast<uintptr_t>(kernel_stack) + STACK_SIZE,
    .flags = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4),
#if (LVL5_PAGING != 0)
    .tags = reinterpret_cast<uintptr_t>(&lvl5_hdr_tag)
#else
    .tags = reinterpret_cast<uintptr_t>(&unmap_null_tag)
#endif
};

template<typename type>
type *get_tag(stivale2_struct *stivale, uint64_t id)
{
    stivale2_tag *current_tag = reinterpret_cast<stivale2_tag*>(stivale->tags);
    while (current_tag)
    {
        if (current_tag->identifier == id) return reinterpret_cast<type*>(current_tag);

        current_tag = reinterpret_cast<stivale2_tag*>(current_tag->next);
    }
    return nullptr;
}

stivale2_module *find_module(const char *name)
{
    for (size_t i = 0; i < mod_tag->module_count; i++)
    {
        if (!strcmp(mod_tag->modules[i].string, name)) return &mod_tag->modules[i];
    }
    return nullptr;
}

extern "C" void _start(stivale2_struct *stivale2_struct)
{
    enableSSE();
    enableSMEP();
    enableSMAP();
    enableUMIP();
    enablePAT();

    serial::early_init();

    term_tag = get_tag<stivale2_struct_tag_terminal>(stivale2_struct, STIVALE2_STRUCT_TAG_TERMINAL_ID);
    frm_tag = get_tag<stivale2_struct_tag_framebuffer>(stivale2_struct, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);
    smp_tag = get_tag<stivale2_struct_tag_smp>(stivale2_struct, STIVALE2_STRUCT_TAG_SMP_ID);
    mmap_tag = get_tag<stivale2_struct_tag_memmap>(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);
    rsdp_tag = get_tag<stivale2_struct_tag_rsdp>(stivale2_struct, STIVALE2_STRUCT_TAG_RSDP_ID);
    mod_tag = get_tag<stivale2_struct_tag_modules>(stivale2_struct, STIVALE2_STRUCT_TAG_MODULES_ID);
    cmd_tag = get_tag<stivale2_struct_tag_cmdline>(stivale2_struct, STIVALE2_STRUCT_TAG_CMDLINE_ID);
    kfilev2_tag = get_tag<stivale2_struct_tag_kernel_file_v2>(stivale2_struct, STIVALE2_STRUCT_TAG_KERNEL_FILE_V2_ID);
    epoch_tag = get_tag<stivale2_struct_tag_epoch>(stivale2_struct, STIVALE2_STRUCT_TAG_EPOCH_ID);
    hhdm_tag = get_tag<stivale2_struct_tag_hhdm>(stivale2_struct, STIVALE2_STRUCT_TAG_HHDM_ID);
    pmrs_tag = get_tag<stivale2_struct_tag_pmrs>(stivale2_struct, STIVALE2_STRUCT_TAG_PMRS_ID);
    kbad_tag = get_tag<stivale2_struct_tag_kernel_base_address>(stivale2_struct, STIVALE2_STRUCT_TAG_KERNEL_BASE_ADDRESS_ID);

    assert(term_tag, "Could not get terminal structure tag!");
    terminal::init();

    assert(frm_tag, "Could not get framebuffer structure tag!");
    framebuffer::init();
    ssfn::init();

    assert(smp_tag, "Could not get smp structure tag!");
    assert(mmap_tag, "Could not get memmap structure tag!");
    assert(rsdp_tag, "Could not get rsdp structure tag!");
    assert(mod_tag, "Could not get modules structure tag!");
    assert(cmd_tag, "Could not get cmdline structure tag!");
    cmdline = reinterpret_cast<char*>(cmd_tag->cmdline);

    assert(kfilev2_tag, "Could not get kernel file v2 structure tag!");
    assert(epoch_tag, "Could not get epoch structure tag!");
    assert(hhdm_tag, "Could not get hhdm structure tag!");
    assert(pmrs_tag, "Could not get pmrs structure tag!");
    assert(kbad_tag, "Could not get kernel base address structure tag!");

    kernel::main();

    while (true) asm volatile ("hlt");
}