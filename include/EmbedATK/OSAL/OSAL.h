#pragma once

#include "EmbedATK/Core/Core.h"

#include "EmbedATK/Memory/Polymorphic.h"
#include "EmbedATK/Memory/Any.h"
#include "EmbedATK/Container/Queue.h"

#include "EmbedATK/Utils/Timestamp.h"

enum class ConsoleColor
{
    Standard,
    Green,
    Yellow,
    Red,
    Cyan,
};

class OSAL
{
public:
    virtual ~OSAL() = default;

    // --- Network ---
    static uint16_t hostToNetwork(uint16_t h) { return instance().hostToNetworkImpl(h); }
    static uint16_t networkToHost(uint16_t n) { return instance().networkToHostImpl(n); }

    // --- Printing ---
    static void print(const char* msg) { instance().printImpl(msg); }
    static void println(const char* msg) { instance().printlnImpl(msg); }
    static void eprint(const char* emsg) { instance().eprintImpl(emsg); }
    static void eprintln(const char* emsg) { instance().eprintlnImpl(emsg); }
    static void setConsoleColor(ConsoleColor col) { instance().setConsoleColorImpl(col); }

    // --- Time ---
    static uint64_t monotonicTime() { return instance().monotonicTimeImpl(); };
    static Timestamp currentTime() { return instance().currentTimeImpl(); };
    static bool sleep(uint64_t us) { return instance().sleepImpl(us); };
    static bool sleepUntil(uint64_t monotonic) { return instance().sleepUntilImpl(monotonic); };

    // --- Timer ---
    class Timer
    {
    public:
        virtual ~Timer() = default;
        virtual void start(uint64_t timeout_us)
        {
            m_stopTime = monotonicTime() + timeout_us;
        }
        virtual bool isExpired() const
        {
            const auto now = monotonicTime();
            return now >= m_stopTime;
        }
        uint64_t m_stopTime;
    };
    static void createTimer(IPolymorphic<Timer>& timer) { instance().createTimerImpl(timer); }

    // --- Mutex ---
    class Mutex
    {
    public:
        virtual ~Mutex() = default;
        virtual void lock() = 0;
        virtual void unlock() = 0;
    private:
        struct AllocInfo;
    };
    static void createMutex(IPolymorphic<Mutex>& mutex) { instance().createMutexImpl(mutex); }

    // --- Thread ---
    class Thread
    {
    public:
        virtual ~Thread() = default;
        virtual bool start() = 0;
        virtual void shutdown() = 0;
        virtual bool setPriority(int prio, int policy = 0) = 0;
    protected: 
        int m_prio;
        std::span<std::byte> m_stack;
        std::function<void()> m_task;
    private:
        void setPrio(int prio) { m_prio = prio; }
        void setStack(std::span<std::byte> stack) { m_stack = stack; }
        void setTask(std::function<void()>&& task) { m_task = std::move(task); }
        friend class OSAL;
    };
    template<typename Callable, typename... Args>
    static void createThread(IPolymorphic<Thread>& thread, int prio, std::span<std::byte> stack, Callable&& func, Args&&... args) 
    { 
        instance().createThreadImpl(thread); 
        thread.get()->setPrio(prio);
        thread.get()->setStack(stack);
        auto task = std::bind(std::forward<Callable>(func), std::forward<Args>(args)...);
        thread.get()->setTask(std::move(task));
    }

    // --- Cyclic Thread ---
    class CyclicThread
    {
    public:
        virtual ~CyclicThread() = default;
        virtual bool start(uint64_t cycleTime_us = 0) = 0;
        virtual void shutdown() = 0;
        virtual bool setPriority(int prio, int policy = 0) = 0;
        virtual bool isRunning() const = 0;
    protected: 
        int m_prio;
        std::span<std::byte> m_stack;
        std::function<void()> m_cyclicTask;
    private:
        void setPrio(int prio) { m_prio = prio; }
        void setStack(std::span<std::byte> stack) { m_stack = stack; }
        void setCyclicTask(std::function<void()>&& cyclicTask) { m_cyclicTask = std::move(cyclicTask); }
        friend class OSAL;
    };
    template<typename Callable, typename... Args>
    static void createCyclicThread(IPolymorphic<CyclicThread>& cyclicThread, int prio, std::span<std::byte> stack, Callable&& func, Args&&... args) 
    { 
        instance().createCyclicThreadImpl(cyclicThread); 
        cyclicThread.get()->setPrio(prio);
        cyclicThread.get()->setStack(stack);
        auto cyclicTask = std::bind(std::forward<Callable>(func), std::forward<Args>(args)...);
        cyclicThread.get()->setCyclicTask(std::move(cyclicTask));
    }

    // --- Message-Queue ---
    class MessageQueue
    {
    public:
        using MsgType = StaticAny<8>;
        virtual ~MessageQueue() = default;
        virtual bool empty() const = 0;
        virtual bool push(MsgType&& msg) = 0;
        virtual bool push(const MsgType& msg) = 0;
        virtual bool pushMany(IQueue<MsgType>&& data) = 0;
        virtual std::optional<MsgType> pop() = 0;
        virtual bool popAvail(IQueue<MsgType>& data) = 0;
        virtual std::optional<MsgType> tryPop() = 0;
        virtual bool tryPopAvail(IQueue<MsgType>& data) = 0;
    };
    static void createMessageQueue(IPolymorphic<MessageQueue>& queue, IObjectStore<MessageQueue::MsgType*>& store, IPool& pool)
    {
        instance().createMessageQueueImpl(queue, store, pool);
    }

    struct IMessageQueueDef
    {
        virtual IPolymorphic<MessageQueue>& queue() = 0;
        virtual IObjectStore<MessageQueue::MsgType*>& store() = 0;
        virtual IPool& pool() = 0;
    };
    template<typename Impl, size_t N>
    requires std::is_base_of_v<IPolymorphic<MessageQueue>, Impl>
    struct StaticMessageQueueDef : public IMessageQueueDef
    {
    public:
        IPolymorphic<MessageQueue>& queue() override { return m_queue; }
        IObjectStore<MessageQueue::MsgType*>& store() override { return m_store; }
        IPool& pool() override { return m_pool; }
    private:
        StaticBlockPool<N, allocData<MessageQueue::MsgType>()> m_pool;
        StaticObjectStore<MessageQueue::MsgType*, N> m_store;
        Impl m_queue;
    };
    static void createMessageQueue(IMessageQueueDef& def)
    {
        instance().createMessageQueueImpl(def.queue(), def.store(), def.pool());
    }

    struct StaticImpl;
    struct DynamicImpl
    {
        using Timer         = DynamicPolymorphic<OSAL::Timer>;
        using Mutex         = DynamicPolymorphic<OSAL::Mutex>;
        using Thread        = DynamicPolymorphic<OSAL::Thread>;
        using CyclicThread  = DynamicPolymorphic<OSAL::CyclicThread>;
        using MessageQueue  = DynamicPolymorphic<OSAL::MessageQueue>;
    };

protected:
    // --- Platform specific implementations to override ---
    virtual uint16_t hostToNetworkImpl(uint16_t) const = 0;
    virtual uint16_t networkToHostImpl(uint16_t) const = 0;
    virtual void printImpl(const char*) const = 0;
    virtual void printlnImpl(const char*) const = 0;
    virtual void eprintImpl(const char*) const = 0;
    virtual void eprintlnImpl(const char*) const = 0;
    virtual void setConsoleColorImpl(ConsoleColor) const = 0;
    virtual uint64_t monotonicTimeImpl() const = 0;
    virtual Timestamp currentTimeImpl() const = 0;
    virtual bool sleepImpl(uint64_t) const = 0;
    virtual bool sleepUntilImpl(uint64_t) const = 0;
    virtual void createTimerImpl(IPolymorphic<Timer>&) const = 0;
    virtual void createMutexImpl(IPolymorphic<Mutex>&) const = 0;
    virtual void createThreadImpl(IPolymorphic<Thread>&) const = 0;
    virtual void createCyclicThreadImpl(IPolymorphic<CyclicThread>&) const = 0;
    virtual void createMessageQueueImpl(IPolymorphic<MessageQueue>&, IObjectStore<MessageQueue::MsgType*>&, IPool&) const = 0;

private:
    // --- Singleton instance ---
    static const OSAL& instance();
};