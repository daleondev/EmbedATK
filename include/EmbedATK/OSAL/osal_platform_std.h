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
public:
    StdMessageQueue(IObjectStore<MsgType*>& store, IPool& pool) 
        : m_pool(pool), m_store(store), m_queue(m_store) {}
private:
    bool empty() const override;
    bool push(MsgType&& msg) override;
    bool pushMany(IQueue<MsgType>&& data) override;
    std::optional<MsgType> pop() override;
    bool popAvail(IQueue<MsgType>& data) override;
    std::optional<MsgType> tryPop() override;
    bool tryPopAvail(IQueue<MsgType>& data) override;
    
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;

    IPool& m_pool;
    IObjectStore<MsgType*>& m_store;
    StaticQueueView<MsgType*> m_queue;

    friend class StdOSAL;
};

// static_assert(std::is_default_constructible_v<StdMessageQueue>);