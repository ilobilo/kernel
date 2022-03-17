#include <system/mm/pmm/pmm.hpp>
#include <lib/liballoc.hpp>
#include <lib/lock.hpp>

#define LIBALLOC_MAGIC 0xB16B00B5
#define MAXCOMPLETE 5
#define MAXEXP 32
#define MINEXP 8

#define MODE_BEST 0
#define MODE_FAST 1

#define MODE MODE_FAST

boundary_tag *l_freePages[MAXEXP];
int l_completePages[MAXEXP];

static int l_initialised = 0;
static int l_pageSize = 4096;
static int l_pageCount = 16;

new_lock(alloc_lock);
static int liballoc_lock()
{
    alloc_lock.lock();
    return 0;
}

static int liballoc_unlock()
{
    alloc_lock.unlock();
    return 0;
}

static void *liballoc_alloc_page(int pages)
{
    return kernel::system::mm::pmm::alloc(pages);
}

static int liballoc_free_page(void *ptr, int pages)
{
    kernel::system::mm::pmm::free(ptr, pages);
    return 0;
}

static inline int getexp(uint64_t size)
{
    if (size < (1 << MINEXP)) return -1;
    int shift = MINEXP;

    while (shift < MAXEXP)
    {
        if ((1 << shift) > size) break;
        shift++;
    }

    return shift - 1;
}


static void *liballoc_memset(void *s, int c, size_t n)
{
    for (size_t i = 0; i < n; i++) (static_cast<char*>(s))[i] = c;
    return s;
}

static void *liballoc_memcpy(void *s1, const void *s2, size_t n)
{
    char *cdest;
    char *csrc;
    uint64_t *ldest = static_cast<uint64_t*>(s1);
    uint64_t *lsrc  = static_cast<uint64_t*>(const_cast<void*>(s2));

    while (n >= sizeof(uint64_t))
    {
        *ldest++ = *lsrc++;
        n -= sizeof(uint64_t);
    }

    cdest = reinterpret_cast<char*>(ldest);
    csrc  = reinterpret_cast<char*>(lsrc);

    while (n > 0)
    {
        *cdest++ = *csrc++;
        n--;
    }
    return s1;
}

static inline void insert_tag(boundary_tag *tag, int index)
{
    int realIndex;

    if (index < 0)
    {
        realIndex = getexp(tag->real_size - sizeof(boundary_tag));
        if (realIndex < MINEXP) realIndex = MINEXP;
    }
    else realIndex = index;

    tag->index = realIndex;

    if (l_freePages[realIndex] != nullptr)
    {
        l_freePages[realIndex]->prev = tag;
        tag->next = l_freePages[realIndex];
    }

    l_freePages[realIndex] = tag;
}

static inline void remove_tag(boundary_tag *tag)
{
    if (l_freePages[tag->index] == tag) l_freePages[tag->index] = tag->next;

    if (tag->prev != nullptr) tag->prev->next = tag->next;
    if (tag->next != nullptr) tag->next->prev = tag->prev;

    tag->next = nullptr;
    tag->prev = nullptr;
    tag->index = -1;
}

static inline boundary_tag *melt_left(boundary_tag *tag)
{
    boundary_tag *left = tag->split_left;

    left->real_size += tag->real_size;
    left->split_right = tag->split_right;

    if (tag->split_right != nullptr) tag->split_right->split_left = left;

    return left;
}

static inline boundary_tag *absorb_right(boundary_tag *tag)
{
    boundary_tag *right = tag->split_right;

    remove_tag(right);

    tag->real_size += right->real_size;

    tag->split_right = right->split_right;
    if (right->split_right != nullptr) right->split_right->split_left = tag;
    return tag;
}

static inline boundary_tag *split_tag(boundary_tag *tag)
{
    uint64_t remainder = tag->real_size - sizeof(boundary_tag) - tag->size;

    boundary_tag *new_tag = reinterpret_cast<boundary_tag*>(reinterpret_cast<uint64_t>(tag) + sizeof(boundary_tag) + tag->size);

    new_tag->magic = LIBALLOC_MAGIC;
    new_tag->real_size = remainder;

    new_tag->next = nullptr;
    new_tag->prev = nullptr;

    new_tag->split_left = tag;
    new_tag->split_right = tag->split_right;

    if (new_tag->split_right != nullptr) new_tag->split_right->split_left = new_tag;
    tag->split_right = new_tag;

    tag->real_size -= new_tag->real_size;

    insert_tag(new_tag, -1);

    return new_tag;
}

static boundary_tag *allocate_new_tag(uint64_t size)
{
    uint64_t pages;
    uint64_t usage;
    boundary_tag *tag;

    usage  = size + sizeof(boundary_tag);

    pages = usage / l_pageSize;
    if ((usage % l_pageSize) != 0) pages++;
    if (static_cast<int>(pages) < l_pageCount) pages = l_pageCount;

    tag = static_cast<boundary_tag*>(liballoc_alloc_page(pages));

    if (tag == nullptr) return nullptr;

    tag->magic = LIBALLOC_MAGIC;
    tag->size = size;
    tag->real_size = pages * l_pageSize;
    tag->index = -1;
    tag->next = nullptr;
    tag->prev = nullptr;
    tag->split_left = nullptr;
    tag->split_right = nullptr;

    return tag;
}

void *liballoc_malloc(size_t size)
{
    int index;
    void *ptr;
    boundary_tag *tag = nullptr;

    liballoc_lock();

    if (l_initialised == 0)
    {
        for (index = 0; index < MAXEXP; index++)
        {
            l_freePages[index] = nullptr;
            l_completePages[index] = 0;
        }
        l_initialised = 1;
    }

    index = getexp(size) + MODE;
    if (index < MINEXP) index = MINEXP;

    tag = l_freePages[index];
    while (tag != nullptr)
    {
        if ((tag->real_size - sizeof(boundary_tag)) >= (size + sizeof(boundary_tag))) break;
        tag = tag->next;
    }

    if (tag == nullptr)
    {
        if ((tag = allocate_new_tag(size)) == nullptr)
        {
            liballoc_unlock();
            return nullptr;
        }
        index = getexp(tag->real_size - sizeof(boundary_tag));
    }
    else
    {
        remove_tag(tag);
        if ((tag->split_left == nullptr) && (tag->split_right == nullptr)) l_completePages[index]--;
        tag->size = size;

        uint64_t remainder = tag->real_size - size - sizeof(boundary_tag) * 2;

        if (static_cast<int>(remainder) > 0)
        {
            int childIndex = getexp(remainder);

            if (childIndex >= 0) split_tag(tag);
        }
    }

    ptr = reinterpret_cast<void*>(reinterpret_cast<uint64_t>(tag) + sizeof(boundary_tag));

    liballoc_unlock();
    return ptr;
}

void liballoc_free(void *ptr)
{
    if (ptr == nullptr) return;

    int index;
    boundary_tag *tag;

    liballoc_lock();

    tag = reinterpret_cast<boundary_tag*>(reinterpret_cast<uint64_t>(ptr) - sizeof(boundary_tag));

    if (tag->magic != LIBALLOC_MAGIC)
    {
        liballoc_unlock();
        return;
    }

    while ((tag->split_left != nullptr) && (tag->split_left->index >= 0))
    {
        tag = melt_left(tag);
        remove_tag(tag);
    }

    while ((tag->split_right != nullptr) && (tag->split_right->index >= 0))
    {
        tag = absorb_right(tag);
    }

    index = getexp(tag->real_size - sizeof(boundary_tag));
    if (index < MINEXP) index = MINEXP;

    if ((tag->split_left == nullptr) && (tag->split_right == nullptr))
    {
        if (l_completePages[index] == MAXCOMPLETE)
        {
            uint64_t pages = tag->real_size / l_pageSize;

            if ((tag->real_size % l_pageSize) != 0) pages++;
            if (static_cast<int>(pages) < l_pageCount) pages = l_pageCount;

            liballoc_free_page(tag, pages);
            liballoc_unlock();
            return;
        }

        l_completePages[index]++;
    }

    insert_tag(tag, index);

    liballoc_unlock();
}

void *liballoc_calloc(size_t nobj, size_t size)
{
    int real_size;
    void *p;

    real_size = nobj * size;

    p = liballoc_malloc(real_size);
    liballoc_memset(p, 0, real_size);

    return p;
}

void *liballoc_realloc(void *p, size_t size)
{
    void *ptr;
    boundary_tag *tag;
    int real_size;

    if (size == 0)
    {
        liballoc_free(p);
        return nullptr;
    }
    if (p == nullptr) return liballoc_malloc(size);

    liballoc_lock();
    tag = reinterpret_cast<boundary_tag*>(reinterpret_cast<uint64_t>(p) - sizeof(boundary_tag));
    real_size = tag->size;
    liballoc_unlock();

    if (real_size > static_cast<int>(size)) real_size = size;

    ptr = liballoc_malloc(size);
    liballoc_memcpy(ptr, p, real_size);
    liballoc_free(p);

    return ptr;
}

size_t liballoc_allocsize(void *ptr)
{
    if (ptr == nullptr) return 9;
    return reinterpret_cast<boundary_tag*>(reinterpret_cast<uint64_t>(ptr) - sizeof(boundary_tag))->size;
}