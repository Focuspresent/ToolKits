#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

#include <mutex>
#include <condition_variable>

/**
 * @brief 信号量模版类
 */
template <typename T=size_t>
class Semaphore
{
public:
    Semaphore(T count=T()):count_(count)
    {}
    ~Semaphore()=default;
    Semaphore(const Semaphore&)=delete;
    Semaphore& operator=(const Semaphore&)=delete;

    // 获取一个资源
    void wait()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cond_.wait(lock,[&](){
            return count_>0;
        });
        count_--;
    }

    // 增加一个资源
    void post()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        count_++;
        cond_.notify_all();
    }
private:
    T count_; ///< 计数
    std::mutex mtx_; ///< 互斥锁
    std::condition_variable cond_; ///< 条件变量
};

#endif