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
    address += (((getsize(reinterpret_cast<file_header_t*>(address)->size)) + 511) / 512 + 1) * 512;
    unsigned int i;
    for (i = 0; ; i++)
    {
        file_header_t *header = reinterpret_cast<file_header_t*>(address);
        memmove(header->name, header->name + 1, strlen(header->name));

        if (strcmp(header->signature, "ustar")) break;

        if (filecount >= (allocsize(headers) / sizeof(header_t)))
        {
            allocated += 5;
            headers = static_cast<header_t*>(realloc(headers, allocated * sizeof(header_t)));
        }

        uintptr_t size = getsize(header->size);
        if (header->name[strlen(header->name) - 1] == '/') header->name[strlen(header->name) - 1] = 0;
        headers[i].size = size;
        headers[i].header = header;
        headers[i].address = address + 512;
        filecount++;

        vfs::fs_node_t *node = vfs::open_r(nullptr, headers[i].header->name);

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
                break;
            case filetypes::SYMLINK:
                node->flags = vfs::FS_SYMLINK;
                break;
            case filetypes::DIRECTORY:
                node->flags = vfs::FS_DIRECTORY;
                break;
            case filetypes::CHARDEV:
                node->flags = vfs::FS_CHARDEVICE;
                break;
            case filetypes::BLOCKDEV:
                node->flags = vfs::FS_BLOCKDEVICE;
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

char *read(const char *filename)
{
    if (!check()) return nullptr;

    for (uint64_t i = 0; i < filecount; i++)
    {
        if (!strcmp(headers[i].header->name, filename))
        {
            return reinterpret_cast<char*>(headers[i].address);
        }
    }
    return nullptr;
}

static size_t ustar_read(vfs::fs_node_t *node, size_t offset, size_t size, char *buffer)
{
    if (!size) size = node->length;
    if (offset > node->length) return 0;
    if (offset + size > node->length) size = node->length - offset;
    memcpy(buffer, reinterpret_cast<uint8_t*>(node->address + offset), size);
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

    headers = static_cast<header_t*>(malloc(allocated * sizeof(header_t)));

    initrd_root = vfs::mount_root(&ustar_fs);
    parse(address);

    serial::newline();
    initialised = true;
}
}