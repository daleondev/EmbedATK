#pragma once

#include <chrono>
#include <future>
#include <bit>

// --- Timer ---
void StdTimer::start(uint64_t timeout_us) { m_stopTime = std::chrono::steady_clock::now() + std::chrono::microseconds(timeout_us); }
bool StdTimer::isExpired() const { return std::chrono::steady_clock::now() >= m_stopTime; }

// --- Mutex ---
void StdMutex::lock() { m_mutex.lock(); }
void StdMutex::unlock() { m_mutex.unlock(); }

// --- Thread ---
bool StdThread::start()
{
    m_thread = std::thread(m_task);
    return true;
}
void StdThread::shutdown()
{
    if (m_thread.joinable())
        m_thread.join();
}

// --- Cyclic Thread ---
bool StdCyclicThread::start(uint64_t cycleTime_us)
{
    std::promise<void> promise;
    auto future = promise.get_future();
    m_thread = std::thread([this, cycleTime_us, &promise]() -> void {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        m_running = true;
        promise.set_value();

        auto nextCycle = std::chrono::steady_clock::now();
        while (m_running) {
            nextCycle += std::chrono::microseconds(cycleTime_us);

            lock.unlock();
            m_cyclicTask();
            lock.lock();

            m_condition.wait_until(lock, nextCycle, [this]() { return !m_running; });
        }
    });
    future.wait();
    return true;
}
void StdCyclicThread::shutdown()
{
    m_running = false;
    m_condition.notify_all();
    if (m_thread.joinable())
        m_thread.join();
}
bool StdCyclicThread::isRunning() const { return m_running; }

// --- Message-Queue ---
bool StdMessageQueue::empty() const 
{  
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

bool StdMessageQueue::push(MsgType&& msg) 
{ 
    if constexpr (std::is_move_constructible_v<MsgType>) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.full())
                return false;

            static constexpr auto alloc = allocData<MsgType>();
            auto* ptr = static_cast<MsgType*>(m_pool.allocate(alloc.size, alloc.align));
            std::construct_at(ptr, std::move(msg));
            m_queue.push(ptr); 
        }

        m_condition.notify_one();
        return true; 
    }
    else {
        return false;
    }
}

bool StdMessageQueue::pushMany(IQueue<MsgType>&& data)
{
    if constexpr (std::is_move_constructible_v<MsgType> && std::is_move_assignable_v<MsgType>) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.size() + data.size() > m_queue.capacity())
                return false;

            static constexpr auto alloc = allocData<MsgType>();
            for (auto& msg : data) {
                auto* ptr = static_cast<MsgType*>(m_pool.allocate(alloc.size, alloc.align));
                std::construct_at(ptr, std::move(msg));
                m_queue.push(ptr); 
            }
        }

        m_condition.notify_all();
        data.clear();
        return true;
    }
    else {
        return false;
    }
}
std::optional<OSAL::MessageQueue::MsgType> StdMessageQueue::pop()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_queue.empty()) {
        m_condition.wait(lock, [this]() { return !m_queue.empty(); });
        if (m_queue.empty())
            return {};
    }

    auto ptr = m_queue.pop();
    if (!ptr.has_value()) return std::nullopt;

    std::optional<MsgType> msg = std::move(*ptr.value());
    static constexpr auto alloc = allocData<MsgType>();
    m_pool.deallocate(ptr.value(), alloc.size, alloc.align);
    return msg;
}
bool StdMessageQueue::popAvail(IQueue<MsgType>& data)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_queue.empty()) {
        m_condition.wait(lock, [this]() { return !m_queue.empty(); });
        if (m_queue.empty())
            return false;
    }

    static constexpr auto alloc = allocData<MsgType>();
    if (m_queue.size() > data.capacity()) {
        for (size_t i = 0; i < data.capacity(); ++i) {
            auto* ptr = m_queue[i];
            data.push(std::move(*ptr));
            m_pool.deallocate(ptr, alloc.size, alloc.align);
        }
        m_queue.erase(0, data.capacity());
    }
    else {
        for (auto* ptr : m_queue) {
            data.push(std::move(*ptr));
            m_pool.deallocate(ptr, alloc.size, alloc.align);
        }
        m_queue.clear();
    }
    return true;
}
std::optional<OSAL::MessageQueue::MsgType> StdMessageQueue::tryPop()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    auto ptr = m_queue.pop();
    if (!ptr.has_value()) return std::nullopt;

    std::optional<MsgType> result = std::move(*ptr.value());
    static constexpr auto alloc = allocData<MsgType>();
    m_pool.deallocate(ptr.value(), alloc.size, alloc.align);
    return result;
}
bool StdMessageQueue::tryPopAvail(IQueue<MsgType>& data)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_queue.empty())
        return false;

    static constexpr auto alloc = allocData<MsgType>();
    if (m_queue.size() > data.capacity()) {
        for (size_t i = 0; i < data.capacity(); ++i) {
            auto* ptr = m_queue[i];
            data.push(std::move(*ptr));
            m_pool.deallocate(ptr, alloc.size, alloc.align);
        }
        m_queue.erase(0, data.capacity());
    }
    else {
        for (auto* ptr : m_queue) {
            data.push(std::move(*ptr));
            m_pool.deallocate(ptr, alloc.size, alloc.align);
        }
        m_queue.clear();
    }
    return true;
}

class StdOSAL : public OSAL
{
private:
    // --- Network ---
    uint16_t hostToNetworkImpl(uint16_t h) const override 
    {
        if constexpr (std::endian::native == std::endian::little) {
            return std::byteswap(h);
        } else {
            return h;
        }
    };
    uint16_t networkToHostImpl(uint16_t n) const override 
    { 
        if constexpr (std::endian::native == std::endian::little) {
            return std::byteswap(n);
        } else {
            return n;
        }
    };

    // --- Printing ---
    void printImpl(const char* msg) const override { std::cout << msg; }
    void printlnImpl(const char* msg) const override { std::cout << msg << '\n'; }
    void eprintImpl(const char* emsg) const override { std::cerr << emsg; }
    void eprintlnImpl(const char* emsg) const override { std::cerr << emsg << '\n'; }

    // --- Time ---
    uint64_t monotonicTimeImpl() const override 
    {
        const auto time = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now());
        return time.time_since_epoch().count();
    }

    Timestamp currentTimeImpl() const override 
    {
        const auto zone = std::chrono::current_zone();
        const auto now = std::chrono::system_clock::now();
        const auto localTime = std::chrono::zoned_time(zone, now).get_local_time();
        const auto startOfDay = std::chrono::floor<std::chrono::days>(localTime);
        const auto timeOfDay = localTime - startOfDay;

        const auto date = std::chrono::year_month_day(startOfDay);
        const auto time = std::chrono::hh_mm_ss(timeOfDay);

        Timestamp ts;
        ts.year        = static_cast<uint16_t>(static_cast<int>(date.year()));
        ts.month       = static_cast<uint8_t>(static_cast<unsigned int>(date.month()));
        ts.day         = static_cast<uint8_t>(static_cast<unsigned int>(date.day()));
        ts.hour        = static_cast<uint8_t>(time.hours().count());
        ts.minute      = static_cast<uint8_t>(time.minutes().count());
        ts.second      = static_cast<uint8_t>(time.seconds().count());
        ts.millisecond = static_cast<uint16_t>(std::chrono::duration_cast<std::chrono::milliseconds>(time.subseconds()).count());
        return ts;
    }

    bool sleepImpl(uint64_t us) const override  
    {
        const auto duration = std::chrono::microseconds(us);
        std::this_thread::sleep_for(duration); 
        return true;
    }

    bool sleepUntilImpl(uint64_t monotonic) const override 
    {
        const auto timepoint = std::chrono::steady_clock::time_point(std::chrono::microseconds(monotonic));
        std::this_thread::sleep_until(timepoint); 
        return true;
    }

    // --- Timer ---
    void createTimerImpl(IPolymorphic<OSAL::Timer>& timer) const override { timer.construct<StdTimer>(); }

    // --- Mutex ---
    void createMutexImpl(IPolymorphic<OSAL::Mutex>& mutex) const override { mutex.construct<StdMutex>(); }

    // --- Message Queue ---
    void createMessageQueueImpl(IPolymorphic<OSAL::MessageQueue>& queue, IObjectStore<OSAL::MessageQueue::MsgType*>& store, IPool& pool) const override 
    { 
        queue.construct<StdMessageQueue>(store, pool); 
    }
};