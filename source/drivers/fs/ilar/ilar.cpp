// Copyright (C) 2021-2022  ilobilo

#include <drivers/fs/ilar/ilar.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/vfs/vfs.hpp>
#include <kernel/kernel.hpp>
#include <lib/string.hpp>
#include <lib/math.hpp>
#include <lib/log.hpp>
#include <limine.h>

using namespace kernel::system::mm;
using namespace kernel::system;

namespace kernel::drivers::fs::ilar {

bool initialised = false;

void init(uint64_t module)
{
    log("Initialising ILAR");

    if (initialised)
    {
        warn("ILAR has already been initialised!\n");
        return;
    }

    auto initrd_mod = reinterpret_cast<limine_file*>(module);
    uint64_t base = reinterpret_cast<uint64_t>(initrd_mod->address);
    uint64_t top = base + initrd_mod->size;
    uint64_t address = base;

    fileheader *file = reinterpret_cast<fileheader*>(address);
    if (strcmp(file->signature, ILAR_SIGNATURE))
    {
        error("ILAR: Invalid signature!\n");
        return;
    }

    while (true)
    {
        uint64_t size = file->size;
        uint32_t mode = file->mode;
        std::string name(file->name);
        std::string link(file->link);
        if (name[0] == '/') name.erase(0, 1);
        if (link[0] == '/') link.erase(0, 1);

        vfs::fs_node_t *node = nullptr;
        switch (file->type)
        {
            case ILAR_DIRECTORY:
                node = vfs::create(vfs::fs_root, name, mode | vfs::ifdir);
                if (node == nullptr)
                {
                    error("ILAR: Could not create directory %s", name.c_str());
                    goto next;
                }
                break;
            case ILAR_REGULAR:
            {
                node = vfs::create(vfs::fs_root, name, mode | vfs::ifreg);
                if (node == nullptr)
                {
                    error("ILAR: Could not create file %s", name.c_str());
                    goto next;
                }
                uint8_t *buffer = reinterpret_cast<uint8_t*>(address + sizeof(fileheader));
                if (node->res->write(nullptr, buffer, 0, size) == -1)
                {
                    error("ILAR: Could not write to file %s", name.c_str());
                    goto next;
                }
                break;
            }
            case ILAR_SYMLINK:
                node = vfs::symlink(vfs::fs_root, name, link);
                if (node == nullptr)
                {
                    error("Initrd: Could not create symlink %s", name.c_str());
                    goto next;
                }
                break;
        }

        next:
        address += sizeof(fileheader) + file->size;

        file = reinterpret_cast<fileheader*>(address);
        if (strcmp(file->signature, ILAR_SIGNATURE) || address >= top) break;
    }

    pmm::free(reinterpret_cast<void*>(ALIGN_DOWN(base - hhdm_offset, 0x1000)), ALIGN_UP(initrd_mod->size, 0x1000) / 0x1000);

    serial::newline();
    initialised = true;
}
}