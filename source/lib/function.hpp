// Copyright (C) 2021-2022  ilobilo

#pragma once

template<typename func>
class function;

template<typename retval, typename... Args>
class function<retval(Args...)>
{
    public:
    function() { };

    template<typename func>
    function(func t) : callable_(new callableFunc<func>(t)) { }

    ~function()
    {
        delete this->callable_;
    }

    template<typename func>
    function &operator=(func t)
    {
        this->callable_ = new callableFunc<func>(t);
        return *this;
    }

    retval operator()(Args ...args) const
    {
        return this->callable_->invoke(args...);
    }

    private:
    class callable {
        public:
        virtual ~callable() = default;
        virtual retval invoke(Args...) = 0;
    };

    template<typename func>
    class callableFunc : public callable {
        public:
        callableFunc(const func &t) : t_(t) { }
        ~callableFunc() override = default;

        retval invoke(Args ...args) override
        {
            return t_(args...);
        }

        private:
        func t_;
    };

    callable *callable_;
};

namespace std
{
    template<typename func>
    using function = ::function<func>;
}