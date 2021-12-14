// Copyright (C) 2021  ilobilo

#pragma once

#include <system/mm/heap/heap.hpp>
#include <stddef.h>

using namespace kernel::system::mm;

template<typename T>
class Vector
{
private:
    T *vector = NULL;
    size_t cap = 0;
    size_t num = 0;
public:
    volatile bool on = false;

    void init()
    {
        if (on) return;
        cap = 1;
        vector = (T*)heap::malloc(sizeof(T));
        num = 0;
        on = true;
    }

    void init(size_t size)
    {
        if (on) return;
        cap = size;
        vector = (T*)heap::malloc(size * sizeof(T));
        num = 0;
        on = true;
    }

    void destroy()
    {
        if (!on) return;
        heap::free(vector);
        on = false;
    }

    void push_back(const T &data)
    {
        if (!on) this->init();
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
        if (!on) return;
        num--;
    }

    void remove(size_t pos)
    {
        if (!on) return;
        for (size_t i = 1; i < (num - 1); i++)
        {
            *(vector + pos + i - 1) = *(vector + pos + i);
        }
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
        if (!on) this->init();
        cap = size;
        vector = (T*)heap::realloc(vector, size * sizeof(T));
        if (num > size) num = size + 1;
    }

    void insert(size_t pos, const T &data)
    {
        if (!on) this->init();
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