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
    OSAL::StaticImpl::Timer timer;
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
    OSAL::DynamicImpl::Mutex mutex;
    OSAL::createMutex(mutex);
    ASSERT_TRUE(mutex);

    int counter = 0;
    const int num_threads = 10;
    std::vector<OSAL::DynamicImpl::Thread> threads(num_threads);

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
    OSAL::StaticImpl::Thread thread;
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
    OSAL::StaticImpl::CyclicThread cyclicThread;
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

TEST(OSAL, MessageQueue_PushPop)
{
    constexpr size_t QUEUE_SIZE = 16;
    StaticObjectStore<SboAny, QUEUE_SIZE> store;
    OSAL::StaticImpl::MessageQueue queue;
    OSAL::createMessageQueue(queue, store);
    ASSERT_TRUE(queue);

    EXPECT_TRUE(queue.get()->empty());

    // Push a value
    int test_val = 42;
    EXPECT_TRUE(queue.get()->push(SboAny(std::in_place_type<int>, test_val)));
    EXPECT_FALSE(queue.get()->empty());

    // Pop the value
    auto result = queue.get()->pop();
    ASSERT_TRUE(result.has_value());
    int popped_val = sbo_any_cast<int>(*result);
    EXPECT_EQ(popped_val, test_val);
    EXPECT_TRUE(queue.get()->empty());
}

TEST(OSAL, MessageQueue_TryPop)
{
    constexpr size_t QUEUE_SIZE = 16;
    StaticObjectStore<SboAny, QUEUE_SIZE> store;
    OSAL::StaticImpl::MessageQueue queue;
    OSAL::createMessageQueue(queue, store);
    ASSERT_TRUE(queue);

    // TryPop on empty queue
    auto result = queue.get()->tryPop();
    EXPECT_FALSE(result.has_value());

    // Push a value
    double test_val = 123.345;
    queue.get()->push(SboAny(std::in_place_type<double>, test_val));

    // TryPop on non-empty queue
    result = queue.get()->tryPop();
    ASSERT_TRUE(result.has_value());
    double popped_val = sbo_any_cast<double>(*result);
    EXPECT_EQ(popped_val, test_val);
    EXPECT_TRUE(queue.get()->empty());
}

TEST(OSAL, MessageQueue_BlockingPop)
{
    constexpr size_t QUEUE_SIZE = 16;
    StaticObjectStore<SboAny, QUEUE_SIZE> store;
    OSAL::StaticImpl::MessageQueue queue;
    OSAL::createMessageQueue(queue, store);
    ASSERT_TRUE(queue);

    std::atomic<bool> popped = false;
    int test_val = 123;
    int popped_val = 0;

    OSAL::StaticImpl::Thread popThread;
    OSAL::createThread(popThread, {}, [&]() {
        auto result = queue.get()->pop(); // This should block
        popped_val = sbo_any_cast<int>(*result);
        popped = true;
    });

    popThread.get()->start();

    // Give the pop thread time to start and block
    OSAL::sleep(20000); // 20ms
    EXPECT_FALSE(popped);

    // Push a value to unblock the other thread
    queue.get()->push(SboAny(std::in_place_type<int>, test_val));

    // Wait for the pop thread to finish
    popThread.get()->shutdown();

    EXPECT_TRUE(popped);
    EXPECT_EQ(popped_val, test_val);
}

TEST(OSAL, MessageQueue_PushManyPopAvail)
{
    constexpr size_t QUEUE_SIZE = 16;
    constexpr size_t NUM_MSGS = 5;
    StaticObjectStore<SboAny, QUEUE_SIZE> store;
    OSAL::StaticImpl::MessageQueue queue;
    OSAL::createMessageQueue(queue, store);
    ASSERT_TRUE(queue);

    // Prepare messages to push
    StaticObjectStore<SboAny, NUM_MSGS> pushStore;
    StaticQueueView<SboAny> pushQueue(pushStore);
    for(size_t i = 0; i < NUM_MSGS; ++i) {
        pushQueue.push(SboAny(std::in_place_type<int>, static_cast<int>(i)));
    }

    // Push many
    EXPECT_TRUE(queue.get()->pushMany(std::move(pushQueue)));
    EXPECT_TRUE(pushQueue.empty()); // pushMany should move the items

    // Pop avail
    StaticObjectStore<SboAny, QUEUE_SIZE> popStore;
    StaticQueueView<SboAny> popQueue(popStore);
    EXPECT_TRUE(queue.get()->popAvail(popQueue));
    EXPECT_TRUE(queue.get()->empty());
    EXPECT_EQ(popQueue.size(), NUM_MSGS);

    // Verify popped messages
    for(size_t i = 0; i < NUM_MSGS; ++i) {
        auto val = sbo_any_cast<int>(popQueue[i]);
        EXPECT_EQ(val, i);
    }
}