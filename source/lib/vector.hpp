// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/buddy.hpp>
#include <stddef.h>

template<typename T>
class vector
{
    private:
    T *data = nullptr;
    size_t cap = 0;
    size_t num = 0;

    public:
    volatile bool on = false;

    void init()
    {
        if (on) return;
        cap = 1;
        data = new T;
        num = 0;
        on = true;
    }

    void init(size_t size)
    {
        if (on) return;
        cap = size;
        data = new T[size];
        num = 0;
        on = true;
    }

    void destroy()
    {
        if (!on) return;
        delete data;
        on = false;
    }

    void push_back(const T &item)
    {
        if (!on) this->init();
        if (num < cap)
        {
            *(data + num) = item;
            num++;
        }
        else
        {
            data = static_cast<T*>(realloc(data, 2 * cap * sizeof(T)));
            cap *= 2;
            if (data)
            {
                *(data + num) = item;
                num++;
            }
        }
    }

    void pop_back()
    {
        if (!on) return;
        num--;
        *(data + num) = nullptr;
    }

    void remove(size_t pos)
    {
        if (!on) return;
        for (size_t i = 1; i < (num - 1); i++)
        {
            *(data + pos + i - 1) = *(data + pos + i);
        }
        num--;
    }

    T &operator[](size_t pos)
    {
        return *(this->data + pos);
    }

    T &at(size_t pos)
    {
        return *(this->data + pos);
    }

    T &front()
    {
        return *this->data;
    }

    T &back()
    {
        return *(this->data + num - 1);
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
        data = static_cast<T*>(realloc(data, size * sizeof(T)));
        if (num > size) num = size + 1;
    }

    void insert(size_t pos, const T &item)
    {
        if (!on) this->init();
        if (num < cap)
        {
            for (size_t i = num - pos; i > 0; i--)
            {
                *(data + pos + i) = *(data + pos + i - 1);
            }
            *(data + pos) = item;
            num++;
        }
        else
        {
            data = static_cast<T*>(realloc(data, 2 * cap * sizeof(T)));
            cap *= 2;
            if (data)
            {
                for (size_t i = num - pos; i > 0; i--)
                {
                    *(data + pos + i) = *(data + pos + i - 1);
                }
                *(data + pos) = item;
                num++;
            }
        }
    }
};