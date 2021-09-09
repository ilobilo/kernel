#pragma once

#include <stdint.h>

#define FS_FILE         0x01
#define FS_DIRECTORY    0x02
#define FS_CHARDEVICE   0x03
#define FS_BLOCKDEVICE  0x04
#define FS_PIPE         0x05
#define FS_SYMLINK      0x06
#define FS_MOUNTPOINT   0x08

typedef uint64_t (*read_t)(struct fs_node*, uint64_t, uint64_t, uint8_t*);
typedef uint64_t (*write_t)(struct fs_node*, uint64_t, uint64_t, uint8_t*);
typedef void (*open_t)(struct fs_node*);
typedef void (*close_t)(struct fs_node*);
typedef struct dirent * (*readdir_t)(struct fs_node*, uint64_t);
typedef struct fs_node * (*finddir_t)(struct fs_node*, char* name);

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

uint64_t read_fs(fs_node* node, uint64_t offset, uint64_t size, uint8_t* buffer);
uint64_t write_fs(fs_node* node, uint64_t offset, uint64_t size, uint8_t* buffer);
void open_fs(fs_node* node, uint8_t read, uint8_t write);
void close_fs(fs_node* node);
struct dirent* readdir_fs(fs_node *node, uint64_t index);
fs_node* finddir_fs(fs_node *node, char* name);