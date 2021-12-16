// Copyright (C) 2021  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/buddy.hpp>

using namespace kernel::drivers::display;

namespace kernel::drivers::fs::ustar {

bool initialised = false;
uint64_t filecount;
header_t *headers;

vfs::fs_node_t *initrd_root;

uint64_t allocated = 10;

unsigned int getsize(const char *s)
{
    uint64_t ret = 0;
    while (*s)
    {
        ret *= 8;
        ret += *s - '0';
        s++;
    }
    return ret;
}

int parse(unsigned int address)
{
    address += (((getsize(((file_header_t*)((uintptr_t)address))->size) + 511) / 512) + 1) * 512;
    unsigned int i;
    for (i = 0; ; i++)
    {
        file_header_t *header = (file_header_t*)((uintptr_t)address);
        memmove(header->name, header->name + 1, strlen(header->name));

        if (strcmp(header->signature, "ustar")) break;

        if (filecount >= (allocsize(headers) / sizeof(header_t)))
        {
            allocated += 5;
            headers = (header_t*)realloc(headers, allocated * sizeof(header_t));
        }

        uintptr_t size = getsize(header->size);
        if (header->name[strlen(header->name) - 1] == '/') header->name[strlen(header->name) - 1] = 0;
        headers[i].size = size;
        headers[i].header = header;
        headers[i].address = address + 512;
        filecount++;

        vfs::fs_node_t *node = vfs::open_r(NULL, headers[i].header->name);

        node->mask = string2int(headers[i].header->mode);
        node->address = headers[i].address;
        node->length = headers[i].size;
        node->gid = getsize(header->gid);
        node->uid = getsize(header->uid);
        node->inode = i;

        switch (headers[i].header->typeflag[0])
        {
            case filetypes::REGULAR_FILE:
                node->flags = vfs::FS_FILE;
                node->children.destroy();
                break;
            case filetypes::SYMLINK:
                node->flags = vfs::FS_SYMLINK;
                node->children.destroy();
                break;
            case filetypes::DIRECTORY:
                node->flags = vfs::FS_DIRECTORY;
                break;
            case filetypes::CHARDEV:
                node->flags = vfs::FS_CHARDEVICE;
                node->children.destroy();
                break;
            case filetypes::BLOCKDEV:
                node->flags = vfs::FS_BLOCKDEVICE;
                node->children.destroy();
                break;
        }

        address += (((size + 511) / 512) + 1) * 512;
    }
    return i;
}

bool check()
{
    if (!initialised)
    {
        printf("\033[31mUSTAR filesystem has not been initialised!%s\n", terminal::colour);
        return false;
    }
    return true;
}

int getid(const char *name)
{
    for (uint64_t i = 0; i < filecount; ++i)
    {
        if(!strcmp(headers[i].header->name, name)) return i;
    }
    return 0;
}

int search(const char *filename, char **contents)
{
    if (!check()) return 0;

    for (uint64_t i = 0; i < filecount; i++)
    {
        if (!strcmp(headers[i].header->name, filename))
        {
            *contents = (char*)((uintptr_t)headers[i].address);
            return 1;
        }
    }
    return 0;
}

static size_t ustar_read(vfs::fs_node_t *node, size_t offset, size_t size, char *buffer)
{
    if (!size) size = node->length;
    if (offset > node->length) return 0;
    if (offset + size > node->length) size = node->length - offset;
    memcpy(buffer, (char*)(node->address + offset), size);
    return size;
}

static vfs::fs_t ustar_fs = {
    .name = "USTAR",
    .read = &ustar_read
};

void init(unsigned int address)
{
    serial::info("Mounting USTAR initrd");

    if (initialised)
    {
        serial::warn("USTAR initrd has already been mounted!\n");
        return;
    }

    headers = (header_t*)malloc(allocated * sizeof(header_t));

    initrd_root = vfs::mount_root(&ustar_fs);
    parse(address);

    serial::newline();
    initialised = true;
}
}