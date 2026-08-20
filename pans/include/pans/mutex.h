#ifndef PANS_INCLUDE_PANS_MUTEX_H
#define PANS_INCLUDE_PANS_MUTEX_H

#include <atomic>
#include <mutex>

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elif defined(__aarch64__) || defined(__arm__)
#include <arm_acle.h>
#endif

#if defined(__linux__) && defined(__GLIBC__)
#include <pthread.h>
#endif

// noexcept：该函数不会抛出异常
inline void cpu_relax() noexcept
{
#if defined(__x86_64__) || defined(__i386__)
    // 通常生成 PAUSE 指令，不会让线程真正休眠，也不会主动放弃CPU调度时间，而是提示处理器当前正在进行自旋等待
    _mm_pause();
#elif defined(__aarch64__) || defined(__arm__)
    __yield();
#endif
}

namespace pans
{
#if defined(__linux__) && defined(__GLIBC__)
/**
 * @brief 基于 @link(pthread_spinlock_t)  
 */
class Spinlock
{
public:
    // noexcept：该函数不会抛出异常
    Spinlock() noexcept
    {
        pthread_spin_init(&m_mutex, PTHREAD_PROCESS_PRIVATE);
    }

    ~Spinlock() noexcept
    {
        pthread_spin_destroy(&m_mutex);
    }

    // 禁止拷贝构造
    Spinlock(const Spinlock&) = delete;
    // 禁止赋值运算符构造
    Spinlock& operator=(const Spinlock&) = delete;

    void lock() noexcept
    {
        pthread_spin_lock(&m_mutex);
    }

    void unlock() noexcept
    {
        pthread_spin_unlock(&m_mutex);
    }


private:
    // pthread_spinlock_t 是 pthread 库定义的自旋锁类型
    pthread_spinlock_t m_mutex{};
}; // Spinlock

#else
class Spinlock
{
public:
    using Lock = std::lock_guard<Spinlock>;
    // noexcept：该函数不会抛出异常
    Spinlock() noexcept = default;

    ~Spinlock() noexcept = defult;

    // 禁止拷贝构造
    Spinlock(const Spinlock&) = delete;
    // 禁止赋值运算符构造
    Spinlock& operator=(const Spinlock&) = delete;

    void lock() noexcept
    {
        while (m_mutex.test_and_set(std::memory_order_acquire))
        {
            while (m_mutex.test(std::memory_order_relaxed))
            {
                cpu_relax();
            }
        }
        
    }

    // [[nodiscard]] 表示这个函数的返回值不应该被忽略
    [[nodiscard]] bool try_lock() noexcept
    {
        return !m_mutex.test_and_set(std::memory_order_acquire);
    }

    void unlock() noexcept
    {
        m_mutex.clear(std::memory_order_release);
    }


private:
    std::atomic_flag m_mutex = ATOMIC_FLAG_INIT;
};
#endif
}

#endif