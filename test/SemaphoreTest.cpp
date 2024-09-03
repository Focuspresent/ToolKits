#include <iostream>
#include <thread>
#include <chrono>

#include "../Semaphore.hpp"

using namespace std;

int main()
{
    Semaphore<size_t> sem;

    thread t([&]{
        this_thread::sleep_for(chrono::seconds(3));
        cout<<"当前线程: "<<this_thread::get_id()<<"资源计数加一"<<endl;
        sem.post();
    });

    t.detach();

    cout<<"当前线程: "<<this_thread::get_id()<<"尝试获取资源"<<endl;
    sem.wait();
    cout<<"当前线程: "<<this_thread::get_id()<<"获取资源成功"<<endl;


    return 0;
}