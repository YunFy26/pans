#include <iostream>
#include <cstdint>
#include <iomanip>
#include <vector>
#include <thread>
#include <latch>
#include <string>
#include <stdexcept>
#include <atomic>
#include <pans/mutex.h>



constexpr int RUNS = 7;
std::uint64_t g_target = 0;
std::uint64_t g_Counter = 0;

/**
 * @brief 基于 std::atomic_flag 实现的自旋互斥锁（纯自旋锁）
 */
class Spinlock
{
public:
    using Lock = std::lock_guard<Spinlock>;

    Spinlock() noexcept = default;
    ~Spinlock() noexcept = default;

    Spinlock(const Spinlock&) = delete;
    Spinlock& operator=(const Spinlock&) = delete;
    
    void lock() noexcept
    {   
        /*
         * test_and_set() 是一个原子的读、修改、写操作：
         *
         * 1. 读取 atomic_flag 原来的值。
         * 2. 将 atomic_flag 设置为 true。
         * 3. 返回修改前的值。
         *
         * 如果返回 false：
         *   说明锁原来处于未占用状态。
         *   当前线程已经将它设置为 true，因此成功获得锁。
         *   while 条件为 false，循环结束。
         *
         * 如果返回 true：
         *   说明锁已经被其他线程占用。
         *   while 条件为 true，当前线程进入等待循环。
         *
         * memory_order_acquire: 当前线程在 lock() 之后执行的内存操作,不能被重排到获得锁之前
         *
         */
        while (m_mutex.test_and_set(std::memory_order_acquire))
        {
            /*
             * 自旋，m_mutex.test()为 true 说明锁被其他线程持有，要自旋
             */
            while (m_mutex.test(std::memory_order_relaxed))
            {
                cpu_relax();
            }   
        }
    }

    /**
     * @brief 尝试获得锁，但是不自旋等待
     * 
     * @return true 表示成功获得锁，false 表示锁已被占用
     */
    [[nodiscard]] bool try_lock() noexcept
    {
        return !m_mutex.test_and_set(std::memory_order_acquire);
    }

    /**
     * @brief 释放锁
     */
    void unlock() noexcept
    {
        /*
         * 将 atomic_flag 清除为 false，表示锁恢复为未占用状态
         * memory_order_release: 当前线程在 unlock() 之前完成的内存操作，不能被重排到 unlock() 之后
         */
        m_mutex.clear(std::memory_order_release);
    }

private:
    // false 表示锁未被占用
    std::atomic_flag m_mutex = ATOMIC_FLAG_INIT;
}; // Spinlock

/**
 * @brief 基于 std::atomic<bool> 和 C++20 原子等待机制实现的互斥锁
 * 与 Spinlock 不同，该锁在竞争失败后调用 atomic_wait。
 *
 * atomic_wait 的具体实现由标准库和操作系统决定。实现通常会先进行
 * 少量自旋，随后可能通过 futex、WaitOnAddress 等操作系统机制挂起
 * 等待线程。因此，等待线程通常不会一直占用处理器核心。
 */
class AtomicWaitLock
{
public:
    using Lock = std::lock_guard<AtomicWaitLock>;
    AtomicWaitLock() noexcept = default;
    ~AtomicWaitLock() noexcept = default;
    AtomicWaitLock(const AtomicWaitLock&) = delete;
    AtomicWaitLock& operator=(const AtomicWaitLock&) = delete;

    /**
     * @brief 等待并获得锁
     */
    void lock() noexcept
    {
        bool expected = false;
        /*
         * compare_exchange 要求提供一个 expected 参数。
         *
         * 此处将 expected 初始化为 false，表示当前线程期望锁处于
         * 未占用状态。
         */
        bool expected = false;

        /*
         * compare_exchange_weak() 执行以下原子操作：
         *
         * 如果 m_mutex == expected：
         *   将 m_mutex 设置为 true。
         *   返回 true，表示当前线程成功获得锁。
         *
         * 如果 m_mutex != expected：
         *   不修改 m_mutex。
         *   将 m_mutex 的当前值写回 expected。
         *   返回 false。
         *
         * 因此，当锁为 false 时，操作相当于原子地完成：
         *
         *   false -> true
         *
         * 成功内存序为 memory_order_acquire，用于获得锁。
         *
         * 失败内存序为 memory_order_relaxed，因为竞争失败时并未进入
         * 临界区，不需要建立内存同步关系。
         *
         * weak 版本允许出现伪失败，即使 m_mutex 与 expected 相等，
         * 操作也可能返回 false。因为外部本来就有循环，所以 weak
         * 版本适合在这里使用。
         */
        while (!m_mutex.compare_exchange_weak(expected, true, std::memory_order_acquire, std::memory_order_relaxed))
        {
            /*
             * 加锁失败通常意味着 m_mutex 当前为 true。
             *
             * atomic_wait_explicit 的第二个参数 true 是等待值：
             *
             *   如果 m_mutex == true，则等待。
             *   如果 m_mutex != true，则立即返回。
             *
             * 这种先比较再等待的操作由原子等待机制统一处理，可以避免
             * 普通条件变量中检查状态和进入睡眠之间发生的丢失唤醒问题。
             */
            std::atomic_wait_explicit(&m_mutex, true, std::memory_order_relaxed);
            /*
             * compare_exchange 失败时，会把 m_mutex 的实际值写入
             * expected。锁被占用时，expected 通常因此变为 true。
             *
             * 下一次尝试仍然要比较 false，并将 false 改为 true，
             * 所以这里必须重新将 expected 设置为 false。
             */
            expected = false;
        }
        
    }

    /**
     * @brief 尝试获得锁，但不等待
     *
     * @return true 表示获得锁，false 表示锁已被占用
     */
    [[nodiscard]] bool try_lock() noexcept
    {
        bool expected = false;

        /*
         * strong 版本不会发生伪失败。
         *
         * 如果 m_mutex 为 false，则原子地将它设置为 true，并返回 true。
         * 如果 m_mutex 为 true，则返回 false。
         */
        return m_mutex.compare_exchange_strong(expected, true, std::memory_order_acquire, std::memory_order_relaxed);
    }

    void unlock() noexcept
    {
        m_mutex.store(false, std::memory_order_release);

        /*
         * 唤醒一个正在 m_mutex 上等待的线程
         */
        std::atomic_notify_one(&m_mutex);
    }


private:
    std::atomic<bool> m_mutex{false};
}; // AtomicWaitLock

std::uint64_t OperationsPerThread(std::size_t thread_index, std::size_t thread_count)
{
    const auto base = g_target / thread_count;
    const auto reminder = g_target % thread_count;
    return base + (thread_index < reminder ? 1 : 0);
}

template <typename Operation>
std::chrono::nanoseconds BenchmarkThreads(std::size_t thread_count, Operation&& operation)
{
    std::latch ready(static_cast<std::ptrdiff_t>(thread_count));
    std::latch start_gate(1);
    std::latch finished(static_cast<std::ptrdiff_t>(thread_count));

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (std::size_t thread_index = 0; thread_index < thread_count; thread_index++)
    {
        threads.emplace_back([&, thread_index](){
            ready.count_down();
            start_gate.wait();
            operation(thread_index, OperationsPerThread(thread_index, thread_count));
            finished.count_down();
        });
    }

    ready.wait();
    const auto begin = std::chrono::steady_clock::now();
    start_gate.count_down();
    finished.wait();
    const auto end = std::chrono::steady_clock::now();

    for (std::thread& thread : threads)
    {
        thread.join();
    }
    
    // return end - begin;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
}


template <typename Mutex>
std::chrono::nanoseconds BenchmarkMutex(std::size_t thread_count)
{
    Mutex mutex;
    g_Counter = 0;
    const auto elsaped = BenchmarkThreads(thread_count, [&mutex](std::size_t, std::uint64_t operations){
        for (std::size_t index = 0; index < operations; index++)
        {
            std::lock_guard<Mutex> lock(mutex);
            ++g_Counter;
        }
    });

    if (g_Counter != g_target)
    {
        throw std::runtime_error("mutex correctness check failed");
    }
    
    return elsaped;
}

template <typename Benchmark>
double RUN(Benchmark&& benckmark)
{
    long double total_nanoseconds = 0.0L;
    for(int i = 0; i < RUNS; ++i)
    {
        total_nanoseconds += static_cast<long double>(benckmark().count());
    }
    return static_cast<double>(total_nanoseconds / RUNS);
}

void PrintResult(std::string_view name, double avarge_nanoseconds)
{
    const double nanoseconds_per_target = avarge_nanoseconds / static_cast<double>(g_target);
    std::cout << std::left << std::setw(30) << name << std::right
              << avarge_nanoseconds << "ns \taverage_nanoseconds for " << RUNS << " RUNS\t"
              << nanoseconds_per_target << " ns/op\n";
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage ./test_mutex count\n";
        return -1;
    }

    g_target = std::stoull(argv[1]);
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "operations: " << g_target << ", average of " << RUNS << " runs.\n\n";

    const std::vector<std::size_t> thread_counts = {1, 2, 4, 8, 10};
    for (const std::size_t thread_count : thread_counts)
    {
        std::cout << "thread_count is: " << thread_count << '\n';
        PrintResult("pthread_spinlock::Spinlock", RUN([&](){ return BenchmarkMutex<pans::Spinlock>(thread_count); }));
        PrintResult("atomic_flag::Spinlock", RUN([&](){ return BenchmarkMutex<Spinlock>(thread_count); }));
        PrintResult("atomic_wait", RUN([&](){ return BenchmarkMutex<AtomicWaitLock>(thread_count); }));
        PrintResult("std::mutex", RUN([&](){ return BenchmarkMutex<std::mutex>(thread_count); }));
        std::cout << '\n';
    }
    
    
}