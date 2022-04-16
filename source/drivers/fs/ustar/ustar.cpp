// Copyright (C) 2021-2022  ilobilo

#include <drivers/fs/ustar/ustar.hpp>
#include <system/mm/pmm/pmm.hpp>
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

void init(uint64_t module)
{
    log("Initialising USTAR");

    if (initialised)
    {
        warn("USTAR has already been initialised!\n");
        return;
    }

    auto initrd_mod = reinterpret_cast<limine_file*>(module);
    uint64_t base = reinterpret_cast<uint64_t>(initrd_mod->address);
    uint64_t top = base + initrd_mod->size;
    uint64_t address = base;

    while (true)
    {
        file_header_t *header = reinterpret_cast<file_header_t*>(address);
        if (strcmp(header->signature, "ustar") || address >= top) break;

        uint64_t size = oct2dec(header->size);
        uint64_t mode = oct2dec(header->mode);
        std::string name(header->name);
        std::string link(header->link);

        vfs::fs_node_t *node = nullptr;
        if (name == "./") goto next;

        switch (header->typeflag[0])
        {
            case DIRECTORY:
                node = vfs::create(vfs::fs_root, name, mode | vfs::ifdir);
                if (node == nullptr)
                {
                    error("USTAR: Could not create directory %s", name.c_str());
                    goto next;
                }
                break;
            case REGULAR_FILE:
            {
                node = vfs::create(vfs::fs_root, name, mode | vfs::ifreg);
                if (node == nullptr)
                {
                    error("USTAR: Could not create file %s", name.c_str());
                    goto next;
                }
                uint8_t *buffer = reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(header) + 512);
                if (node->res->write(nullptr, buffer, 0, size) == -1)
                {
                    error("USTAR: Could not write to file %s", name.c_str());
                    goto next;
                }
                break;
            }
            case SYMLINK:
                node = vfs::symlink(vfs::fs_root, name, link);
                if (node == nullptr)
                {
                    error("USTAR: Could not create symlink %s", name.c_str());
                    goto next;
                }
                break;
        }

        node->res->stat.uid = oct2dec(header->uid);
        node->res->stat.gid = oct2dec(header->gid);

        next:
        address += 512 + ALIGN_UP(size, 512);
    }

    pmm::free(reinterpret_cast<void*>(ALIGN_DOWN(base - hhdm_offset, 0x1000)), ALIGN_UP(initrd_mod->size, 0x1000) / 0x1000);

    serial::newline();
    initialised = true;
}
}