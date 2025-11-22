#if defined(EATK_PLATFORM_ARM)

#include "../../src/pch.h"
#include "ArmOSAL.h"

#include "EmbedATK/Core/Assert.h"

#include <tx_api.h>
#include <stm32h7xx_hal.h>

extern "C" {
    extern RTC_HandleTypeDef* g_rtc;
}

// --- Mutex ---
ArmMutex::ArmMutex()
{
    static std::atomic_size_t id = 0;
    m_id = id++;

    CHAR name[32];
    snprintf(name, sizeof(name), "Mutex %zu", m_id);
    UINT status = tx_mutex_create(&m_mutex, name, TX_NO_INHERIT);

    EATK_ASSERT(status == TX_SUCCESS, "failed to create Mutex");
}
ArmMutex::~ArmMutex()
{
    tx_mutex_delete(&m_mutex);
}
void ArmMutex::lock() { while(tx_mutex_get(&m_mutex, TX_WAIT_FOREVER) != TX_SUCCESS); }
void ArmMutex::unlock() { tx_mutex_put(&m_mutex); }

// --- Thread ---
ArmThread::~ArmThread()
{
    tx_thread_delete(&m_thread);
    tx_semaphore_delete(&m_taskDone);
}
bool ArmThread::start()
{
    static std::atomic_size_t s_id = 0;
    const size_t id = s_id++;

    CHAR name[32];
    snprintf(name, sizeof(name), "Task Done Sem %zu", id);
    tx_semaphore_create(&m_taskDone, name, 0);

    snprintf(m_name, sizeof(m_name), "Thread %zu", id);
    UINT status = tx_thread_create(&m_thread, m_name,
        taskWrapper, reinterpret_cast<uintptr_t>(this),
        m_stack.data(), m_stack.size(),
        m_prio, m_prio, TX_NO_TIME_SLICE, TX_AUTO_START);

    return status == TX_SUCCESS;
}
void ArmThread::shutdown()
{
    if (tx_semaphore_get(&m_taskDone, 1000) != TX_SUCCESS) {
        tx_thread_terminate(&m_thread);
    }
}
bool ArmThread::setPriority(int prio, int)
{
    UINT prevPrio = m_prio;
    if (tx_thread_priority_change(&m_thread, prio, &prevPrio) != TX_SUCCESS)
        return false;
    m_prio = prio;
    return true;
}
VOID ArmThread::taskWrapper(ULONG context)
{
    auto self = reinterpret_cast<ArmThread*>(static_cast<uintptr_t>(context));
    if (self && self->m_task) {
        self->m_task();
        tx_semaphore_put(&self->m_taskDone);
    }
}

// --- Cyclic Thread ---
ArmCyclicThread::~ArmCyclicThread()
{
    tx_thread_delete(&m_thread);
    tx_semaphore_delete(&m_started);
    tx_semaphore_delete(&m_shutdown);
    tx_semaphore_delete(&m_taskDone);
}
bool ArmCyclicThread::start(uint64_t cycleTime_us)
{
    static std::atomic_size_t s_id = 0;
    const size_t id = s_id++;

    m_cycleTime_us = cycleTime_us;

    CHAR name[64];
    snprintf(name, sizeof(name), "Cyclic Task Done Sem %zu", id);
    if (tx_semaphore_create(&m_taskDone, name, 0) != TX_SUCCESS)
        return false;

    snprintf(name, sizeof(name), "Thread started Semaphore %zu", id);
    if (tx_semaphore_create(&m_started, name, 0) != TX_SUCCESS)
        return false;

    snprintf(name, sizeof(name), "Shutdown thread Semaphore %zu", id);
    if (tx_semaphore_create(&m_shutdown, name, 0) != TX_SUCCESS)
        return false;

    snprintf(m_name, sizeof(m_name), "Cyclic Thread %zu", id);
    UINT status = tx_thread_create(&m_thread, m_name,
        cyclicTaskWrapper, reinterpret_cast<uintptr_t>(this),
        m_stack.data(), m_stack.size(),
        m_prio, m_prio, TX_NO_TIME_SLICE, TX_AUTO_START);

    if (status != TX_SUCCESS)
        return false;

    tx_semaphore_get(&m_started, TX_WAIT_FOREVER);
    return true;
}
void ArmCyclicThread::shutdown()
{
    m_running = false;
    tx_semaphore_put(&m_shutdown);

    if (tx_semaphore_get(&m_taskDone, 1000) != TX_SUCCESS) {
        tx_thread_terminate(&m_thread);
    }
}
bool ArmCyclicThread::setPriority(int prio, int)
{
    UINT prevPrio = m_prio;
    if (tx_thread_priority_change(&m_thread, prio, &prevPrio) != TX_SUCCESS)
        return false;
    m_prio = prio;
    return true;
}
bool ArmCyclicThread::isRunning() const { return m_running; }
VOID ArmCyclicThread::cyclicTaskWrapper(ULONG context)
{
    auto self = reinterpret_cast<ArmCyclicThread*>(static_cast<uintptr_t>(context));
    if (!self || !self->m_cyclicTask)
        return;

    self->m_running = true;
    tx_semaphore_put(&self->m_started);

    constexpr auto tick_us = 1000000 / TX_TIMER_TICKS_PER_SECOND;
    const auto ticks = self->m_cycleTime_us / tick_us;
    if (ticks == 0) {
        self->m_running = false;
        tx_semaphore_put(&self->m_taskDone);
        return;
    }

    ULONG nextCycleTicks = tx_time_get();
    while (self->m_running) {
        nextCycleTicks += ticks;

        self->m_cyclicTask();
        if (!self->m_running) break;

        ULONG currentTicks = tx_time_get();
        ULONG delayTicks;

        if (nextCycleTicks <= currentTicks) {
            delayTicks = TX_NO_WAIT; // 0
        } else {
            delayTicks = nextCycleTicks - currentTicks;
        }

        UINT status = tx_semaphore_get(&self->m_shutdown, delayTicks);
        if (status != TX_NO_INSTANCE) break;
    }

    self->m_running = false;
    tx_semaphore_put(&self->m_taskDone);
}

// --- Message-Queue ---
ArmMessageQueue::ArmMessageQueue(IObjectStore<MsgType*>& store, IPool& pool)
    : m_pool(pool), m_store(store)
{
    static std::atomic_size_t id = 0;
    m_id = id++;

    constexpr auto MSG_SIZE = sizeof(MsgType*);

    CHAR name[32];
    snprintf(name, sizeof(name), "Message Queue %zu", m_id);
    UINT status = tx_queue_create(&m_queue,
        name,
        MSG_SIZE / sizeof(ULONG),
        m_store.data(),
        m_store.size() * MSG_SIZE);

    EATK_ASSERT(status == TX_SUCCESS, "failed to create queue");
}

ArmMessageQueue::~ArmMessageQueue()
{
    tx_queue_delete(&m_queue);
}

bool ArmMessageQueue::empty() const 
{  
    ULONG enqueued = 0;
    tx_queue_info_get(const_cast<TX_QUEUE*>(&m_queue), NULL, &enqueued, NULL, NULL, NULL, NULL);
    return enqueued == 0;
}

bool ArmMessageQueue::push(MsgType&& msg) 
{ 
    if constexpr (std::is_move_constructible_v<MsgType>) {
        static constexpr auto alloc = allocData<MsgType>();
        auto* ptr = static_cast<MsgType*>(m_pool.allocate(alloc.size, alloc.align));
        std::construct_at(ptr, std::move(msg));
        return tx_queue_send(&m_queue, &ptr, TX_NO_WAIT) == TX_SUCCESS; // CHECK IF WORKS
    }
    else {
        return false;
    }
}

bool ArmMessageQueue::push(const MsgType& msg) 
{ 
    if constexpr (std::is_copy_constructible_v<MsgType>) {
        static constexpr auto alloc = allocData<MsgType>();
        auto* ptr = static_cast<MsgType*>(m_pool.allocate(alloc.size, alloc.align));
        std::construct_at(ptr, msg);
        return tx_queue_send(&m_queue, &ptr, TX_NO_WAIT) == TX_SUCCESS; // CHECK IF WORKS
    }
    else {
        return false;
    }
}

bool ArmMessageQueue::pushMany(IQueue<MsgType>&& data)
{
    if constexpr (std::is_move_constructible_v<MsgType> && std::is_move_assignable_v<MsgType>) {
        static constexpr auto alloc = allocData<MsgType>();
        for (auto& msg : data) {
            auto* ptr = static_cast<MsgType*>(m_pool.allocate(alloc.size, alloc.align));
            std::construct_at(ptr, std::move(msg));
            if (tx_queue_send(&m_queue, &ptr, TX_NO_WAIT) != TX_SUCCESS) {
                return false;
            }
        }
        return true;
    }
    else {
        return false;
    }
}

std::optional<OSAL::MessageQueue::MsgType> ArmMessageQueue::pop()
{
    MsgType* ptr;
    if (tx_queue_receive(&m_queue, &ptr, TX_WAIT_FOREVER) == TX_SUCCESS) {
        MsgType msg(std::move(*ptr));
        static constexpr auto alloc = allocData<MsgType>();
        m_pool.deallocate(ptr, alloc.size, alloc.align);
        return msg;
    }
    return std::nullopt;
}

bool ArmMessageQueue::popAvail(IQueue<MsgType>& data)
{
    MsgType* ptr;
    if (tx_queue_receive(&m_queue, &ptr, TX_WAIT_FOREVER) != TX_SUCCESS) {
        return false;
    }

    data.push(std::move(*ptr));
    static constexpr auto alloc = allocData<MsgType>();
    m_pool.deallocate(ptr, alloc.size, alloc.align);

    while (tx_queue_receive(&m_queue, &ptr, TX_NO_WAIT) == TX_SUCCESS) {
        data.push(std::move(*ptr));
        m_pool.deallocate(ptr, alloc.size, alloc.align);
    }

    return true;
}

std::optional<OSAL::MessageQueue::MsgType> ArmMessageQueue::tryPop()
{
    MsgType* ptr;
    if (tx_queue_receive(&m_queue, &ptr, TX_NO_WAIT) == TX_SUCCESS) {
        MsgType msg(std::move(*ptr));
        static constexpr auto alloc = allocData<MsgType>();
        m_pool.deallocate(ptr, alloc.size, alloc.align);
        return msg;
    }
    return std::nullopt;
}

bool ArmMessageQueue::tryPopAvail(IQueue<MsgType>& data)
{
    MsgType* ptr;
    if (tx_queue_receive(&m_queue, &ptr, TX_NO_WAIT) != TX_SUCCESS) {
        return false;
    }

    data.push(std::move(*ptr));
    static constexpr auto alloc = allocData<MsgType>();
    m_pool.deallocate(ptr, alloc.size, alloc.align);
    
    while (tx_queue_receive(&m_queue, &ptr, TX_NO_WAIT) == TX_SUCCESS) {
        data.push(std::move(*ptr));
        m_pool.deallocate(ptr, alloc.size, alloc.align);
    }

    return true;
}

uint16_t ArmOSAL::hostToNetworkImpl(uint16_t h) const { return h; }
uint16_t ArmOSAL::networkToHostImpl(uint16_t n) const { return n; }

void ArmOSAL::printImpl(const char* msg) const { printf("%s", msg); }
void ArmOSAL::printlnImpl(const char* msg) const { printf("%s\r\n", msg); }
void ArmOSAL::eprintImpl(const char* emsg) const { printf("%s", emsg); }
void ArmOSAL::eprintlnImpl(const char* emsg) const { printf("%s\r\n", emsg); }
void ArmOSAL::setConsoleColorImpl(ConsoleColor col) const
{
    switch(col)
    {
        case ConsoleColor::Green:
            printImpl("\033[32m");
            break;
        case ConsoleColor::Yellow:
            printImpl("\033[33m");
            break;
        case ConsoleColor::Red:
            printImpl("\033[31m");
            break;
        case ConsoleColor::Cyan:
            printImpl("\033[36m");
            break;
        default:
            printImpl("\033[0m");
            break;
    };
}

uint64_t ArmOSAL::monotonicTimeImpl() const
{
    const auto ticks = tx_time_get();
    const auto tick_us = 1000000 / TX_TIMER_TICKS_PER_SECOND;
    return ticks * tick_us;
}

Timestamp ArmOSAL::currentTimeImpl() const
{
    if (!g_rtc)
        return {};

    RTC_DateTypeDef date;
    RTC_TimeTypeDef time;

    HAL_RTC_GetDate(g_rtc, &date, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(g_rtc, &time, RTC_FORMAT_BIN);

    Timestamp ts;
    ts.year        = static_cast<uint16_t>(date.Year);
    ts.month       = static_cast<uint8_t>(date.Month);
    ts.day         = static_cast<uint8_t>(date.Date);
    ts.hour        = static_cast<uint8_t>(time.Hours);
    ts.minute      = static_cast<uint8_t>(time.Minutes);
    ts.second      = static_cast<uint8_t>(time.Seconds);
    ts.millisecond = static_cast<uint16_t>(((time.SecondFraction - time.SubSeconds) * 1000) / (time.SecondFraction + 1));
    return ts;
}

bool ArmOSAL::sleepImpl(uint64_t duration_us) const
{
    if (duration_us == 0) return true;

    constexpr auto tick_us = 1000000 / TX_TIMER_TICKS_PER_SECOND;
    const auto ticks = duration_us / tick_us;

    tx_thread_sleep(ticks);
    return true;
}

bool ArmOSAL::sleepUntilImpl(uint64_t time_us) const
{
    const auto current_us = monotonicTimeImpl();
    if (time_us > current_us)
        sleepImpl(time_us - current_us);
    return true;
}

void ArmOSAL::createTimerImpl(IPolymorphic<OSAL::Timer>& timer) const { timer.construct<Timer>(); }

void ArmOSAL::createMutexImpl(IPolymorphic<OSAL::Mutex>& mutex) const { mutex.construct<ArmMutex>(); }

void ArmOSAL::createThreadImpl(IPolymorphic<OSAL::Thread>& thread) const { thread.construct<ArmThread>(); }

void ArmOSAL::createCyclicThreadImpl(IPolymorphic<OSAL::CyclicThread>& cyclicThread) const { cyclicThread.construct<ArmCyclicThread>(); }

void ArmOSAL::createMessageQueueImpl(IPolymorphic<OSAL::MessageQueue>& queue, IObjectStore<OSAL::MessageQueue::MsgType*>& store, IPool& pool) const 
{ 
    queue.construct<ArmMessageQueue>(store, pool); 
}

#endif
