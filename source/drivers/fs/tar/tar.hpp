#pragma once

struct tar_header
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

extern struct tar_header *headers[32];
extern int count;
extern unsigned int addrs[];

void tar_getaddrs(); 

void tar_list();

void tar_cat(char* name);

void tar_init(unsigned int address);