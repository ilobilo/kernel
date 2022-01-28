// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/alloc.hpp>
#include <lib/log.hpp>

using namespace kernel::drivers::display;

namespace kernel::drivers::fs::ustar {

bool initialised = false;
vector<header_t*> headers;

vfs::fs_node_t *initrd_root;

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
        if (strcmp(header->signature, "ustar")) break;

        memmove(header->name, header->name + 1, strlen(header->name));
        uintptr_t size = getsize(header->size);
        if (header->name[strlen(header->name) - 1] == '/') header->name[strlen(header->name) - 1] = 0;

        headers.push_back(new header_t);
        headers.back()->header = header;
        headers.back()->size = size;
        headers.back()->address = address + 512;

        vfs::fs_node_t *node = vfs::open_r(nullptr, headers.back()->header->name);

        node->mode = string2int(headers.back()->header->mode);
        node->address = headers.back()->address;
        node->length = headers.back()->size;
        node->gid = getsize(headers.back()->header->gid);
        node->uid = getsize(headers.back()->header->uid);
        node->inode = i;

        switch (headers.back()->header->typeflag[0])
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
    log("Mounting USTAR initrd");

    if (initialised)
    {
        warn("USTAR initrd has already been mounted!\n");
        return;
    }

    initrd_root = vfs::mount_root(&ustar_fs);
    parse(address);

    serial::newline();
    initialised = true;
}
}