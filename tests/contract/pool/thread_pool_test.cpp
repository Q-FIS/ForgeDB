// Contract tests for POOL-2: ThreadPool.   (owner: lead)
// Switched on when src/pool/stub_thread_pool.cpp is deleted. Needs POOL-1.
// Run these on the tsan preset too: ctest --preset tsan -R thread_pool
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>

#include "forge/thread_pool.hpp"

using forge::ThreadPool;
using namespace std::chrono_literals;

TEST(ThreadPool, SizesMustBeAtLeastOne) {
    EXPECT_THROW(ThreadPool(0, 10), std::invalid_argument);
    EXPECT_THROW(ThreadPool(2, 0), std::invalid_argument);
}

TEST(ThreadPool, RunsEveryTask) {
    std::atomic<int> count{0};
    ThreadPool pool(4, 2000);
    EXPECT_EQ(pool.threads(), 4u);
    for (int i = 0; i < 1000; ++i) ASSERT_TRUE(pool.submit([&] { ++count; }));
    pool.stop();
    EXPECT_EQ(count, 1000);
}

TEST(ThreadPool, RunsTasksAtTheSameTime) {
    // 4 tasks that each wait until all 4 have started: possible only if 4
    // threads run them at once.
    ThreadPool pool(4, 10);
    std::atomic<int> started{0};
    std::atomic<int> saw_all{0};
    for (int i = 0; i < 4; ++i)
        pool.submit([&] {
            ++started;
            auto deadline = std::chrono::steady_clock::now() + 5s;
            while (started < 4 && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
            if (started == 4) ++saw_all;
        });
    pool.stop();
    EXPECT_EQ(saw_all, 4);
}

TEST(ThreadPool, SubmitReturnsFalseWhenTheQueueIsFull) {
    ThreadPool pool(1, 1);
    std::promise<void> release;
    std::shared_future<void> released = release.get_future().share();
    std::promise<void> started;
    std::atomic<int> ran{0};
    ASSERT_TRUE(pool.submit([&] {
        started.set_value();
        released.wait();
        ++ran;
    }));
    started.get_future().wait();                     // the only worker is busy now
    EXPECT_TRUE(pool.submit([&] { ++ran; }));        // waits in the queue
    EXPECT_FALSE(pool.submit([&] { ran += 100; }));  // no room: refused, never run
    release.set_value();
    pool.stop();
    EXPECT_EQ(ran, 2);
}

TEST(ThreadPool, StopFinishesTheTasksAlreadyQueued) {
    ThreadPool pool(1, 100);
    std::atomic<int> ran{0};
    pool.submit([&] {
        std::this_thread::sleep_for(50ms);
        ++ran;
    });
    for (int i = 0; i < 50; ++i) pool.submit([&] { ++ran; });
    pool.stop();
    EXPECT_EQ(ran, 51);
}

TEST(ThreadPool, SubmitAfterStopIsRefused) {
    ThreadPool pool(2, 10);
    pool.stop();
    bool ran = false;
    EXPECT_FALSE(pool.submit([&] { ran = true; }));
    pool.stop();  // twice is fine
    EXPECT_FALSE(ran);
}

TEST(ThreadPool, TheDestructorStopsThePool) {
    std::atomic<int> ran{0};
    {
        ThreadPool pool(2, 100);
        for (int i = 0; i < 20; ++i) pool.submit([&] { ++ran; });
    }
    EXPECT_EQ(ran, 20);
}
