#pragma once

#include <system/mm/heap/heap.hpp>
#include <stdint.h>

template<typename T>
class Array 
{
    private:
    T *array;
    uint64_t allocsize;
    public:
    uint64_t size;
    Array(uint64_t arrsize)
    {
        if (array) return;
        array = (T*)heap::malloc(arrsize * sizeof(T*));
        allocsize = heap::getsize(array);
        size = allocsize / sizeof(T*);
    }
    void destroy()
    {
        heap::free(array);
        allocsize = 0;
        size = 0;
    }
    T& operator[](uint64_t id)
    {
        while (id >= size)
        {
            array = (T*)heap::realloc(array, allocsize + sizeof(T*));
            allocsize = heap::getsize(array);
            size = allocsize / sizeof(T*);
        }
        return array[id];
    }
};