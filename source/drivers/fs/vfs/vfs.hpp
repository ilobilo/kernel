// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/vector.hpp>
#include <stdint.h>

namespace kernel::drivers::fs::vfs {

#define FILENAME_LENGTH 256
#define ROOTNAME "[ROOT]"

enum filetypes
{
    FS_FILE = 0x01,
    FS_DIRECTORY = 0x02,
    FS_CHARDEVICE = 0x03,
    FS_BLOCKDEVICE = 0x04,
    FS_PIPE = 0x05,
    FS_SYMLINK = 0x06,
    FS_MOUNTPOINT = 0x08
};

struct fs_node_t;

using read_t = size_t (*)(fs_node_t*, size_t, size_t, char*);
using write_t = size_t (*)(fs_node_t*, size_t, size_t, char*);
using open_t = void (*)(fs_node_t*, uint8_t, uint8_t);
using close_t = void (*)(fs_node_t*);

struct fs_t
{
    char name[FILENAME_LENGTH];
    read_t read;
    write_t write;
    open_t open;
    close_t close;
};

struct fs_node_t
{
    char name[FILENAME_LENGTH];
    uint64_t mode;
    uint64_t uid;
    uint64_t gid;
    uint64_t flags;
    uint64_t inode;
    uint64_t address;
    uint64_t length;
    fs_node_t *ptr;
    fs_t *fs;
    fs_node_t *parent;
    vector<fs_node_t*> children;
};

extern bool initialised;
extern bool debug;
extern fs_node_t *fs_root;

size_t read_fs(fs_node_t *node, size_t offset, size_t size, char *buffer);
size_t write_fs(fs_node_t *node, size_t offset, size_t size, char *buffer);
void open_fs(fs_node_t *node, uint8_t read, uint8_t write);
void close_fs(fs_node_t *node);

fs_node_t *getchild(fs_node_t *parent, const char *path);
fs_node_t *add_new_child(fs_node_t *parent, const char *name);
void remove_child(fs_node_t *parent, const char *name);

char* node2path(fs_node_t *node);

fs_node_t *open(fs_node_t *parent, const char *path, bool create = false);

fs_node_t *mount_root(fs_t *fs);
fs_node_t *mount(fs_t *fs, fs_node_t *parent, const char *path);

void init();
}