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
    uint64_t count;
    Array()
    {
        if (array) return;
        array = (T*)heap::calloc(1, sizeof(T*));
        allocsize = heap::getsize(array);
        size = allocsize / sizeof(T*);
        count = 0;
    }
    void destroy()
    {
        heap::free(array);
        allocsize = 0;
        size = 0;
        count = 0;
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
    void push(T item)
    {
        while (count >= size)
        {
            array = (T*)heap::realloc(array, allocsize + sizeof(T*));
            allocsize = heap::getsize(array);
            size = allocsize / sizeof(T*);
        }
        array[count] = item;
        count++;
    }
    void pop()
    {
        array[--count] = 0;
    }
    void remove(uint64_t id)
    {
        if (id >= count) return;
        if (id == (count - 1)) array[id] = 0; 
        else
        {
            for (size_t i = id; i < count; i++)
            {
                array[i] = array[i + 1];
            }
        }
        count--;
    }
};