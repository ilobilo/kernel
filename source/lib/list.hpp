// Copyright (C) 2021  ilobilo

#pragma once

#include <lib/string.hpp>
#include <lib/vector.hpp>
#include <lib/alloc.hpp>
#include <stddef.h>

template<typename type>
class list
{
    private:
    vector<const char*> keys;
    vector<type> values;
    type null;

    public:
    volatile bool on = false;

    void init()
    {
        if (this->on) return;
        this->keys.init();
        this->values.init();
        on = true;
    }

    void init(size_t size)
    {
        if (on) return;
        this->keys.init(size);
        this->values.init(size);
        on = true;
    }

    void destroy()
    {
        if (!on) return;
        this->keys.destroy();
        this->values.destroy();
        on = false;
    }

    void push_back(const char *key, const type &value)
    {
        if (!on) this->init();
        this->keys.push_back(key);
        this->values.push_back(value);
    }

    void pop_back()
    {
        if (!on) return;
        this->keys.pop_back();
        this->values.pop_back();
    }

    size_t find(type item)
    {
        return this->values.find(item);
    }

    void remove(size_t pos)
    {
        if (!on) return;
        this->keys.remove(pos);
        this->values.remove(pos);
    }

    type &operator[](size_t pos)
    {
        return this->values[pos];
    }

    type &operator[](int pos)
    {
        return this->values[pos];
    }

    type &operator[](const char *key)
    {
        for (size_t i = 0; i < this->keys.size(); i++)
        {
            if (!strcmp(this->keys[i], key)) return this->values[i];
        }
        return this->null;
    }

    type &at(size_t pos)
    {
        return this->values[pos];
    }

    type &front()
    {
        return this->values.front();
    }

    type &back()
    {
        return this->values.back();
    }

    type *begin()
    {
        return this->values.begin();
    }

    type *end()
    {
        return this->values.end();
    }

    type *data()
    {
        return this->values.data();
    }

    const type *cbegin()
    {
        return this->values.cbegin();
    }

    const type *cend()
    {
        return this->values.cend();
    }

    size_t size()
    {
        return this->values.size();
    }

    size_t max_size()
    {
        return this->values.max_size();
    }

    void resize(size_t size)
    {
        if (!on) this->init();
        this->keys.resize(size);
        this->values.resize(size);
    }

    void expand(size_t size)
    {
        if (!on) this->init();
        this->keys.expand(size);
        this->values.expand(size);
    }

    void insert(size_t pos, const char *key, const type &value)
    {
        if (!on) this->init();
        this->keys.insert(pos, key);
        this->values.insert(pos, value);
    }
};