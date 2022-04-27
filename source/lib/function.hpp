// // Copyright (C) 2021-2022  ilobilo

// #pragma once

// template<typename Result, typename ...Args>
// struct abstract_function
// {
//     virtual Result operator()(Args ...args) = 0;
//     virtual abstract_function *clone() const = 0;
//     virtual ~abstract_function() = default;
// };

// template<typename Func, typename Result, typename ...Args>
// class concrete_function : public abstract_function<Result, Args...>
// {
//     private:
//     Func f;

//     public:
//     concrete_function(const Func &x) : f(x) { };
//     Result operator()(Args ...args) override
//     {
//         return f(args...);
//     }
//     concrete_function *clone() const override
//     {
//         return new concrete_function { f };
//     }
// };

// template<typename Func>
// struct func_filter
// {
//     using type = Func;
// };

// template<typename Result, typename ...Args>
// struct func_filter<Result(Args...)>
// {
//     typedef Result (*type)(Args...);
// };

// template<typename signature>
// class function;

// template<typename Result, typename ...Args>
// class function<Result(Args...)>
// {
//     private:
//     abstract_function<Result, Args...> *f;

//     public:
//     function() : f(nullptr) { }

//     function(const function &rhs) : f(rhs.f ? rhs.f->clone() : nullptr) { }

//     template<typename Func>
//     function(const Func &x) : f(new concrete_function<typename func_filter<Func>::type, Result, Args...>(x)) { }

//     function &operator=(const function &rhs)
//     {
//         if ((&rhs != this) && rhs.f)
//         {
//             auto temp = rhs.f->clone();
//             delete this->f;
//             this->f = temp;
//         }
//         return *this;
//     }

//     template<typename Func>
//     function &operator=(const Func &x)
//     {
//         auto temp = new concrete_function<typename func_filter<Func>::type, Result, Args...>(x);
//         delete this->f;
//         this->f = temp;
//         return *this;
//     }

//     Result operator()(Args ...args)
//     {
//         if (this->f) return (*this->f)(args...);
//         else return Result();
//     }

//     ~function()
//     {
//         delete this->f;
//     }
// };

// namespace std
// {
//     template<typename func>
//     using function = ::function<func>;
// }