#include <drivers/display/serial/serial.hpp>
#include <drivers/display/terminal/terminal.hpp>
#include <drivers/fs/ustar/ustar.hpp>
#include <include/string.hpp>

bool initrd = false;

ustar_header_t *ustar_headers;

unsigned int getsize(const char *s)
{
    uint64_t ret = 0;
    while (*s) {
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
        struct ustar_file_header_t *header = (ustar_file_header_t*)address;

        if (strcmp(header->signature, "ustar"))
        {
            break;
        }

        uintptr_t size = getsize(header->size);
        ustar_headers->headers[i] = header;
        ustar_headers->address[i] = address + 512;
        ustar_headers->count++;

        address += (((size + 511) / 512) + 1)/ * 512;
    }
    for (int g = 1; g < ustar_headers->count; g++)
    {
        memmove(ustar_headers->headers[g]->name, ustar_headers->headers[g]->name + 1, strlen(ustar_headers->headers[g]->name));
    }
    ustar_headers->count--;
    i--;
    return i;
}

void not_initialized()
{
    printf("\033[31mInitrd has not been initialized!\033[0m\n");
}

void ustar_list()
{
    if (!initrd)
    {
        not_initialized();
        return;
    }
    int size = 0;
    printf("Total %d items:\n--------------------\n", ustar_headers->count);
    for (int i = 1; i < ustar_headers->count + 1; i++)
    {
        switch (ustar_headers->headers[i]->typeflag[0])
        {
            case REGULAR_FILE:
                printf("%d) (REGULAR) %s %s\n", i, ustar_headers->headers[i]->name, humanify(oct_to_dec(string_to_int(ustar_headers->headers[i]->size))));
                size += oct_to_dec(string_to_int(ustar_headers->headers[i]->size));
                break;
            case SYMLINK:
                printf("%d) \033[96m(SYMLINK) %s --> %s\033[0m\n", i, ustar_headers->headers[i]->name, ustar_headers->headers[i]->link);
                break;
            case DIRECTORY:
                printf("%d) \033[35m(DIRECTORY) %s\033[0m\n", i, ustar_headers->headers[i]->name);
                break;
            default:
                printf("%d) \033[31m(File type not supported!) %s\033[0m\n", i, ustar_headers->headers[i]->name);
                break;
        }
    }
    printf("--------------------\nTotal size: %s\n", humanify(size));
}

char *ustar_cat(char *name)
{
    if (!initrd)
    {
        not_initialized();
        return 0;
    }
    char *contents;
    int i = 0;
    i = ustar_getid(name);
    switch (ustar_headers->headers[i]->typeflag[0])
    {
        case REGULAR_FILE:
            if (ustar_search(name, &contents) != 0)
            {
                printf("--BEGIN-- %s\n", name);
                printf("%s", contents);
                printf("--END-- %s\n", name);
            }
            else
            {
                goto Error;
            }
            break;
        default:
            contents = "";
            printf("\033[31m\"%s\" is not a regular file!\033[0m\n", name);
            break;
    }
    return contents;
    Error:
    printf("Invalid file name!");
    return "Invalid file name!";
}

int ustar_getid(char *name)
{
    if (!initrd)
    {
        not_initialized();
        return 0;
    }
    for (int i = 0; i < ustar_headers->count; ++i)
    {
        if(!strcmp(ustar_headers->headers[i]->name, name))
        {
            return i;
        }
    }
    return 0;
}

int ustar_search(char *filename, char* *contents)
{
    if (!initrd)
    {
        not_initialized();
        return 0;
    }
    for (int i = 1; i < ustar_headers->count + 1; i++)
    {
        if (!strcmp(ustar_headers->headers[i]->name, filename))
        {
          / * contents = (char*)ustar_headers->address[i];
            return 1;
        }
    }
    return 0;
}

void initrd_init(unsigned int address)
{
    serial_info("Initializing initrd");

    ustar_parse(address);
    initrd = true;

    serial_info("Initialized initrd\n");
}
