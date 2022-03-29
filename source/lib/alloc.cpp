#include <lib/alloc.hpp>
#include <lib/math.hpp>

BuddyAlloc buddyheap;
SlabAlloc slabheap;

void *malloc(size_t size, bool calloc)
{
    switch (defalloc)
    {
        case LIBALLOC:
            if (calloc) return liballoc_calloc(1, size);
            return liballoc_malloc(size);
            break;
        case BUDDY:
            if (calloc) return buddyheap.calloc(1, size);
            return buddyheap.malloc(size);
            break;
        case SLAB:
            if (calloc) return slabheap.calloc(1, size);
            return slabheap.malloc(size);
            break;
    }
    panic("No default allocator!");
    return nullptr;
}

void *calloc(size_t num, size_t size)
{
    switch (defalloc)
    {
        case LIBALLOC:
            return liballoc_calloc(num, size);
            break;
        case BUDDY:
            return buddyheap.calloc(num, size);
            break;
        case SLAB:
            return slabheap.calloc(num, size);
            break;
    }
    panic("No default allocator!");
    return nullptr;
}

void *realloc(void *ptr, size_t size)
{
    switch (defalloc)
    {
        case LIBALLOC:
            return liballoc_realloc(ptr, size);
            break;
        case BUDDY:
            return buddyheap.realloc(ptr, size);
            break;
        case SLAB:
            return slabheap.realloc(ptr, size);
            break;
    }
    panic("No default allocator!");
    return nullptr;
}

void free(void *ptr)
{
    switch (defalloc)
    {
        case LIBALLOC:
            return liballoc_free(ptr);
            break;
        case BUDDY:
            return buddyheap.free(ptr);
            break;
        case SLAB:
            return slabheap.free(ptr);
            break;
    }
    panic("No default allocator!");
    return;
}
size_t allocsize(void *ptr)
{
    switch (defalloc)
    {
        case LIBALLOC:
            return liballoc_allocsize(ptr);
            break;
        case BUDDY:
            return buddyheap.allocsize(ptr);
            break;
        case SLAB:
            return slabheap.allocsize(ptr);
            break;
    }
    panic("No default allocator!");
    return 0;
}


namespace std
{
    enum class align_val_t: size_t {};
}

void *operator new(size_t size)
{
    return malloc(size);
}

void *operator new(size_t size, std::align_val_t alignment)
{
    return malloc(ALIGN_UP(size, static_cast<size_t>(alignment)));
}

void *operator new[](size_t size)
{
    return malloc(size);
}

void *operator new[](size_t size, std::align_val_t alignment)
{
    return malloc(ALIGN_UP(size, static_cast<size_t>(alignment)));
}

void operator delete(void *ptr)
{
    free(ptr);
}

void operator delete(void *ptr, std::align_val_t alignment)
{
    free(ptr);
}

void operator delete[](void *ptr)
{
    free(ptr);
}

void operator delete[](void *ptr, std::align_val_t alignment)
{
    free(ptr);
}