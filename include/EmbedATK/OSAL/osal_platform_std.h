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

class StdMessageQueue : public OSAL::MessageQueue
{
private:
    bool empty() const override;
    bool push(SboAny&& msg) override;
    bool pushMany(IQueue<SboAny>&& data) override;
    std::optional<SboAny> pop() override;
    bool popAvail(IQueue<SboAny>& data) override;
    std::optional<SboAny> tryPop() override;
    bool tryPopAvail(IQueue<SboAny>& data) override;
    
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    StaticQueueView<SboAny> m_queue{OSAL::MessageQueue::m_store};

    friend class StdOSAL;
};