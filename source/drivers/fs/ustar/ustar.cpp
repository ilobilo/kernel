#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/mm/heap/heap.hpp>
#include <lib/string.hpp>
#include <lib/memory.hpp>

using namespace kernel::drivers::display;
using namespace kernel::system::mm;

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
    address += (((getsize(((file_header_t*)address)->size) + 511) / 512) + 1) * 512;
    unsigned int i;
    for (i = 0; ; i++)
    {
        file_header_t *header = (file_header_t*)address;
        memmove(header->name, header->name + 1, strlen(header->name));

        if (strcmp(header->signature, "ustar")) break;

        if (filecount >= (heap::getsize(headers) / sizeof(header_t)))
        {
            allocated += 5;
            headers = (header_t*)heap::realloc(headers, allocated * sizeof(header_t));
        }

        uintptr_t size = getsize(header->size);
        if (header->name[strlen(header->name) - 1] == '/') header->name[strlen(header->name) - 1] = 0;
        headers[i].size = size;
        headers[i].header = header;
        headers[i].address = address + 512;
        filecount++;

        vfs::fs_node_t *node = vfs::create(NULL, headers[i].header->name);

        node->address = headers[i].address;
        node->length = size;
        node->gid = getsize(header->gid);
        node->uid = getsize(header->uid);

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

void list()
{
    if (!check()) return;

    int size = 0;
    printf("Total %ld items:\n--------------------\n", filecount);
    for (uint64_t i = 0; i < filecount; i++)
    {
        switch (headers[i].header->typeflag[0])
        {
            case REGULAR_FILE:
                printf("%ld) (REGULAR) %s %s\n", i + 1, headers[i].header->name, humanify(headers[i].size));
                size += oct_to_dec(string_to_int(headers[i].header->size));
                break;
            case SYMLINK:
                printf("%ld) \033[96m(SYMLINK) %s --> %s%s\n", i + 1, headers[i].header->name, headers[i].header->link, terminal::colour);
                break;
            case DIRECTORY:
                printf("%ld) \033[35m(DIRECTORY) %s%s\n", i + 1, headers[i].header->name, terminal::colour);
                break;
            default:
                printf("%ld) \033[31m(File type not supported!) %s%s\n", i + 1, headers[i].header->name, terminal::colour);
                break;
        }
    }
    printf("--------------------\nTotal size: %s\n", humanify(size));
}

char *cat(char *name)
{
    if (!check()) return "";

    char *contents;
    int i = 0;
    i = getid(name);
    switch (headers[i].header->typeflag[0])
    {
        case filetypes::REGULAR_FILE:
            if (search(name, &contents) != 0) printf("%s", contents);
            else goto Error;
            break;
        default:
            goto Error;
            break;
    }
    return contents;
    Error:
    printf("\033[31mInvalid file name!%s\n", terminal::colour);
    return "Invalid file name!";
}

int getid(const char *name)
{
    if (!check()) return 0;

    for (uint64_t i = 0; i < filecount; ++i)
    {
        if(!strcmp(headers[i].header->name, name)) return i;
    }
    return 0;
}

int search(char *filename, char **contents)
{
    if (!check()) return 0;

    for (uint64_t i = 0; i < filecount; i++)
    {
        if (!strcmp(headers[i].header->name, filename))
        {
            *contents = (char*)headers[i].address;
            return 1;
        }
    }
    return 0;
}

size_t ustar_read(vfs::fs_node_t *node, size_t offset, size_t size, char *buffer)
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
    serial::info("Initialising USTAR filesystem");

    if (initialised)
    {
        serial::info("USTAR filesystem has already been initialised!\n");
        return;
    }

    headers = (header_t*)heap::malloc(allocated * sizeof(header_t));

    initrd_root = vfs::mount_root(&ustar_fs);
    initrd_root->children.init();
    parse(address);

    serial::newline();
    initialised = true;
}
}