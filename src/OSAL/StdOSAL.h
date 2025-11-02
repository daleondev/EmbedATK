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

bool StdMessageQueue::push(SboAny&& msg) 
{ 
    if constexpr (std::is_move_constructible_v<SboAny>) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.full())
                return false;

            m_queue.push(std::move(msg)); 
        }

        m_condition.notify_one();
        return true; 
    }
    else {
        return false;
    }
}

bool StdMessageQueue::pushMany(IQueue<SboAny>&& data)
{
    if constexpr (std::is_move_constructible_v<SboAny> && std::is_move_assignable_v<SboAny>) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.size() + data.size() > m_queue.capacity())
                return false;

            m_queue.insert(
                m_queue.end(), 
                std::make_move_iterator(data.begin()),
                std::make_move_iterator(data.end())
            );
            data.clear();
        }

        m_condition.notify_all();
        return true;
    }
    else {
        return false;
    }
}
std::optional<SboAny> StdMessageQueue::pop()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_queue.empty()) {
        m_condition.wait(lock, [this]() { return !m_queue.empty(); });
        if (m_queue.empty())
            return {};
    }

    return m_queue.pop();
}
bool StdMessageQueue::popAvail(IQueue<SboAny>& data)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_queue.empty()) {
        m_condition.wait(lock, [this]() { return !m_queue.empty(); });
        if (m_queue.empty())
            return false;
    }

    if (m_queue.size() > data.capacity()) {
        data.insert(data.begin(), std::move_iterator(m_queue.begin()), std::move_iterator(m_queue.begin()+data.capacity()));
        m_queue.erase(0, data.capacity());
    }
    else {
        data.insert(data.begin(), std::move_iterator(m_queue.begin()), std::move_iterator(m_queue.end()));
        m_queue.clear();
    }
    return true;
}
std::optional<SboAny> StdMessageQueue::tryPop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_queue.pop();
}
bool StdMessageQueue::tryPopAvail(IQueue<SboAny>& data)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_queue.empty())
        return false;

    if (m_queue.size() > data.capacity()) {
        data.insert(data.begin(), std::move_iterator(m_queue.begin()), std::move_iterator(m_queue.begin()+data.capacity()));
        m_queue.erase(0, data.capacity());
    }
    else {
        data.insert(data.begin(), std::move_iterator(m_queue.begin()), std::move_iterator(m_queue.end()));
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
    void createMessageQueueImpl(IPolymorphic<OSAL::MessageQueue>& queue, IObjectStore<SboAny>& store) const override 
    { 
        queue.construct<StdMessageQueue>(store); 
    }
};