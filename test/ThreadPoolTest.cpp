#include <iostream>
#include <chrono>

#include "../ThreadPool.hpp"

using namespace std;
using namespace pool;

int main(){
    ThreadPool pool(4);
    vector<future<int>> res;
    for(int i=0;i<8;i++){
        res.emplace_back(pool.enqueue([](int a)->int{
            this_thread::sleep_for(chrono::seconds(1));
            return a*2;
        },i));
    }
    for(auto& r: res){
        cout<<r.get()<<endl;
    }
    return 0;
}