#include <drivers/fs/tar/tar.hpp>
#include <drivers/terminal/terminal.hpp>
#include <include/string.hpp>

struct tar_header *headers[32];
int count;
unsigned int addrs[32];
char* names[32];

unsigned int getsize(const char *in)
{
    unsigned int size = 0;
    unsigned int j;
    unsigned int count = 1;
    for (j = 11; j > 0; j--, count *= 8)
    {
        size += ((in[j - 1] - '0') * count);
    }
    return size;
}

void tar_parse(unsigned int address)
{
    unsigned int i;
    for (i = 0; ; i++)
    {
        struct tar_header *header = (struct tar_header *)address;
        if (header->name[0] == '\0')
        {
            break;
        }

        unsigned int size = getsize(header->size);

        headers[i] = header;
        names[i] = header->name;
        address += ((size / 512) + 1) * 512;

        if (size % 512)
        {
            address += 512;
        }
    }
    count = i;
}

void tar_getaddrs(unsigned int address)
{
    for (int i = 0; i < count; i++)
    {
        addrs[i] = address;
        address += ((getsize(headers[i]->size) / 512) + 1) * 512 + 512;
    }
}

void tar_list()
{
    int size = 0;
    printf("Total %d items:\n", count);
    for (int i = 0; i < count; i++)
    {
        printf("%d) %s %dB\n", i + 1, names[i], oct_to_dec(string_to_int(headers[i]->size)));
        size += oct_to_dec(string_to_int(headers[i]->size));
    }
    printf("Total size: %dB\n", size);
}

void tar_cat(char* name)
{
    int len = sizeof(names) / sizeof(names[0]);
    for (int i = 0; i < len; ++i)
    {
        if(!strcmp(names[i], name))
        {
            printf("--BEGIN-- %s\n", name);
            printf("%s", (char *)addrs[i]);
            printf("--END-- %s\n", name);
            return;
        }
    }
    printf("\033[31mInvalid Filename!\n");
    term_resetcolour();
}

int tar_getnum(char* name)
{
    int len = sizeof(names) / sizeof(names[0]);
    for (int i = 0; i < len; ++i)
    {
        if(!strcmp(names[i], name))
        {
            return i;
        }
    }
    printf("\033[31mInvalid Filename!\n");
    term_resetcolour();
}

void tar_init(unsigned int address)
{
    tar_parse(address);
    tar_getaddrs(address);
}
