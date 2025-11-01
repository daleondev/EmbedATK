#pragma once

#include "EmbedATK/OSAL/osal_platform_std.h"

class LinuxThread : public OSAL::Thread
{
private:
    bool start() override;
    void shutdown() override;
    bool setPriority(int prio, int policy) override;
    static void* taskWrapper(void* context);

    pthread_t m_thread;
    
    friend class LinuxOSAL;
};
constexpr AllocData OSAL::threadAllocData() { return allocData<LinuxThread>(); }

class LinuxCyclicThread : public OSAL::CyclicThread
{
private:
    bool start(uint64_t cycleTime_us) override;
    void shutdown() override;
    bool setPriority(int prio, int policy) override;
    bool isRunning() const override;
    static void* cyclicTaskWrapper(void* context);

    std::atomic_bool m_running;
    sem_t m_started;
    sem_t m_shutdown;
    uint64_t m_cycleTime_us;

    pthread_t m_thread;

    friend class LinuxOSAL;
};
constexpr AllocData OSAL::cyclicThreadAllocData() { return allocData<LinuxCyclicThread>(); }