#ifndef ANY_HPP
#define ANY_HPP

#include <memory>

/**
 * @brief 可以接受/返回任意类型的参数
 * @attention 类型转换失败会返回默认参数(空构造)
 * 
 * Example: usage:
 * ```
 *  Any a_int(5);
 *  int i=a_int.cast<int>();
 * ```
 */
class Any
{
public:
    Any()=default;
    ~Any()=default;
    Any(const Any& any)=delete;
    Any& operator=(const Any& any)=delete;
    Any(Any&& any)=default;
    Any& operator=(Any&& any)=default;

    template <typename T>
    Any(T data):base_(new Derive<T>(data)){}

    template <typename T>
    T cast()
    {
        Derive<T>* derive=dynamic_cast<Derive<T>*>(base_.get());
        if(nullptr==derive) return T();
        return derive->data_;
    }

private:
    class Base
    {
    public:
        virtual ~Base()=default;
    };
    template <typename T>
    class Derive:public Base
    {
    public:
        Derive(T data): data_(data){}

        T data_;
    };
    
    std::unique_ptr<Base> base_;
};

#endif