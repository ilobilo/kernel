#include <lib/alloc.hpp>

BuddyAlloc buddyheap;
SlabAlloc slabheap;

void *operator new(size_t size)
{
    return malloc(size);
}

void *operator new[](size_t size)
{
    return malloc(size);
}

void operator delete(void *ptr)
{
    free(ptr);
}

void operator delete[](void *ptr)
{
    free(ptr);
}