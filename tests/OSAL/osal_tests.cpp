#include "EmbedATK/EmbedATK.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}

TEST(OSAL, htons)
{
    auto result = OSAL::hostToNetwork(0x2431);
    EXPECT_EQ(result, 0x3124);
}

TEST(OSAL, ntohs)
{
    auto result = OSAL::networkToHost(0x3124);
    EXPECT_EQ(result, 0x2431);
}

TEST(OSAL, monotonicTime)
{
    auto time1 = OSAL::monotonicTime();
    auto time2 = OSAL::monotonicTime();
    EXPECT_GE(time2, time1);
}

TEST(OSAL, currentTime)
{
    auto ts = OSAL::currentTime();
    EXPECT_GT(ts.year, 2024);
}

TEST(OSAL, sleep)
{
    const uint64_t sleep_us = 100000; // 100ms
    auto start = OSAL::monotonicTime();
    OSAL::sleep(sleep_us);
    auto end = OSAL::monotonicTime();
    auto diff = end - start;
    EXPECT_GE(diff, sleep_us);
    EXPECT_NEAR(diff, sleep_us, 40000); // 40ms
}

TEST(OSAL, sleepUntil)
{
    const uint64_t sleep_us = 100000; // 100ms
    auto start = OSAL::monotonicTime();
    auto future_time = start + sleep_us;
    OSAL::sleepUntil(future_time);
    auto end = OSAL::monotonicTime();
    EXPECT_GE(end, future_time);
    EXPECT_NEAR(end, future_time, 40000); // 40ms
}

TEST(OSAL, Timer)
{
    DynamicPolymorphic<OSAL::Timer> timer;
    ASSERT_FALSE(timer);
    OSAL::createTimer(timer);
    ASSERT_TRUE(timer);

    timer.get()->start(100000); // 100ms
    EXPECT_FALSE(timer.get()->isExpired());

    OSAL::sleep(150000); // 150ms
    EXPECT_TRUE(timer.get()->isExpired());
}

TEST(OSAL, Mutex)
{
    DynamicPolymorphic<OSAL::Mutex> mutex;
    OSAL::createMutex(mutex);
    ASSERT_TRUE(mutex);

    int counter = 0;
    const int num_threads = 10;
    std::vector<DynamicPolymorphic<OSAL::Thread>> threads(num_threads);

    for (auto& thread : threads)
    {
        OSAL::createThread(thread, {}, [&]() {
            mutex.get()->lock();
            counter++;
            mutex.get()->unlock();
        });
    }

    for (auto& thread : threads)
    {
        thread.get()->start();
    }

    OSAL::sleep(100);

    for (auto& thread : threads)
    {
        thread.get()->shutdown();
    }

    EXPECT_EQ(counter, num_threads);
}

TEST(OSAL, Thread)
{
    std::atomic<bool> executed = false;
    StaticPolymorphicStore<OSAL::Thread, OSAL::threadAllocData()> thread;
    OSAL::createThread(thread, {}, [&]() {
        executed = true;
    });
    ASSERT_TRUE(thread);

    thread.get()->start();
    thread.get()->shutdown();

    EXPECT_TRUE(executed);
}

TEST(OSAL, CyclicThread)
{
    std::atomic<int> counter = 0;
    DynamicPolymorphic<OSAL::CyclicThread> cyclicThread;
    std::array<std::byte, 16384> stack;
    OSAL::createCyclicThread(cyclicThread, stack, [&]() {
        counter++;
    });
    ASSERT_TRUE(cyclicThread);

    EXPECT_FALSE(cyclicThread.get()->isRunning());

    cyclicThread.get()->start(10000); // 10ms cycle
    EXPECT_TRUE(cyclicThread.get()->isRunning());

    OSAL::sleep(55000); // Sleep for ~5 cycles

    EXPECT_TRUE(cyclicThread.get()->isRunning());
    cyclicThread.get()->shutdown();
    EXPECT_FALSE(cyclicThread.get()->isRunning());

    EXPECT_GE(counter, 4);
    EXPECT_LE(counter, 6);
}
