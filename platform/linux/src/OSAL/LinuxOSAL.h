// #pragma once

// #if defined(EATK_PLATFORM_LINUX)

// #include "OSAL/StdOSAL.h"

// #include <arpa/inet.h>

// // --- Thread ---
// bool LinuxThread::start()
// {
//     pthread_attr_t attr;
//     pthread_attr_init(&attr);

//     if (!m_stack.empty()) {
//         int res = pthread_attr_setstack(&attr, m_stack.data(), m_stack.size());
//         if (res != 0)
//             return false;
//     }

//     int status = pthread_create(&m_thread, &attr, &LinuxThread::taskWrapper, this);
//     pthread_attr_destroy(&attr);

//     return status == 0;
// }

// void LinuxThread::shutdown()
// {
//     if (m_thread) {
//         if (pthread_join(m_thread, nullptr) != 0)
//             pthread_cancel(m_thread);
//         m_thread = 0;
//     }
// }

// bool LinuxThread::setPriority(int prio, int policy)
// {
//     sched_param sch_params;
//     sch_params.sched_priority = prio;
//     return pthread_setschedparam(m_thread, policy, &sch_params) == 0;
// }

// void* LinuxThread::taskWrapper(void* context)
// {
//     auto self = static_cast<LinuxThread*>(context);
//     if (self && self->m_task) {
//         self->m_task();
//     }
//     return nullptr;
// }

// // --- Cyclic Thread ---
// bool LinuxCyclicThread::start(uint64_t cycleTime_us)
// {
//     m_cycleTime_us = cycleTime_us;

//     pthread_attr_t attr;
//     pthread_attr_init(&attr);

//     if (!m_stack.empty()) {
//         if (pthread_attr_setstack(&attr, m_stack.data(), m_stack.size()) != 0)
//             return false;
//     }

//     sem_init(&m_started, 0, 0);
//     sem_init(&m_shutdown, 0, 0);

//     int status = pthread_create(&m_thread, &attr, &LinuxCyclicThread::cyclicTaskWrapper, this);
//     pthread_attr_destroy(&attr);

//     if (status != 0)
//         return false;

//     sem_wait(&m_started);
//     return true;
// }

// void LinuxCyclicThread::shutdown()
// {
//     m_running = false;
//     sem_post(&m_shutdown);

//     if (m_thread) {
//         if (pthread_join(m_thread, nullptr) != 0)
//             pthread_cancel(m_thread);
//         m_thread = 0;
//     }

//     sem_destroy(&m_started);
//     sem_destroy(&m_shutdown);
// }

// bool LinuxCyclicThread::setPriority(int prio, int policy)
// {
//     sched_param sch_params;
//     sch_params.sched_priority = prio;
//     return pthread_setschedparam(m_thread, policy, &sch_params) == 0;
// }

// bool LinuxCyclicThread::isRunning() const { return m_running; }

// void* LinuxCyclicThread::cyclicTaskWrapper(void* context)
// {
//     auto self = static_cast<LinuxCyclicThread*>(context);
//     if (!self || !self->m_cyclicTask)
//         return nullptr;

//     self->m_running = true;
//     sem_post(&self->m_started);

//     timespec nextCycle{};
//     clock_gettime(CLOCK_MONOTONIC, &nextCycle);
    
//     while (self->m_running) {
//         uint64_t next_ns = (static_cast<uint64_t>(nextCycle.tv_sec) * 1000000000 + static_cast<uint64_t>(nextCycle.tv_nsec)) + (self->m_cycleTime_us * 1000);
//         nextCycle.tv_sec = next_ns / 1000000000;
//         nextCycle.tv_nsec = next_ns % 1000000000;

//         self->m_cyclicTask();
//         if (!self->m_running) break;

//         while (sem_timedwait(&self->m_shutdown, &nextCycle) != 0 && errno == EINTR);
//     }
//     return nullptr;
// }

// class LinuxOSAL : public StdOSAL
// {
// private:
//     // --- Printing ---
//     void setConsoleColorImpl(ConsoleColor col) const override
//     {
//         switch(col)
//         {
//             case ConsoleColor::Green:
//                 print("\033[32m");
//                 break;
//             case ConsoleColor::Yellow:
//                 print("\033[33m");
//                 break;
//             case ConsoleColor::Red:
//                 print("\033[31m");
//                 break;
//             case ConsoleColor::Cyan:
//                 print("\033[36m");
//                 break;
//             default:
//                 print("\033[0m");
//                 break;
//         };
//     }

//     // --- Thread ---
//     void createThreadImpl(IPolymorphic<OSAL::Thread>& thread) const override { thread.construct<LinuxThread>(); }

//     // --- Cyclic Thread ---
//     void createCyclicThreadImpl(IPolymorphic<OSAL::CyclicThread>& cyclicThread) const override { cyclicThread.construct<LinuxCyclicThread>(); }
// };

// #endif