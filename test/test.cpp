#include "../thread_pool.h"

#include <algorithm>
#include <iostream>
#include <ostream>
#include <vector>
#include <future>

int work(int n) {
    int i = 100000;
    while (--i) {
        n *= i;
    }
    return n % 100; 
}

int main() {
    Thread_pool mpool(8);
    mpool.init();
    std::vector<std::future<int>> res;
    for (int i =  0; i < 10000; i++) {
        res.emplace_back(mpool.submit(work, i));
    }
    for (int i =  0; i < res.size(); i++) {
        res[i].get();
    }
    std::cout << "end" << std::endl;
    mpool.shutdown();
}