#include <drivers/display/terminal/terminal.hpp>
#include <drivers/display/serial/serial.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <system/mm/heap/heap.hpp>
#include <lib/string.hpp>

bool initrd_initialised = false;
uint64_t ustar_filecount;
ustar_header_t *ustar_headers;

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

int ustar_parse(unsigned int address)
{
    unsigned int i;
    for (i = 0; ; i++)
    {
        ustar_file_header_t *header = (ustar_file_header_t*)address;

        if (strcmp(header->signature, "ustar"))
        {
            break;
        }

        if (ustar_filecount >= (alloc_getsize(ustar_headers) / sizeof(ustar_header_t)))
        {
            allocated += 5;
            ustar_headers = (ustar_header_t*)realloc(ustar_headers, allocated * sizeof(ustar_header_t));
        }

        uintptr_t size = getsize(header->size);
        ustar_headers[i].header = header;
        ustar_headers[i].address = address + 512;
        ustar_filecount++;

        address += (((size + 511) / 512) + 1) * 512;
    }
    for (uint64_t g = 1; g < ustar_filecount; g++)
    {
        memmove(ustar_headers[g].header->name, ustar_headers[g].header->name + 1, strlen(ustar_headers[g].header->name));
    }
    ustar_filecount--;
    i--;
    return i;
}

bool check_initrd()
{
    if (!initrd_initialised)
    {
        printf("\033[31mInitrd has not been initialised!%s\n", term_colour);
        return false;
    }
    return true;
}

void ustar_list()
{
    if (!check_initrd()) return;

    int size = 0;
    printf("Total %ld items:\n--------------------\n", ustar_filecount);
    for (uint64_t i = 1; i < ustar_filecount + 1; i++)
    {
        switch (ustar_headers[i].header->typeflag[0])
        {
            case REGULAR_FILE:
                printf("%ld) (REGULAR) %s %s\n", i, ustar_headers[i].header->name, humanify(oct_to_dec(string_to_int(ustar_headers[i].header->size))));
                size += oct_to_dec(string_to_int(ustar_headers[i].header->size));
                break;
            case SYMLINK:
                printf("%ld) \033[96m(SYMLINK) %s --> %s%s\n", i, ustar_headers[i].header->name, ustar_headers[i].header->link, term_colour);
                break;
            case DIRECTORY:
                printf("%ld) \033[35m(DIRECTORY) %s%s\n", i, ustar_headers[i].header->name, term_colour);
                break;
            default:
                printf("%ld) \033[31m(File type not supported!) %s%s\n", i, ustar_headers[i].header->name, term_colour);
                break;
        }
    }
    printf("--------------------\nTotal size: %s\n", humanify(size));
}

char *ustar_cat(char *name)
{
    if (!check_initrd()) return "";

    char *contents;
    int i = 0;
    i = ustar_getid(name);
    switch (ustar_headers[i].header->typeflag[0])
    {
        case ustar_filetypes::REGULAR_FILE:
            if (ustar_search(name, &contents) != 0)
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
    printf("\033[31mInvalid file name!%s\n", term_colour);
    return "Invalid file name!";
}

int ustar_getid(char *name)
{
    if (!check_initrd()) return 0;

    for (uint64_t i = 0; i < ustar_filecount; ++i)
    {
        if(!strcmp(ustar_headers[i].header->name, name))
        {
            return i;
        }
    }
    return 0;
}

int ustar_search(char *filename, char **contents)
{
    if (!check_initrd()) return 0;

    for (uint64_t i = 1; i < ustar_filecount + 1; i++)
    {
        if (!strcmp(ustar_headers[i].header->name, filename))
        {
            *contents = (char*)ustar_headers[i].address;
            return 1;
        }
    }
    return 0;
}

void initrd_init(unsigned int address)
{
    serial_info("Initialising initrd\n");

    if (initrd_initialised)
    {
        serial_info("Initrd has already been initialised!\n");
        return;
    }

    if (!heap_initialised)
    {
        serial_info("Heap has not been initialised!\n");
        Heap_init();
    }

    ustar_headers = (ustar_header_t*)malloc(allocated * sizeof(ustar_header_t));

    ustar_parse(address);
    
    initrd_initialised = true;

    serial_newline();
}