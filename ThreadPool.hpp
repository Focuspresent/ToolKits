#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <thread>
#include <queue>
#include <future>

namespace pool
{
    class ThreadPool
    {
    public:
        ThreadPool(size_t size);
        template<typename F,typename...Args>
        auto enqueue(F&& f,Args&&... args)
            ->std::future<typename std::result_of<F(Args...)>::type>;
        ~ThreadPool();
    private:
        std::vector<std::thread> threads_;
        std::queue<std::function<void()>> tasks_;
        std::mutex mutex_;
        std::condition_variable cond_;
        bool stop;
    };

    ThreadPool::ThreadPool(size_t size)
        :stop(false)
    {
        for(size_t i=0;i<size;i++)
        {
            threads_.emplace_back(
                [this]
                {
                    for(;;)
                    {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(this->mutex_);
                            cond_.wait(lock,[this]{
                                return this->stop||!this->tasks_.empty();
                            });
                            if(this->stop&&this->tasks_.empty()) return ;
                            task=std::move(this->tasks_.front());
                            tasks_.pop();
                        }
                        task();
                    }
                }
            );
        }
    }

    template<typename F,typename...Args>
    auto ThreadPool::enqueue(F&& f,Args&&... args)
        ->std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type=typename std::result_of<F(Args...)>::type;

        auto task=std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f),std::forward<Args>(args)...)
        );

        std::future<return_type> res=task->get_future();
        {
            std::unique_lock<std::mutex> lock(mutex_);
            tasks_.emplace([task](){(*task)();});
        }
        return res;
    }

    ThreadPool::~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            stop=true;
        }
        cond_.notify_all();
        for(auto& th:threads_){
            th.join();
        }
    }
}

#endif