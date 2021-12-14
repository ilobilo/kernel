#include <lib/liballoc.hpp>

#define LIBALLOC_MAGIC 0xC001C0DE
#define MAXCOMPLETE 5
#define MAXEXP 32
#define MINEXP 8	

#define MODE_BEST 0
#define MODE_INSTANT 1

#define MODE MODE_BEST

boundary_tag *l_freePages[MAXEXP];
int l_completePages[MAXEXP];

static int l_initialized = 0;
static int l_pageSize = 0x1000;
static int l_pageCount = 16;

static inline int getexp(unsigned int size)
{
	if (size < (1 << MINEXP)) return -1;

	int shift = MINEXP;

	while (shift < MAXEXP)
	{
		if ((1 << shift) > size) break;
		shift += 1;
	}

	return shift - 1;	
}

static void* liballoc_memset(void* s, int c, size_t n)
{
	for (size_t i = 0; i < n; i++) ((char*)s)[i] = c;
	return s;
}

static void* liballoc_memcpy(void* s1, const void* s2, size_t n)
{
	char *cdest;
	char *csrc;
	unsigned int *ldest = (unsigned int*)s1;
	unsigned int *lsrc  = (unsigned int*)s2;

	while (n >= sizeof(unsigned int))
	{
		*ldest++ = *lsrc++;
		n -= sizeof(unsigned int);
	}

	cdest = (char*)ldest;
	csrc  = (char*)lsrc;

	while (n > 0)
	{
		*cdest++ = *csrc++;
		n -= 1;
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
	
	if (l_freePages[realIndex] != NULL)
	{
		l_freePages[realIndex]->prev = tag;
		tag->next = l_freePages[realIndex];
	}

	l_freePages[realIndex] = tag;
}

static inline void remove_tag(boundary_tag *tag)
{
	if (l_freePages[tag->index] == tag) l_freePages[tag->index] = tag->next;

	if (tag->prev != NULL) tag->prev->next = tag->next;
	if (tag->next != NULL) tag->next->prev = tag->prev;

	tag->next = NULL;
	tag->prev = NULL;
	tag->index = -1;
}


static inline boundary_tag* melt_left(boundary_tag *tag)
{
	boundary_tag *left = tag->split_left;
							
	left->real_size += tag->real_size;
	left->split_right = tag->split_right;
	
	if (tag->split_right != NULL) tag->split_right->split_left = left;

	return left;
}


static inline boundary_tag* absorb_right(boundary_tag *tag)
{
	boundary_tag *right = tag->split_right;

		remove_tag(right);

		tag->real_size += right->real_size;

		tag->split_right = right->split_right;
		if (right->split_right != NULL) right->split_right->split_left = tag;

	return tag;
}

static inline boundary_tag* split_tag(boundary_tag* tag)
{
	unsigned int remainder = tag->real_size - sizeof(boundary_tag) - tag->size;
	boundary_tag *new_tag = (boundary_tag*)((unsigned int)((uintptr_t)tag) + sizeof(boundary_tag) + tag->size);	

	new_tag->magic = LIBALLOC_MAGIC;
	new_tag->real_size = remainder;	

	new_tag->next = NULL;
	new_tag->prev = NULL;

	new_tag->split_left = tag;
	new_tag->split_right = tag->split_right;

	if (new_tag->split_right != NULL) new_tag->split_right->split_left = new_tag;
	tag->split_right = new_tag;

	tag->real_size -= new_tag->real_size;

	insert_tag( new_tag, -1 );
	
	return new_tag;
}

static boundary_tag* allocate_new_tag(unsigned int size)
{
	unsigned int pages;
	unsigned int usage;
	boundary_tag *tag;

		// This is how much space is required.
		usage  = size + sizeof(boundary_tag);

				// Perfect amount of space
		pages = usage / l_pageSize;
		if ((usage % l_pageSize) != 0) pages += 1;

		// Make sure it's >= the minimum size.
		if (pages < l_pageCount) pages = l_pageCount;

		tag = (boundary_tag*)liballoc_alloc(pages);

		if (tag == NULL) return NULL;
		
		tag->magic = LIBALLOC_MAGIC;
		tag->size = size;
		tag->real_size = pages * l_pageSize;
		tag->index = -1;

		tag->next = NULL;
		tag->prev = NULL;
		tag->split_left = NULL;
		tag->split_right = NULL;
		
      return tag;
}

void *malloc(size_t size)
{
	int index;
	void *ptr;
	boundary_tag *tag = NULL;

	liballoc_lock();

		if (l_initialized == 0)
		{
			for (index = 0; index < MAXEXP; index++)
			{
				l_freePages[index] = NULL;
				l_completePages[index] = 0;
			}
			l_initialized = 1;
		}

		index = getexp(size) + MODE;
		if (index < MINEXP) index = MINEXP;

			tag = l_freePages[index];
			while (tag != NULL)
			{
				if ((tag->real_size - sizeof(boundary_tag)) >= (size + sizeof(boundary_tag))) break;

				tag = tag->next;
			}

			if (tag == NULL)
			{	
				if ((tag = allocate_new_tag(size)) == NULL)
				{
					liballoc_unlock();
					return NULL;
				}
				
				index = getexp(tag->real_size - sizeof(boundary_tag));
			}
			else
			{
				remove_tag(tag);

				if ((tag->split_left == NULL) && (tag->split_right == NULL))
					l_completePages[index] -= 1;
			}
		
		tag->size = size;

		unsigned int remainder = tag->real_size - size - sizeof( boundary_tag ) * 2;

		if (((int)(remainder) > 0) /*&& ( (tag->real_size - remainder) >= (1<<MINEXP))*/ )
		{
			int childIndex = getexp(remainder);
	
			if (childIndex >= 0)
			{
				boundary_tag *new_tag = split_tag(tag); 

				new_tag = new_tag;
			}	
		}

	ptr = (void*)((unsigned int)((uintptr_t)tag) + sizeof(boundary_tag));

	liballoc_unlock();
	return ptr;
}

void free(void *ptr)
{
	int index;
	boundary_tag *tag;

	if (ptr == NULL) return;

	liballoc_lock();

		tag = (boundary_tag*)((unsigned int)((uintptr_t)ptr) - sizeof(boundary_tag));
	
		if (tag->magic != LIBALLOC_MAGIC) 
		{
			liballoc_unlock();
			return;
		}

		while ((tag->split_left != NULL) && (tag->split_left->index >= 0))
		{
			tag = melt_left(tag);
			remove_tag(tag);
		}

		while ((tag->split_right != NULL) && (tag->split_right->index >= 0))
		{
			tag = absorb_right(tag);
		}

		index = getexp(tag->real_size - sizeof(boundary_tag));
		if (index < MINEXP) index = MINEXP;

		if ((tag->split_left == NULL) && (tag->split_right == NULL))
		{	
			if (l_completePages[index] == MAXCOMPLETE)
			{
				unsigned int pages = tag->real_size / l_pageSize;

				if ((tag->real_size % l_pageSize) != 0) pages += 1;
				if (pages < l_pageCount) pages = l_pageCount;

				liballoc_free(tag, pages);

				liballoc_unlock();
				return;
			}
			l_completePages[index] += 1;
		}
		insert_tag(tag, index);

	liballoc_unlock();
}

size_t allocsize(void *ptr)
{
	boundary_tag *tag = (boundary_tag*)((unsigned int)((uintptr_t)ptr) - sizeof(boundary_tag));
	return tag->real_size - sizeof(boundary_tag);
}

void* calloc(size_t nobj, size_t size)
{
	int real_size;
	void *p;

	real_size = nobj * size;

	p = malloc(real_size);

	liballoc_memset(p, 0, real_size);

	return p;
}

void* realloc(void *p, size_t size)
{
	void *ptr;
	boundary_tag *tag;
	int real_size;
	
	if (size == 0)
	{
		free(p);
		return NULL;
	}
	if (p == NULL) return malloc(size);

	liballoc_lock();
		tag = (boundary_tag*)((unsigned int)((uintptr_t)p) - sizeof(boundary_tag));
		real_size = tag->size;
	liballoc_unlock();

	if ( real_size > size ) real_size = size;

	ptr = malloc(size);
	liballoc_memcpy(ptr, p, real_size);
	free(p);

	return ptr;
}

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