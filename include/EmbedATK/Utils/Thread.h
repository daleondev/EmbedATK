#pragma once

#include "EmbedATK/OSAL/OSAL.h"

namespace Utils {

    using TaskFunction = void(*)();

    template<typename T>
    inline constexpr bool is_task_v = std::invocable<T> && std::is_same_v<std::invoke_result_t<T>, void>;

    template<IsStaticPolymorphic Thread, size_t StackSize, int Prio, auto Task = nullptr, uint64_t CycleTime_us = 0>
    requires
        (Task == nullptr || (is_task_v<decltype(Task)>)) &&
        (
            (std::is_base_of_v<IPolymorphic<OSAL::Thread>, Thread> && CycleTime_us == 0) ||
            (std::is_base_of_v<IPolymorphic<OSAL::CyclicThread>, Thread> && CycleTime_us > 0)
        )
    struct StaticThread
    {
        inline static constexpr int PRIO = Prio;
        inline static constexpr bool IS_CYCLIC = std::is_base_of_v<IPolymorphic<OSAL::CyclicThread>, Thread>;
        inline static constexpr uint64_t CYCLE_TIME_US = CycleTime_us;
        inline static constexpr TaskFunction TASK = Task;
        
        StaticBuffer<StackSize, alignof(std::max_align_t)> stackBuff;
        Thread thread;
    };

    template <typename T>
    struct is_static_thread : std::false_type {};

    template<IsStaticPolymorphic Thread, size_t StackSize, int Prio, auto Task, uint64_t CycleTime_us>
    struct is_static_thread<Utils::StaticThread<Thread, StackSize, Prio, Task, CycleTime_us>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_static_thread_v = is_static_thread<T>::value;

    template <typename T>
    concept IsStaticThread = is_static_thread_v<T>;

    template<IsStaticThread Thread>
    constexpr void setupStaticThread(Thread& thread, bool autoStart = false)
    {
        static_assert(thread.TASK != nullptr, "Thread has no assigned task");

        if constexpr (thread.IS_CYCLIC) {

            OSAL::createCyclicThread(
                thread.thread,
                thread.PRIO,
                thread.stackBuff,
                thread.TASK
            );

            if (autoStart) {
                thread.thread.get()->start(thread.CYCLE_TIME_US);
            }

        } else {
            
            OSAL::createThread(
                thread.thread,
                thread.PRIO,
                thread.stackBuff,
                thread.TASK
            );

            if (autoStart) {
                thread.thread.get()->start();
            }

        }
    }

    template<class Callable, class... Args, IsStaticThread Thread>
    constexpr void setupStaticThread(Thread& thread, bool autoStart, Callable &&func, Args &&...args)
    {
        static_assert(thread.TASK == nullptr, "Thread already has assigned task");

        if constexpr (thread.IS_CYCLIC) {

            OSAL::createCyclicThread(
                thread.thread,
                thread.PRIO,
                thread.stackBuff,
                std::forward<Callable>(func), 
                std::forward<Args>(args)...
            );

            if (autoStart) {
                thread.thread.get()->start(thread.CYCLE_TIME_US);
            }

        } else {
            
            OSAL::createThread(
                thread.thread,
                thread.PRIO,
                thread.stackBuff,
                std::forward<Callable>(func), 
                std::forward<Args>(args)...
            );

            if (autoStart) {
                thread.thread.get()->start();
            }

        }
    }

    template<IsStaticThread Thread>
    constexpr void shutdownStaticThread(Thread& thread)
    {
        thread.thread.get()->shutdown();
    }

}