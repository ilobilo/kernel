// Copyright (C) 2021-2022  ilobilo

#include <drivers/fs/ustar/ustar.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/mm/vmm/vmm.hpp>
#include <system/vfs/vfs.hpp>
#include <kernel/kernel.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/math.hpp>
#include <lib/log.hpp>

using namespace kernel::system::mm;
using namespace kernel::system;

namespace kernel::drivers::fs::ustar {

bool initialised = false;

uint64_t oct2dec(const char *s)
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

void init(uint64_t address)
{
    log("Initialising initrd");

    if (initialised)
    {
        warn("Initrd has already been initialised!\n");
        return;
    }

    while (true)
    {
        file_header_t *header = reinterpret_cast<file_header_t*>(address);
        if (strcmp(header->signature, "ustar")) break;

        uint64_t size = oct2dec(header->size);
        uint64_t mode = oct2dec(header->mode);
        string name(header->name);
        string link(header->link);

        vfs::fs_node_t *node = nullptr;
        if (name == "./") goto next;

        switch (header->typeflag[0])
        {
            case DIRECTORY:
                node = vfs::create(vfs::fs_root, name, mode | vfs::ifdir);
                if (node == nullptr)
                {
                    error("Initrd: Could not create directory %s", name.c_str());
                    break;
                }
                break;
            case REGULAR_FILE:
            {
                node = vfs::create(vfs::fs_root, name, mode | vfs::ifreg);
                if (node == nullptr)
                {
                    error("Initrd: Could not create file %s", name.c_str());
                    break;
                }
                uint8_t *buffer = reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(header) + 512);
                if (node->res->write(nullptr, buffer, 0, size) == -1)
                {
                    error("Initrd: Could not write to file %s", name.c_str());
                    break;
                }
                break;
            }
            case SYMLINK:
                node = vfs::symlink(vfs::fs_root, name, link);
                if (node == nullptr)
                {
                    error("Initrd: Could not create symlink %s", name.c_str());
                    break;
                }
                break;
        }

        node->res->stat.uid = oct2dec(header->uid);
        node->res->stat.gid = oct2dec(header->gid);

        next:
        pmm::free(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(header) - vmm::hhdm_offset), (512 + ALIGN_UP(size, 512)) / 0x1000);
        address += 512 + ALIGN_UP(size, 512);
    }

    serial::newline();
    initialised = true;
}
}