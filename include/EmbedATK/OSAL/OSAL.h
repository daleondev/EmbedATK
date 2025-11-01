#pragma once

#include "EmbedATK/Memory/Polymorphic.h"
#include "EmbedATK/Memory/Container.h"

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
    static constexpr AllocData timerAllocData();

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
    static constexpr AllocData mutexAllocData();

    // --- Thread ---
    class Thread
    {
    public:
        virtual ~Thread() = default;
        virtual bool start() = 0;
        virtual void shutdown() = 0;
        virtual bool setPriority(int prio, int policy = 0) = 0;
    protected: 
        std::span<std::byte> m_stack;
        std::function<void()> m_task;
    private:
        void setStack(std::span<std::byte> stack) { m_stack = stack; }
        void setTask(std::function<void()>&& task) { m_task = std::move(task); }
        friend class OSAL;
    };
    template<typename Callable, typename... Args>
    static void createThread(IPolymorphic<Thread>& thread, std::span<std::byte> stack, Callable&& func, Args&&... args) 
    { 
        instance().createThreadImpl(thread); 
        thread.get()->setStack(stack);
        auto task = std::bind(std::forward<Callable>(func), std::forward<Args>(args)...);
        thread.get()->setTask(std::move(task));
    }
    static constexpr AllocData threadAllocData();

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
        std::span<std::byte> m_stack;
        std::function<void()> m_cyclicTask;
    private:
        void setStack(std::span<std::byte> stack) { m_stack = stack; }
        void setCyclicTask(std::function<void()>&& cyclicTask) { m_cyclicTask = std::move(cyclicTask); }
        friend class OSAL;
    };
    template<typename Callable, typename... Args>
    static void createCyclicThread(IPolymorphic<CyclicThread>& cyclicThread, std::span<std::byte> stack, Callable&& func, Args&&... args) 
    { 
        instance().createCyclicThreadImpl(cyclicThread); 
        cyclicThread.get()->setStack(stack);
        auto cyclicTask = std::bind(std::forward<Callable>(func), std::forward<Args>(args)...);
        cyclicThread.get()->setCyclicTask(std::move(cyclicTask));
    }
    static constexpr AllocData cyclicThreadAllocData();

    // --- Message-Queue ---
    template <typename T, size_t N>
    class MessageQueue
    {
    public:
        virtual ~MessageQueue() = default;
        virtual bool empty() const = 0;
        virtual bool push(const T& msg) = 0;
        virtual bool push(T&& msg) = 0;
        virtual bool pushMany(const IQueue<T>& data) = 0;
        virtual bool pushMany(IQueue<T>&& data) = 0;
        virtual std::optional<T> pop() = 0;
        virtual bool popAvail(IQueue<T>& data) = 0;
        virtual std::optional<T> tryPop() = 0;
        virtual bool tryPopAvail(IQueue<T>& data) = 0;
    };
    template <typename T, size_t N>
    struct MessageQueueImpl;
    template <typename T, size_t N>
    static void createMessageQueue(IPolymorphic<MessageQueue<T, N>>& queue);
    template <typename T, size_t N>
    static constexpr AllocData messageQueueAllocData();

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

private:
    // --- Singleton instance ---
    static const OSAL& instance();
};

#define OSAL_TIMER              StaticPolymorphicStore<OSAL::Timer, OSAL::timerAllocData()>
#define OSAL_MUTEX              StaticPolymorphicStore<OSAL::Mutex, OSAL::mutexAllocData()>
#define OSAL_THREAD             StaticPolymorphicStore<OSAL::Thread, OSAL::threadAllocData()>
#define OSAL_CYCLIC_THREAD      StaticPolymorphicStore<OSAL::CyclicThread, OSAL::cyclicThreadAllocData()>
#define OSAL_MESSAGE_QUEUE      StaticPolymorphicStore<OSAL::CyclicThread, OSAL::cyclicThreadAllocData()>

#define OSAL_TIMER_DYN          DynamicPolymorphic<OSAL::Timer>
#define OSAL_MUTEX_DYN          DynamicPolymorphic<OSAL::Mutex>
#define OSAL_THREAD_DYN         DynamicPolymorphic<OSAL::Thread>
#define OSAL_CYCLIC_THREAD_DYN  DynamicPolymorphic<OSAL::CyclicThread>
#define OSAL_MESSAGE_QUEUE_DYN  DynamicPolymorphic<OSAL::MessageQueue>