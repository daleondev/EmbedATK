#pragma once

#include "OSAL.h"

#include <mutex>
#include <thread>
#include <condition_variable>

class StdTimer : public OSAL::Timer
{
private:
    void start(uint64_t timeout_us) override;
    bool isExpired() const override;

    std::chrono::steady_clock::time_point m_stopTime;

    friend class StdOSAL;
};

class StdMutex : public OSAL::Mutex
{
private:
    void lock() override;
    void unlock() override;
    
    std::mutex m_mutex;

    friend class StdOSAL;
};

class StdThread : public OSAL::Thread
{
private:
    bool start() override;
    void shutdown() override;

    std::thread m_thread;

    friend class StdOSAL;
};

class StdCyclicThread : public OSAL::CyclicThread
{
private:
    bool start(uint64_t cycleTime_us) override;
    void shutdown() override;
    bool isRunning() const override;

    std::atomic_bool m_running {false};
    std::thread m_thread;
    std::condition_variable m_condition;
};

template <typename T, size_t N>
class StdMessageQueue : public OSAL::MessageQueue<T, N>
{
private:
    bool empty() const override;
    bool push(const T& msg) override;
    bool push(T&& msg) override;
    bool pushMany(const IQueue<T>& data) override;
    bool pushMany(IQueue<T>&& data) override;
    std::optional<T> pop() override;
    bool popAvail(IQueue<T>& data) override;
    std::optional<T> tryPop() override;
    bool tryPopAvail(IQueue<T>& data) override;
    
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    StaticQueue<T, N> m_queue;

    friend class StdOSAL;
};
template <typename T, size_t N>
struct OSAL::MessageQueueImpl
{
    using Type = StdMessageQueue<T, N>;
};
template <typename T, size_t N>
constexpr AllocData OSAL::messageQueueAllocData() { return allocData<StdMessageQueue<T, N>>(); }