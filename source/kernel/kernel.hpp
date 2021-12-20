// Copyright (C) 2021  ilobilo

#pragma once

#include <stivale2.h>

extern struct stivale2_struct_tag_smp *smp_tag;
extern struct stivale2_struct_tag_memmap *mmap_tag;
extern struct stivale2_struct_tag_rsdp *rsdp_tag;
extern struct stivale2_struct_tag_framebuffer *frm_tag;
extern struct stivale2_struct_tag_terminal *term_tag;
extern struct stivale2_struct_tag_modules *mod_tag;
extern struct stivale2_struct_tag_cmdline *cmd_tag;
extern struct stivale2_struct_tag_kernel_file_v2 *kfilev2_tag;

extern char *cmdline;

void *stivale2_get_tag(stivale2_struct *stivale, uint64_t id);

int find_module(const char *name);