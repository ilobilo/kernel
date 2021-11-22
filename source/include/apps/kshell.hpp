// Copyright (C) 2021  ilobilo

#pragma once

#include <drivers/fs/vfs/vfs.hpp>

using namespace kernel::drivers::fs;

namespace kernel::apps::kshell {
extern vfs::fs_node_t *current_path;

void run();
}