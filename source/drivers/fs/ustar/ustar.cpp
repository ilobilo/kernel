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
    unsigned int i;
    for (i = 0; ; i++)
    {
        file_header_t *header = (file_header_t*)address;

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

        address += (((size + 511) / 512) + 1) * 512;
    }
    for (uint64_t g = 1; g < filecount; g++)
    {
        memmove(headers[g].header->name, headers[g].header->name + 1, strlen(headers[g].header->name));
    }
    filecount--;
    i--;
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
    for (uint64_t i = 1; i < filecount + 1; i++)
    {
        switch (headers[i].header->typeflag[0])
        {
            case REGULAR_FILE:
                printf("%ld) (REGULAR) %s %s\n", i, headers[i].header->name, humanify(oct_to_dec(string_to_int(headers[i].header->size))));
                size += oct_to_dec(string_to_int(headers[i].header->size));
                break;
            case SYMLINK:
                printf("%ld) \033[96m(SYMLINK) %s --> %s%s\n", i, headers[i].header->name, headers[i].header->link, terminal::colour);
                break;
            case DIRECTORY:
                printf("%ld) \033[35m(DIRECTORY) %s%s\n", i, headers[i].header->name, terminal::colour);
                break;
            default:
                printf("%ld) \033[31m(File type not supported!) %s%s\n", i, headers[i].header->name, terminal::colour);
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
            if (search(name, &contents) != 0)
            {
                printf("--BEGIN-- %s\n", name);
                printf("%s", contents);
                printf("--END-- %s\n", name);
            }
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

    for (uint64_t i = 1; i < filecount + 1; i++)
    {
        if (!strcmp(headers[i].header->name, filename))
        {
            *contents = (char*)headers[i].address;
            return 1;
        }
    }
    return 0;
}

void init(unsigned int address)
{
    serial::info("Initialising USTAR filesystem");

    if (initialised)
    {
        serial::info("USTAR filesystem has already been initialised!\n");
        return;
    }

    headers = (header_t*)heap::malloc(allocated * sizeof(header_t));

    parse(address);

    serial::newline();
    initialised = true;
}
}