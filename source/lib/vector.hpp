// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/memory.hpp>
#include <lib/alloc.hpp>
#include <stddef.h>

template<typename type>
class vector
{
    private:
    type *storage = nullptr;
    size_t cap = 0;
    size_t num = 0;

    public:
    volatile bool on = false;

    ~vector()
    {
        this->destroy();
    }

    void init()
    {
        if (this->on) return;
        this->storage = new type;
        this->cap = allocsize(this->storage) / sizeof(type);
        this->num = 0;
        this->on = true;
    }

    void init(size_t size)
    {
        if (this->on) return;
        this->storage = new type[size];
        this->cap = allocsize(this->storage) / sizeof(type);
        this->num = 0;
        this->on = true;
    }

    void destroy()
    {
        if (!this->on) return;
        delete this->storage;
        this->on = false;
    }

    void push_back(const type &item)
    {
        if (!this->on) this->init();
        if (this->num < this->cap)
        {
            *(this->storage + this->num) = item;
            this->num++;
        }
        else
        {
            this->storage = static_cast<type*>(realloc(this->storage, (this->cap + 2) * sizeof(type)));
            this->cap = allocsize(this->storage) / sizeof(type);
            if (this->storage)
            {
                *(this->storage + this->num) = item;
                this->num++;
            }
        }
    }

    void pop_back()
    {
        if (!this->on) return;
        memset(&*(this->storage + --this->num), 0, sizeof(type));
    }

    size_t find(type item)
    {
        for (size_t i = 0; i < this->num; i++)
        {
            if (item == *(this->storage + i)) return i;
        }
        return -1;
    }

    void remove(size_t pos)
    {
        if (!this->on) return;
        if (pos >= this->num) return;
        memset(&*(this->storage + pos), 0, sizeof(type));
        for (size_t i = 0; i < this->num - 1; i++)
        {
            *(this->storage + pos + i) = *(this->storage + pos + i + 1);
        }
        this->num--;
    }

    void remove(type &item)
    {
        if (!this->on) return;
        for (size_t i = 0; i < this->num; i++)
        {
            if (*(this->storage + i) == item) this->remove(i);
        }
    }

    void copyfrom(vector<type> old)
    {
        memset(this->storage, 0, this->num - 1);
        this->num = 0;
        for (size_t i = 0; i < old.size(); i++)
        {
            this->push_back(old[i]);
        }
    }

    type &operator[](size_t pos)
    {
        return *(this->storage + pos);
    }

    type &at(size_t pos)
    {
        return *(this->storage + pos);
    }

    type &front()
    {
        return *this->storage;
    }

    type &back()
    {
        return *(this->storage + this->num - 1);
    }

    type *begin()
    {
        if (this->storage == nullptr) return nullptr;
        return this->storage;
    }

    type *end()
    {
        if (this->storage == nullptr) return nullptr;
        return this->storage + this->num;
    }

    type *data()
    {
        return this->data;
    }

    const type *cbegin()
    {
        if (this->storage == nullptr) return nullptr;
        return this->storage;
    }

    const type *cend()
    {
        if (this->storage == nullptr) return nullptr;
        return this->storage + this->num;
    }

    bool empty()
    {
        return this->num == 0;
    }

    size_t size()
    {
        return this->num;
    }

    size_t max_size()
    {
        return this->cap;
    }

    void resize(size_t size)
    {
        if (!this->on) this->init();
        this->storage = static_cast<type*>(realloc(this->storage, size * sizeof(type)));
        this->cap = allocsize(this->storage) / sizeof(type);
        if (this->num > size) this->num = size + 1;
    }

    void expand(size_t size)
    {
        if (!this->on) this->init();
        this->storage = static_cast<type*>(realloc(this->storage, size * sizeof(type)));
        this->cap = allocsize(this->storage) / sizeof(type);
    }

    void insert(size_t pos, const type &item)
    {
        if (!this->on) this->init();
        if (this->num < this->cap)
        {
            for (size_t i = this->num - pos; i > 0; i--)
            {
                *(this->storage + pos + i) = *(this->storage + pos + i - 1);
            }
            *(this->storage + pos) = item;
            this->num++;
        }
        else
        {
            this->storage = static_cast<type*>(realloc(this->storage, (this->cap + 1) * sizeof(type)));
            this->cap = allocsize(this->storage) / sizeof(type);
            if (this->storage)
            {
                for (size_t i = this->num - pos; i > 0; i--)
                {
                    *(this->storage + pos + i) = *(this->storage + pos + i - 1);
                }
                *(this->storage + pos) = item;
                this->num++;
            }
        }
    }

    void reverse()
    {
        if (this->num <= 1) return;
        for (size_t i = 0, j = this->num - 1; i < j; i++, j--)
        {
            type c = *(this->storage + i);
            *(this->storage + i) = *(this->storage + j);
            *(this->storage + j) = c;
        }
    }
};