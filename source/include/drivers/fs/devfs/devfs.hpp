#pragma once

#include <drivers/fs/vfs/vfs.hpp>

namespace kernel::drivers::fs::devfs {

extern bool initialised;
extern vfs::fs_node_t *devfs_root;

extern uint64_t count;

vfs::fs_node_t *add(vfs::fs_t *fs, const char *name);

void init();
}