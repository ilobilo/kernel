// Copyright (C) 2021  ilobilo

#include <drivers/display/framebuffer/framebuffer.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <drivers/display/ssfn/ssfn.hpp>
#include <kernel/main.hpp>
#include <lib/string.hpp>
#include <lib/panic.hpp>
#include <lib/cpu.hpp>
#include <stivale2.h>
#include <stddef.h>

using namespace kernel::drivers::display;

struct stivale2_struct_tag_smp *smp_tag;
struct stivale2_struct_tag_memmap *mmap_tag;
struct stivale2_struct_tag_rsdp *rsdp_tag;
struct stivale2_struct_tag_framebuffer *frm_tag;
struct stivale2_struct_tag_terminal *term_tag;
struct stivale2_struct_tag_modules *mod_tag;
struct stivale2_struct_tag_cmdline *cmd_tag;
struct stivale2_struct_tag_kernel_file_v2 *kfilev2_tag;
struct stivale2_struct_tag_epoch *epoch_tag;

char *cmdline;

static uint8_t stack[8192];

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

#if (LVL5_PAGING != 0)
static struct stivale2_tag lvl5_hdr_tag = {
    .identifier = STIVALE2_HEADER_TAG_5LV_PAGING_ID,
    .next = reinterpret_cast<uint64_t>(&framebuffer_hdr_tag)
};
#endif

[[gnu::section(".stivale2hdr"), gnu::used]]
static struct stivale2_header stivale_hdr = {
    .entry_point = 0,
    .stack = reinterpret_cast<uintptr_t>(stack) + sizeof(stack),
    .flags = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4),
#if (LVL5_PAGING != 0)
    .tags = reinterpret_cast<uintptr_t>(&lvl5_hdr_tag)
#else
    .tags = reinterpret_cast<uintptr_t>(&framebuffer_hdr_tag)
#endif
};

void *stivale2_get_tag(stivale2_struct *stivale, uint64_t id)
{
    stivale2_tag *current_tag = reinterpret_cast<stivale2_tag*>(stivale->tags);
    while (true)
    {
        if (current_tag == nullptr) return nullptr;

        if (current_tag->identifier == id) return current_tag;

        current_tag = reinterpret_cast<stivale2_tag*>(current_tag->next);
    }
}

int find_module(const char *name)
{
    for (uint64_t i = 0; i < mod_tag->module_count; i++)
    {
        if (!strcmp(mod_tag->modules[i].string, name)) return i;
    }
    return -1;
}

extern "C" void _start(stivale2_struct *stivale2_struct)
{
    smp_tag = static_cast<stivale2_struct_tag_smp*>(stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_SMP_ID));
    mmap_tag = static_cast<stivale2_struct_tag_memmap*>(stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID));
    rsdp_tag = static_cast<stivale2_struct_tag_rsdp*>(stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_RSDP_ID));
    frm_tag = static_cast<stivale2_struct_tag_framebuffer*>(stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID));
    term_tag = static_cast<stivale2_struct_tag_terminal*>(stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_TERMINAL_ID));
    mod_tag = static_cast<stivale2_struct_tag_modules*>(stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MODULES_ID));
    cmd_tag = static_cast<stivale2_struct_tag_cmdline*>(stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_CMDLINE_ID));
    kfilev2_tag = static_cast<stivale2_struct_tag_kernel_file_v2*>(stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_KERNEL_FILE_V2_ID));
    epoch_tag = static_cast<stivale2_struct_tag_epoch*>(stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_EPOCH_ID));

    cmdline = reinterpret_cast<char*>(cmd_tag->cmdline);

    if (!strstr(cmdline, "nocom")) serial::init();

    if (frm_tag == nullptr) PANIC("Could not find framebuffer tag!");
    framebuffer::init();
    ssfn::init();

    if (term_tag == nullptr) PANIC("Could not find terminal tag!");
    terminal::init();

    kernel::main();

    while (true) asm volatile ("hlt");
}