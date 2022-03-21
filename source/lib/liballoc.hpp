#pragma once

#include <cstdint>
#include <cstddef>

struct boundary_tag
{
    uint64_t magic;
    uint64_t size;
    uint64_t real_size;
    int index;

    struct boundary_tag *split_left;
    struct boundary_tag *split_right;

    struct boundary_tag *next;
    struct boundary_tag *prev;
};

void *liballoc_malloc(size_t);
void *liballoc_realloc(void*, size_t);
void *liballoc_calloc(size_t, size_t);
void liballoc_free(void*);
size_t liballoc_allocsize(void*);