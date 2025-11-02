#pragma once

#include "EmbedATK/OSAL/OSAL.h"

#include "tx_api.h"

class ArmMutex : public OSAL::Mutex
{
public:
    ArmMutex();
    ~ArmMutex();
private:
    void lock() override;
    void unlock() override;

    size_t m_id;
    TX_MUTEX m_mutex;

    friend class ArmOSAL;
};

class ArmThread : public OSAL::Thread
{
public:
    ~ArmThread();
private:
    bool start() override;
    void shutdown() override;
    bool setPriority(int prio, int policy) override;
    static VOID taskWrapper(ULONG context);

    TX_THREAD m_thread;
    TX_SEMAPHORE m_taskDone;

    friend class ArmOSAL;
};

class ArmCyclicThread : public OSAL::CyclicThread
{
public:
    ~ArmCyclicThread();
private:
    bool start(uint64_t cycleTime_us) override;
    void shutdown() override;
    bool setPriority(int prio, int policy) override;
    bool isRunning() const override;
    static VOID cyclicTaskWrapper(ULONG context);

    std::atomic_bool m_running;
    TX_SEMAPHORE m_started;
    TX_SEMAPHORE m_shutdown;
    uint64_t m_cycleTime_us;

    TX_THREAD m_thread;
    TX_SEMAPHORE m_taskDone;

    friend class ArmOSAL;
};

class ArmMessageQueue : public OSAL::MessageQueue
{
public:
    ArmMessageQueue(IObjectStore<SboAny>& store);
    ~ArmMessageQueue();
private:
    bool empty() const override;
    bool push(SboAny&& msg) override;
    bool pushMany(IQueue<SboAny>&& data) override;
    std::optional<SboAny> pop() override;
    bool popAvail(IQueue<SboAny>& data) override;
    std::optional<SboAny> tryPop() override;
    bool tryPopAvail(IQueue<SboAny>& data) override;
    
    size_t m_id;
    IObjectStore<SboAny>& m_store;
    TX_QUEUE m_queue;

    friend class ArmOSAL;
};

struct OSAL::StaticImpl
{
    using Timer         = StaticPolymorphic<OSAL::Timer, OSAL::Timer>;
    using Mutex         = StaticPolymorphic<OSAL::Mutex, ArmMutex>;
    using Thread        = StaticPolymorphic<OSAL::Thread, ArmThread>;
    using CyclicThread  = StaticPolymorphic<OSAL::CyclicThread, ArmCyclicThread>;
    using MessageQueue  = StaticPolymorphic<OSAL::MessageQueue, ArmMessageQueue>;
};