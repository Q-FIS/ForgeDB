// Contract tests for POOL-1: BoundedQueue<T>.   (owner: lead)
// Switched on when src/pool/stub_bounded_queue.cpp is deleted.
// Run these on the tsan preset too: ctest --preset tsan -R bounded_queue
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "forge/bounded_queue.hpp"

using forge::BoundedQueue;
using namespace std::chrono_literals;

TEST(BoundedQueue, CapacityMustBeAtLeastOne) {
    EXPECT_THROW(BoundedQueue<int>(0), std::invalid_argument);
}

TEST(BoundedQueue, FirstInFirstOut) {
    BoundedQueue<int> q(10);
    EXPECT_EQ(q.capacity(), 10u);
    for (int i = 0; i < 5; ++i) EXPECT_TRUE(q.push(i));
    EXPECT_EQ(q.size(), 5u);
    for (int i = 0; i < 5; ++i) EXPECT_EQ(q.pop(), i);
    EXPECT_EQ(q.size(), 0u);
}

TEST(BoundedQueue, TryPushFailsWhenFull) {
    BoundedQueue<int> q(2);
    EXPECT_TRUE(q.try_push(1));
    EXPECT_TRUE(q.try_push(2));
    EXPECT_FALSE(q.try_push(3));
    EXPECT_EQ(q.pop(), 1);
    EXPECT_TRUE(q.try_push(3));
}

TEST(BoundedQueue, WorksWithMoveOnlyItems) {
    BoundedQueue<std::unique_ptr<int>> q(2);
    EXPECT_TRUE(q.push(std::make_unique<int>(7)));
    auto item = q.pop();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(**item, 7);
}

TEST(BoundedQueue, PopWaitsForAnItem) {
    BoundedQueue<int> q(1);
    std::atomic<bool> got{false};
    std::thread consumer([&] {
        EXPECT_EQ(q.pop(), 42);
        got = true;
    });
    std::this_thread::sleep_for(100ms);
    EXPECT_FALSE(got) << "pop() must wait while the queue is empty";
    q.push(42);
    consumer.join();
    EXPECT_TRUE(got);
}

TEST(BoundedQueue, PushWaitsForRoom) {
    BoundedQueue<int> q(1);
    q.push(1);
    std::atomic<bool> pushed{false};
    std::thread producer([&] {
        EXPECT_TRUE(q.push(2));
        pushed = true;
    });
    std::this_thread::sleep_for(100ms);
    EXPECT_FALSE(pushed) << "push() must wait while the queue is full";
    EXPECT_EQ(q.pop(), 1);
    producer.join();
    EXPECT_TRUE(pushed);
    EXPECT_EQ(q.pop(), 2);
}

TEST(BoundedQueue, CloseStillHandsOutWhatIsLeft) {
    BoundedQueue<int> q(5);
    q.push(1);
    q.push(2);
    q.close();
    EXPECT_FALSE(q.push(3));
    EXPECT_FALSE(q.try_push(3));
    EXPECT_EQ(q.pop(), 1);
    EXPECT_EQ(q.pop(), 2);
    EXPECT_EQ(q.pop(), std::nullopt);
    EXPECT_EQ(q.pop(), std::nullopt);
    q.close();  // twice is fine
}

TEST(BoundedQueue, CloseWakesAWaitingPop) {
    BoundedQueue<int> q(1);
    std::thread consumer([&] { EXPECT_EQ(q.pop(), std::nullopt); });
    std::this_thread::sleep_for(50ms);
    q.close();
    consumer.join();  // hangs here if close() doesn't wake pop()
}

TEST(BoundedQueue, CloseWakesAWaitingPush) {
    BoundedQueue<int> q(1);
    q.push(1);
    std::thread producer([&] { EXPECT_FALSE(q.push(2)); });
    std::this_thread::sleep_for(50ms);
    q.close();
    producer.join();  // hangs here if close() doesn't wake push()
}

TEST(BoundedQueue, ManyProducersAndConsumersLoseNothing) {
    constexpr int kProducers = 4, kConsumers = 4, kEach = 20000;
    BoundedQueue<int> q(64);
    std::vector<std::atomic<int>> seen(kProducers * kEach);
    std::vector<std::thread> threads;
    for (int p = 0; p < kProducers; ++p)
        threads.emplace_back([&, p] {
            for (int i = 0; i < kEach; ++i) q.push(p * kEach + i);
        });
    for (int c = 0; c < kConsumers; ++c)
        threads.emplace_back([&] {
            while (auto item = q.pop()) seen[static_cast<std::size_t>(*item)]++;
        });
    for (int p = 0; p < kProducers; ++p) threads[static_cast<std::size_t>(p)].join();
    q.close();
    for (std::size_t c = kProducers; c < threads.size(); ++c) threads[c].join();
    int wrong = 0;
    for (auto& s : seen) wrong += (s != 1);
    EXPECT_EQ(wrong, 0) << "every item must come out exactly once";
}
