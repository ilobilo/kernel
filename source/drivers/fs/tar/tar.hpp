#pragma once

#include <drivers/fs/vfs/vfs.hpp>

#define HEADER_SIZE 128

struct tar_file_header_t
{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];
};

struct tar_header_t
{
    tar_file_header_t* headers[HEADER_SIZE];
    unsigned int address[HEADER_SIZE];
    int count;
};

extern tar_header_t* tar_headers;

unsigned int getsize(const char *in);

void initrd_list();

void initrd_cat(char* name);

int initrd_getid(char* name);

void initrd_init(unsigned int address);
