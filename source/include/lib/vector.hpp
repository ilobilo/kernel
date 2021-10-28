#pragma once

#include <system/mm/heap/heap.hpp>
#include <stddef.h>

template<typename T>
class Vector
{
private:
    T *vector;
    size_t cap;
    size_t num;
public:
    void init()
    {
        cap = 5;
        vector = (T*)heap::malloc(5 * sizeof(T));
        num = 0;
    }

    void init(size_t size)
    {
        cap = size;
        vector = (T*)heap::malloc(size * sizeof(T));
        num = 0;
    }

    void destroy()
    {
        heap::free(vector);
    }

    void push_back(const T &data)
    {
        if (num < cap)
        {
            *(vector + num) = data;
            num++;
        }
        else
        {
            vector = (T*)heap::realloc(vector, 2 * cap * sizeof(T*));
            cap *= 2;
            if (vector)
            {
                *(vector + num) = data;
                num++;
            }
        }
    }
    void pop_back()
    {
        num--;
    }

    T &operator[](size_t pos)
    {
        return *(this->vector + pos);
    }

    T &at(size_t pos)
    {
        return *(this->vector + pos);
    }

    T &first()
    {
        return *this->vector;
    }

    T &last()
    {
        return *(this->vector + num - 1);
    }

    size_t size()
    {
        return num;
    }

    size_t max_size()
    {
        return cap;
    }

    void resize(size_t size)
    {
        cap = size;
        vector = (T*)heap::realloc(vector, size * sizeof(T));
        if (num > size) num = size + 1;
    }

    void insert(size_t pos, const T &data)
    {
        if (num < cap)
        {
            for (size_t i = num - pos; i > 0; i--)
            {
                *(vector + pos + i) = *(vector + pos + i - 1);
            }
            *(vector + pos) = data;
            num++;
        }
        else
        {
            vector = (T*)heap::realloc(vector, 2 * cap * sizeof(T*));
            cap *= 2;
            if (vector)
            {
                for (size_t i = num - pos; i > 0; i--)
                {
                    *(vector + pos + i) = *(vector + pos + i - 1);
                }
                *(vector + pos) = data;
                num++;
            }
        }
    }
};