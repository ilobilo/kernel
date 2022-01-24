#include <lib/alloc.hpp>

#if (ALLOC_IMPL == BUDDY)
BuddyAlloc buddyheap;
#elif (ALLOC_IMPL == SLAB)
SlabAlloc slabheap;
#endif

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