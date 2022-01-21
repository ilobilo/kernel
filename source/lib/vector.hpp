// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/memory.hpp>
#include <lib/alloc.hpp>
#include <stddef.h>

template<typename T>
class vector
{
    private:
    T *storage = nullptr;
    size_t cap = 0;
    size_t num = 0;

    public:
    volatile bool on = false;

    void init()
    {
        if (on) return;
        storage = new T;
        cap = allocsize(storage) / sizeof(T);
        num = 0;
        on = true;
    }

    void init(size_t size)
    {
        if (on) return;
        storage = new T[size];
        cap = allocsize(storage) / sizeof(T);
        num = 0;
        on = true;
    }

    void destroy()
    {
        if (!on) return;
        delete storage;
        on = false;
    }

    void push_back(const T &item)
    {
        if (!on) this->init();
        if (num < cap)
        {
            *(storage + num) = item;
            num++;
        }
        else
        {
            storage = static_cast<T*>(realloc(storage, (cap + 2) * sizeof(T)));
            cap = allocsize(storage) / sizeof(T);
            if (storage)
            {
                *(storage + num) = item;
                num++;
            }
        }
    }

    void pop_back()
    {
        if (!on) return;
        memset(&*(storage + --num), 0, sizeof(T));
    }

    size_t find(T item)
    {
        for (size_t i = 0; i < num; i++)
        {
            if (item == *(storage + i)) return i;
        }
        return -1;
    }

    void remove(size_t pos)
    {
        if (!on) return;
        if (pos >= num) return;
        memset(&*(storage + pos), 0, sizeof(T));
        for (size_t i = 0; i < num - 1; i++)
        {
            *(storage + pos + i) = *(storage + pos + i + 1);
        }
        num--;
    }

    T &operator[](size_t pos)
    {
        return *(this->storage + pos);
    }

    T &at(size_t pos)
    {
        return *(this->storage + pos);
    }

    T &front()
    {
        return *this->storage;
    }

    T &back()
    {
        return *(this->storage + num - 1);
    }

    T *begin()
    {
        return &*(this->storage);
    }

    T *end()
    {
        return &*(this->storage + num);
    }

    T *data()
    {
        return this->data;
    }

    const T *cbegin()
    {
        return this->storage;
    }

    const T *cend()
    {
        return this->storage + num;
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
        storage = static_cast<T*>(realloc(storage, size * sizeof(T)));
        cap = allocsize(storage) / sizeof(T);
        if (num > size) num = size + 1;
    }

    void expand(size_t size)
    {
        if (!on) this->init();
        storage = static_cast<T*>(realloc(storage, size * sizeof(T)));
        cap = allocsize(storage) / sizeof(T);
    }

    void insert(size_t pos, const T &item)
    {
        if (!on) this->init();
        if (num < cap)
        {
            for (size_t i = num - pos; i > 0; i--)
            {
                *(storage + pos + i) = *(storage + pos + i - 1);
            }
            *(storage + pos) = item;
            num++;
        }
        else
        {
            storage = static_cast<T*>(realloc(storage, (cap + 1) * sizeof(T)));
            cap = allocsize(storage) / sizeof(T);
            if (storage)
            {
                for (size_t i = num - pos; i > 0; i--)
                {
                    *(storage + pos + i) = *(storage + pos + i - 1);
                }
                *(storage + pos) = item;
                num++;
            }
        }
    }
};