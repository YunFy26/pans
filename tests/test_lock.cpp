#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <iostream>
#include <iomanip>
#include <vector>
#include <latch>
#include <cstddef>
#include <thread>


constexpr int RUNS = 7;

uint64_t g_target = 0;
uint64_t g_Counter = 0;

std::atomic<uint64_t> g_atomicCounter{0};
std::mutex g_mutex;
std::shared_mutex g_sharedMutex;

uint64_t OperationsForThread(std::size_t thread_index, std::size_t thread_count)
{
    const uint64_t base = g_target / thread_count;
    const uint64_t remainder = g_target % thread_count;
    return base + (thread_index < remainder ? 1 : 0);
}

template <typename Operation>
std::chrono::nanoseconds BenchmarkThreads(std::size_t thread_count, Operation&& operation)
{
    // latch c++20 提供的一次性线程同步工具，内部维护一个计数器
    // ready : 初始计数等于线程数量，用于等待所有工作线程完成初始化并进入就绪状态
    // 每个工作线程准备完成后调用 ready.count_down()
    // 主线程调用 ready.wait() 等待所有子线程初始化完成
    std::latch ready(static_cast<std::ptrdiff_t>(thread_count));  // ptrdiff_t : 有符号整数，可以表示两个指针相减得到的距离
    
    // start_gate : 让所有工作线程等待同一个开始信号，从而尽可能同时开始执行任务，这里初始化为1
    std::latch start_gate(1);
    
    // finished : 初始计数等于线程数量，用于等待所有工作线程完成任务
    // 每个工作线程执行完任务后调用 finished.count_down()
    // 主线程调用 finished.wait() 阻塞等待所有子线程结果
    std::latch finished(static_cast<std::ptrdiff_t>(thread_count));

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (std::size_t thread_index = 0; thread_index < thread_count; thread_index++)
    {
        // & ：表示 Lambda 函数体中实际使用的其他外部自动变量，默认按引用捕获
        // thread_index 按值捕获
        threads.emplace_back([&, thread_index]() {
            // 线程调用 count_down ，计数器减一
            ready.count_down();
            // 调用wait，阻塞等待 start_gate 变为 0
            // 这里是为每个子线程设置一个门控，等待主线程调用 start_gate.count_down() ，让子线程同时执行任务
            start_gate.wait();
            // 实际执行的任务
            operation(thread_index, OperationsForThread(thread_index, thread_count));
            // 任务完成，子线程调用 count_down
            finished.count_down();
        });
    }

    // 主线程阻塞等待子线程全都初始化完毕
    ready.wait();
    const auto begin = std::chrono::steady_clock::now();    // 时间
    // 主线程打开开关，所有子线程一起执行任务
    start_gate.count_down();
    // 主线程阻塞等待子线程的执行结果
    finished.wait();
    const auto end = std::chrono::steady_clock::now();      // 时间
    
    for(std::thread& thread : threads)
    {
        thread.join();
    }
    
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
}

std::chrono::nanoseconds BenchmarkMutex(std::size_t thread_count)
{
    g_Counter = 0;
    // (std::size_t, uint64_t operations) 是lambda参数列表
    // 这里第一个参数没有参数名，表示lambda接受这个参数但是未使用
    const auto elapsed = BenchmarkThreads(thread_count, [](std::size_t, uint64_t operations){
        for (uint64_t index = 0; index < operations; index++)
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            ++g_Counter;
        }
    });
    return elapsed;
}

std::chrono::nanoseconds BenchmarkSharedMutex(std::size_t thread_count)
{
    g_Counter = 0;
    const auto elapsed = BenchmarkThreads(thread_count, [](std::size_t, uint64_t operations){
        for (uint64_t index = 0; index < operations; index++)
        {
            std::unique_lock<std::shared_mutex> lock(g_sharedMutex);
            ++g_Counter;
        }
    });
    return elapsed;
}

std::chrono::nanoseconds BenchmarkAtomic(std::size_t thread_count)
{
    // memory_order_relaxed 宽松内存序，只保证本次读写操作本身具有原子性，不负责在线程之间同步其他变量的访问顺序
    g_atomicCounter.store(0, std::memory_order_relaxed);
    const auto elapsed = BenchmarkThreads(thread_count, [](std::size_t, uint64_t operations){
        for (uint64_t index = 0; index < operations; index++)
        {
            g_atomicCounter.fetch_add(1, std::memory_order_relaxed);
        }
        
    });
    return elapsed;
}


std::chrono::nanoseconds BenchmarkAtomicCAS(std::size_t thread_count)
{
    g_atomicCounter = 0;
    const auto elapsed = BenchmarkThreads(thread_count, [](std::size_t, uint64_t operations) {
        for (uint64_t index = 0; index < operations; index++)
        {
            uint64_t expected = g_atomicCounter.load(std::memory_order_relaxed);
            while (!g_atomicCounter.compare_exchange_weak(expected, expected + 1, std::memory_order_relaxed, std::memory_order_relaxed))
            {
                /* code */
            }
            
        }
        
    });
    return elapsed;
}

void PrintResult(std::string_view name, double average_nanoseconds)
{
    const double nanoseconds_per_operation = average_nanoseconds / static_cast<double>(g_target);
    // 流操作符
    // std::left 左对齐  std::setw(14) 设置宽度为14个字符
    std::cout << std::left << std::setw(14) << name << std::right
              << average_nanoseconds << "ns \taverage_nanoseconds, "
              << nanoseconds_per_operation << " ns/op\n";
}

// 执行 RUNS 轮，每轮执行 g_target 次操作
template <typename Benchmark>
double Run(Benchmark&& benchmark)
{
    long double total_nanoseconds = 0.0L;
    for (int run = 0; run < RUNS; ++run)
    {
        total_nanoseconds += static_cast<long double>(benchmark().count());
    }
    return static_cast<double>(total_nanoseconds / RUNS);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./test_lock count\n";
        return -1;
    }
    
    // atoi: ascii to int
    g_target = std::atoi(argv[1]);
    // 把标准输出流设置成固定小数位显示，并保留两位小数
    std::cout << std::fixed << std::setprecision(2);
    // std::cout << "opreations: " << g_target << ", average of " << RUNS << " runs\n\n";
    std::cout << "Testing 4 synchronization mechanisms with 1, 2, 4, 8, and 10 threads.\n"
          << "For each thread count, every mechanism performs "
          << RUNS * g_target << " operations in " << RUNS
          << " runs; results are averaged.\n\n";
    const std::vector<std::size_t> thread_counts = {1, 2, 4, 8, 10};
    for(const std::size_t thread_count : thread_counts)
    {
        std::cout << "thread counts: " << thread_count << '\n';
        PrintResult("mutex", Run([&]() {return BenchmarkMutex(thread_count);}));
        PrintResult("shared mutex", Run([&]() {return BenchmarkSharedMutex(thread_count);}));
        PrintResult("atomic", Run([&]() {return BenchmarkAtomic(thread_count);}));
        PrintResult("atomic CAS", Run([&]() {return BenchmarkAtomicCAS(thread_count);}));
        std::cout << '\n';
    }
    return 0;
}


