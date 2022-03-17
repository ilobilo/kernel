// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <stivale2.h>

static constexpr uint64_t STACK_SIZE = 0x2000;

extern uint8_t kernel_stack[];

extern struct stivale2_struct_tag_terminal *term_tag;
extern struct stivale2_struct_tag_framebuffer *frm_tag;
extern struct stivale2_struct_tag_smp *smp_tag;
extern struct stivale2_struct_tag_memmap *mmap_tag;
extern struct stivale2_struct_tag_rsdp *rsdp_tag;
extern struct stivale2_struct_tag_modules *mod_tag;
extern struct stivale2_struct_tag_cmdline *cmd_tag;
extern struct stivale2_struct_tag_kernel_file_v2 *kfilev2_tag;
extern struct stivale2_struct_tag_epoch *epoch_tag;
extern struct stivale2_struct_tag_hhdm *hhdm_tag;
extern struct stivale2_struct_tag_pmrs *pmrs_tag;
extern struct stivale2_struct_tag_kernel_base_address *kbad_tag;

extern char *cmdline;

void *get_tag(stivale2_struct *stivale, uint64_t id);

stivale2_module *find_module(const char *name);