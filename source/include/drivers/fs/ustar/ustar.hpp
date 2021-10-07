#pragma once

#include <drivers/fs/vfs/vfs.hpp>

#define HEADER_SIZE 128

enum ustar_filetypes
{
    REGULAR_FILE = '0',
    HARDLINK = '1',
    SYMLINK = '2',
    CHARDEV = '3',
    BLOCKDEV = '4',
    DIRECTORY = '5',
    FIFO = '6'
};

struct ustar_file_header_t
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

struct ustar_header_t
{
    ustar_file_header_t *header;
    unsigned int address;
};

extern bool initrd;
extern uint64_t ustar_filecount;
extern ustar_header_t *ustar_headers;

unsigned int getsize(const char *s);

void ustar_list();

char *ustar_cat(char *name);

int ustar_getid(char *name);

int ustar_search(char *filename, char **contents);

void initrd_init(unsigned int address);