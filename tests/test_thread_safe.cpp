#include <vector>
#include <thread>
#include <iostream>
#include <chrono>
#include <mutex>


int gs_your_money = 0;
int thread_count = 10;
int N = 100000;

void add()
{
    for (int i = 0; i < N; i++)
    {
        ++gs_your_money;
    }
    
}


int main()
{
    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; i++)
    {
        threads.emplace_back(add);
    }
    
    for (auto& t : threads)
    {
        t.join();
    }

    std::cout << "-----------------------------" << gs_your_money << std::endl;
    return 0;
    
}