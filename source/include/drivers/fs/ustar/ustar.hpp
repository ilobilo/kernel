#pragma once

#include <drivers/fs/vfs/vfs.hpp>

namespace kernel::drivers::fs::ustar {

#define HEADER_SIZE 128

enum filetypes
{
    REGULAR_FILE = '0',
    HARDLINK = '1',
    SYMLINK = '2',
    CHARDEV = '3',
    BLOCKDEV = '4',
    DIRECTORY = '5',
    FIFO = '6'
};

struct file_header_t
{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];
    char link[100];
    char signature[6];
    char version[2];
    char owner[32];
    char group[32];
    char dev_maj[8];
    char dev_min[8];
    char prefix[155];
};

struct header_t
{
    file_header_t *header;
    unsigned int address;
};

extern bool initialised;
extern uint64_t filecount;
extern header_t *headers;

unsigned int getsize(const char *s);

void list();

char *cat(char *name);

int getid(const char *name);

int search(char *filename, char **contents);

void init(unsigned int address);
}