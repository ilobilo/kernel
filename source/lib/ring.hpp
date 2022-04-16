// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <lib/lock.hpp>
#include <cstddef>
#include <cstdint>

static constexpr uint64_t default_ring_size = 0x1000;

template<typename type>
class ringbuffer
{
    private:
    lock_t lock;
    type *buffer = nullptr;
    uint64_t head = 0;
    uint64_t tail = 0;
    size_t cap = 0;
    bool isfull = false;

    public:
    ringbuffer(size_t cap)
    {
        this->cap = cap;
        this->buffer = new type[cap];
    }
    ringbuffer()
    {
        this->cap = default_ring_size;
        this->buffer = new type[default_ring_size];
    }
    ~ringbuffer()
    {
        delete[] this->buffer;
    }

    void put(type item)
    {
        lockit(this->lock);

        buffer[this->head] = item;

        if (this->isfull) this->tail = (this->tail + 1) % this->cap;
        this->head = (this->head + 1) % this->cap;
        this->isfull = this->head == this->tail;
    }

    type get(bool remove = true)
    {
        lockit(this->lock);

        if (this->empty()) return 0;

        type val = this->buffer[this->tail];
        if (remove == false) return val;

        this->isfull = false;
        this->tail = (this->tail + 1) % this->cap;

        return val;
    }

    void clear()
    {
        lockit(this->lock);

        this->head = this->tail;
        this->isfull = false;
    }

    bool empty()
    {
        return (this->isfull == false && this->head == this->tail);
    }

    bool full()
    {
        return this->isfull;
    }

    size_t capacity()
    {
        return this->cap;
    }

    size_t size()
    {
        lockit(this->lock);

        size_t size = this->cap;
        if (this->isfull == false)
        {
            if (this->head >= this->tail) size = this->head - this->tail;
            else size = this->cap + this->head - this->tail;
        }

        return size;
    }

    void copyto(ringbuffer<type> &newring)
    {
        size_t s = this->size();
        for (size_t i = 0; i < s; i++)
        {
            newring.put(this->buffer[i]);
        }
    }
};