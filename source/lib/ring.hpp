// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/lock.hpp>
#include <cstddef>
#include <cstdint>

template<typename type>
class ringbuffer
{
    private:
    lock_t lock;
    type *buffer = nullptr;
    uint64_t head = 0;
    uint64_t tail = 0;
    size_t cap = 0;
    bool full = false;

    public:
    ringbuffer(size_t cap)
    {
        this->cap = cap;
        this->buffer = new type[cap];
    }
    ~ringbuffer()
    {
        delete[] this->buffer;
    }

    void put(type item)
    {
        lockit(this->lock);

        buffer[this->head] = item;

        if (full) this->tail = (this->tail + 1) % this->cap;
        this->head = (this->head + 1) % this->cap;
        this->full = this->head == this->tail;
    }

    type get()
    {
        lockit(this->lock);

        if (this->empty()) return 0;

        type val = this->buffer[this->tail];
        this->full = false;
        this->tail = (this->tail + 1) % this->cap;

        return val;
    }

    bool empty()
    {
        return (this->full == false && this->head == this->tail);
    }

    size_t capacity()
    {
        return this->cap;
    }

    size_t size()
    {
        lockit(this->lock);

        size_t size = this->cap;
        if (this->full == false)
        {
            if (this->head >= this->tail) size = this->head - this->tail;
            else size = this->cap + this->head - this->tail;
        }

        return size;
    }
};