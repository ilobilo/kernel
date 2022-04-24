// Copyright (C) 2021-2022  ilobilo

#pragma once

#include <cstdint>

template<typename type>
class shared_ptr
{
    private:
    type *ptr = nullptr;
    uint32_t *refCount = nullptr;

    void __cleanup__()
    {
        (*refCount)--;
        if (*refCount == 0)
        {
            if (ptr != nullptr) delete ptr;
            delete refCount;
        }
    }

    public:
    shared_ptr() : ptr(nullptr), refCount(new uint32_t(0)) { }
    shared_ptr(type *ptr) : ptr(ptr), refCount(new uint32_t(1)) { }

    shared_ptr(const shared_ptr &obj)
    {
        this->ptr = obj.ptr;
        this->refCount = obj.refCount;
        if (nullptr != obj.ptr)
        {
            (*this->refCount)++;
        }
    }

    shared_ptr &operator=(const shared_ptr &obj)
    {
        __cleanup__();

        this->ptr = obj.ptr;
        this->refCount = obj.refCount;
        if (nullptr != obj.ptr)
        {
            (*this->refCount)++;
        }
    }

    // shared_ptr(shared_ptr &dyingObj)
    // {
    //     this->ptr = dyingObj.ptr;
    //     this->refCount = dyingObj.refCount;

    //     dyingObj.ptr = nullptr;
    //     dyingObj.refCount = nullptr;
    // }

    shared_ptr &operator=(shared_ptr &dyingObj)
    {
        __cleanup__();

        this->ptr = dyingObj.ptr;
        this->refCount = dyingObj.refCount;

        dyingObj.ptr = dyingObj.refCount = nullptr;
    }

	type &operator[](size_t pos)
	{
		return *(this->ptr + pos);
	}

    uint32_t get_count() const
    {
        return *refCount;
    }

    type *get() const
    {
        return this->ptr;
    }

    type *operator->() const
    {
        return this->ptr;
    }

    type &operator*() const
    {
        return this->ptr;
    }

    ~shared_ptr()
    {
        __cleanup__();
    }
};

namespace std
{
    template<typename type>
    using shared_ptr = ::shared_ptr<type>;
}