#pragma once

#include <stdint.h>

enum fs_filetypes
{
    FS_FILE = 0x01,
    FS_DIRECTORY = 0x02,
    FS_CHARDEVICE = 0x03,
    FS_BLOCKDEVICE = 0x04,
    FS_PIPE = 0x05,
    FS_SYMLINK = 0x06,
    FS_MOUNTPOINT = 0x08
};

struct fs_node;
struct dirent;

/*
typedef uint64_t (*read_t)(struct fs_node*, uint64_t, uint64_t, char*);
typedef uint64_t (*write_t)(struct fs_node*, uint64_t, uint64_t, char*);
typedef void (*open_t)(struct fs_node*);
typedef void (*close_t)(struct fs_node*);
typedef struct dirent * (*readdir_t)(struct fs_node*, uint64_t);
typedef struct fs_node * (*finddir_t)(struct fs_node*, char* name);
*/

using read_t = uint64_t (*)(fs_node*, uint64_t, uint64_t, char*);
using write_t = uint64_t (*)(fs_node*, uint64_t, uint64_t, char*);
using open_t = void (*)(fs_node*);
using close_t = void (*)(fs_node*);
using readdir_t = dirent* (*)(fs_node*, uint64_t);
using finddir_t = fs_node* (*)(fs_node*, char*);

struct fs_node
{
    char name[128];
    uint64_t mask;
    uint64_t uid;
    uint64_t gid;
    uint64_t flags;
    uint64_t inode;
    uint64_t length;
    uint64_t impl;
    read_t read;
    write_t write;
    open_t open;
    close_t close;
    readdir_t readdir;
    finddir_t finddir;
    struct fs_node* ptr;
};

struct dirent
{
    char name[128];
    uint64_t ino;
};

extern fs_node* fs_root;

uint64_t read_fs(fs_node* node, uint64_t offset, uint64_t size, char* buffer);
uint64_t write_fs(fs_node* node, uint64_t offset, uint64_t size, char* buffer);
void open_fs(fs_node* node, uint8_t read, uint8_t write);
void close_fs(fs_node* node);
dirent* readdir_fs(fs_node *node, uint64_t index);
fs_node* finddir_fs(fs_node *node, char* name);