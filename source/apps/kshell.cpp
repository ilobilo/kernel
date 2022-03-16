// Copyright (C) 2021-2022  ilobilo

#include <drivers/display/terminal/terminal.hpp>
#include <system/sched/scheduler/scheduler.hpp>
#include <system/sched/timer/timer.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/sched/rtc/rtc.hpp>
#include <system/sched/pit/pit.hpp>
#include <system/mm/pmm/pmm.hpp>
#include <system/acpi/acpi.hpp>
#include <drivers/ps2/ps2.hpp>
#include <system/pci/pci.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>
#include <lib/alloc.hpp>
#include <lib/cwalk.hpp>
#include <lib/log.hpp>
#include <lib/io.hpp>

using namespace kernel::drivers::display;
using namespace kernel::drivers;
using namespace kernel::system::sched;
using namespace kernel::system::mm;
using namespace kernel::system;

namespace kernel::apps::kshell {

vfs::fs_node_t *current_path = nullptr;

void parse(string cmd, string arg)
{
    switch (hash(cmd.c_str()))
    {
        case hash("help"):
            printf("- help -- This\n");
            printf("- clear -- Clear terminal\n");
            printf("- ls -- List files\n");
            printf("- cat -- Print file contents\n");
            printf("- cd -- Change directory\n");
            printf("- exec -- Execute binary\n");
            printf("- free -- Get memory info\n");
            printf("- time -- Get current RTC time\n");
            printf("- timef -- Get current RTC time (Forever loop)\n");
            printf("- tick -- Get current PIT tick\n");
            printf("- pci -- List PCI devices\n");
            printf("- crash -- Crash whole system\n");
            printf("- reboot -- Reboot the system\n");
            printf("- poweroff -- Shutdown the system\n");
            printf("- shutdown -- Shutdown the system\n");
            printf("You can execute any file in /bin with just typing it's name or with \"exec <filename>\"\n");
            break;
        case hash("clear"):
            terminal::clear();
            break;
        case hash("ls"):
        {
            vfs::fs_node_t *node = vfs::get_node(current_path, arg);
            if (node == nullptr)
            {
                printf("\033[31mNo such file or directory!%s\n", terminal::colour);
                break;
            }
            if (!vfs::isdir(node->res->stat.mode))
            {
                printf("%s\n", arg.c_str());
                break;
            }

            for (vfs::fs_node_t *child : node->children)
            {
                if (child->name != "." && child->name != ".." && vfs::isdir(child->res->stat.mode))
                {
                    printf("\033[35m%s%s ", child->name.c_str(), terminal::colour);
                }
            }
            for (vfs::fs_node_t *child : node->children)
            {
                if (child->name != "." && child->name != ".." &&vfs::ischr(child->res->stat.mode))
                {
                    printf("\033[93m%s%s ", child->name.c_str(), terminal::colour);
                }
            }
            for (vfs::fs_node_t *child : node->children)
            {
                if (child->name != "." && child->name != ".." && vfs::islnk(child->res->stat.mode))
                {
                    printf("\033[96m%s%s ", child->name.c_str(), terminal::colour);
                }
            }
            for (vfs::fs_node_t *child : node->children)
            {
                if (child->name != "." && child->name != ".." && !vfs::isdir(child->res->stat.mode) && !vfs::ischr(child->res->stat.mode) && !vfs::islnk(child->res->stat.mode))
                {
                    printf("%s ", child->name.c_str());
                }
            }
            printf("\n");
            break;
        }
        case hash("cat"):
        {
            vfs::fs_node_t *node = vfs::get_node(current_path, arg);
            if (node == nullptr)
            {
                printf("\033[31mNo such file or directory!%s\n", terminal::colour);
                break;
            }
            if (vfs::isreg(node->res->stat.mode))
            {
                size_t size = node->res->stat.size;
                char *buffer = new char[size];
                node->res->read(nullptr, reinterpret_cast<uint8_t*>(buffer), 0, size);
                printf("%s%c", buffer, buffer[size - 1] == '\n' ? 0 : '\n');
                delete[] buffer;
            }
            else printf("\033[31m%s is not a text file!%s\n", arg.c_str(), terminal::colour);
            break;
        }
        case hash("cd"):
        {
            if (arg.empty()) arg = "/";
            vfs::fs_node_t *node = vfs::get_node(current_path, arg);                                        if (node == nullptr)
            {
                printf("\033[31mNo such file or directory!%s\n", terminal::colour);
                break;
            }
            if (!vfs::isdir(node->res->stat.mode))
            {
                printf("\033[31m%s is not a directory!%s\n", arg.c_str(), terminal::colour);
                break;
            }
            current_path = node;
            break;
        }
        case hash("exec"):
        {
            if (arg.empty())
            {
                printf("exec <filename>\n");
                break;
            }
            vfs::fs_node_t *node = vfs::get_node(current_path, arg);
            if (node == nullptr)
            {
                printf("\033[31mNo such file or directory!%s\n", terminal::colour);
                break;
            }
            if (!vfs::isreg(node->res->stat.mode))
            {
                printf("\033[31m%s is not a regular file!%s\n", arg.c_str(), terminal::colour);
                break;
            }
            size_t size = node->res->stat.size;
            uint8_t *buffer = new uint8_t[size];
            node->res->read(nullptr, buffer, 0, size);
            reinterpret_cast<int (*)()>(buffer)();
            delete[] buffer;
            break;
        }
        case hash("free"):
        {
            uint64_t free = pmm::freemem() / 1024;
            uint64_t used = pmm::usedmem() / 1024;
            uint64_t all = free + used;
            printf("Usable memory: %ld KB\nFree memory: %ld KB\nUsed memory: %ld KB\n", all, free, used);
            break;
        }
        case hash("time"):
            printf("%s\n", rtc::getTime());
            break;
        case hash("tick"):
            printf("%ld\n", pit::get_tick());
            break;
        case hash("timef"):
            while (true)
            {
                printf("\r\033[2K%s", rtc::getTime());
                timer::sleep(1);
            }
            break;
        case hash("pci"):
            for (size_t i = 0; i < pci::devices.size(); i++)
            {
                printf("%.4X:%.4X %s %s\n",
                    pci::devices[i]->device->vendorid,
                    pci::devices[i]->device->deviceid,
                    pci::devices[i]->vendorstr,
                    pci::devices[i]->devicestr);
            }
            break;
        case hash("crash"):
        {
            vfs::fs_node_t *node = vfs::get_node(current_path, "/bin/crash");
            if (node == nullptr)
            {
                printf("\033[31mNo such file or directory!%s\n", terminal::colour);
                break;
            }
            if (!vfs::isreg(node->res->stat.mode))
            {
                printf("\033[31m%s is not a regular file!%s\n", arg.c_str(), terminal::colour);
                break;
            }
            size_t size = node->res->stat.size;
            uint8_t *buffer = new uint8_t[size];
            node->res->read(nullptr, buffer, 0, size);
            reinterpret_cast<int (*)()>(buffer)();
            delete[] buffer;
            break;
        }
        case hash("shutdown"):
        case hash("poweroff"):
            acpi::shutdown();
            timer::sleep(1);
            outw(0xB004, 0x2000);
            outw(0x604, 0x2000);
            outw(0x4004, 0x3400);
            printf("\033[31mCould not shutdown!\033[0m\n");
            break;
        case hash("reboot"):
            acpi::reboot();
            printf("\033[31mCould not reboot!\033[0m\n");
            break;
        default:
        {
            printf("\033[31mCommand not found!\033[0m\n");
            break;
        }
    }
}

void run()
{
    log("Starting kernel shell\n");
    while (true)
    {
        if (!current_path)
        {
            current_path = scheduler::this_proc()->current_dir;
        }
        printf("\033[32mroot@kernel\033[0m:\033[95m%s%s%s# ", (current_path->name.first() != '/') ? "/" : "", current_path->name.c_str(), terminal::colour);
        string command(ps2::getline());

        command.remove_leading();
        command.remove_middle();
        command.remove_trailing();

        string arg(command.c_str());
        arg.erase(0, command.find(" ") + 1);
        string cmd(command.substr(0, command.find(" ")));

        parse(cmd, arg);
    }
}
}