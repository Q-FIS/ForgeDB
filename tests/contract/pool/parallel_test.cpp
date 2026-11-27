// Contract tests for POOL-5: parallel_for.   (owner: lead)
// Switched on when src/pool/stub_parallel.cpp is deleted. Needs POOL-2.
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <stdexcept>
#include <vector>

#include "forge/parallel.hpp"
#include "forge/thread_pool.hpp"

using namespace forge;
using namespace std::chrono_literals;

namespace {

class RefusingExecutor final : public Executor {
public:
    bool submit(std::function<void()>) override { return false; }
};

/// Runs parallel_for and checks that every index in [0, n) was visited once.
void expect_each_once(Executor& ex, std::size_t n, std::size_t pieces) {
    std::vector<std::atomic<int>> visits(n);
    parallel_for(ex, n, pieces, [&](std::size_t begin, std::size_t end) {
        EXPECT_LT(begin, end) << "empty ranges must not be passed to fn";
        for (std::size_t i = begin; i < end; ++i) visits[i]++;
    });
    for (std::size_t i = 0; i < n; ++i) EXPECT_EQ(visits[i], 1) << "index " << i;
}

}  // namespace

TEST(ParallelFor, CoversEveryIndexExactlyOnce) {
    ThreadPool pool(4, 64);
    expect_each_once(pool, 1000, 7);
    expect_each_once(pool, 1000, 1);
    expect_each_once(pool, 3, 10);  // more pieces than items
    expect_each_once(pool, 0, 4);   // nothing to do
}

TEST(ParallelFor, PiecesMustBeAtLeastOne) {
    InlineExecutor ex;
    EXPECT_THROW(parallel_for(ex, 10, 0, [](std::size_t, std::size_t) {}), std::invalid_argument);
}

TEST(ParallelFor, RunsOnTheCallerWhenTheExecutorRefuses) {
    RefusingExecutor ex;
    expect_each_once(ex, 500, 8);
}

TEST(ParallelFor, RethrowsAfterEveryPieceIsDone) {
    ThreadPool pool(4, 64);
    std::atomic<int> finished{0};
    EXPECT_THROW(parallel_for(pool, 80, 8,
                              [&](std::size_t begin, std::size_t) {
                                  if (begin == 30) throw std::runtime_error("piece failed");
                                  std::this_thread::sleep_for(10ms);
                                  ++finished;
                              }),
                 std::runtime_error);
    EXPECT_EQ(finished, 7) << "the other pieces must have finished before it rethrows";
}

TEST(ParallelFor, CallingItFromInsideAPoolTaskDoesNotDeadlock) {
    ThreadPool pool(1, 16);  // one worker: it will be busy running the outer task
    std::promise<std::size_t> result;
    pool.submit([&] {
        std::atomic<std::size_t> sum{0};
        parallel_for(pool, 100, 4, [&](std::size_t b, std::size_t e) {
            for (std::size_t i = b; i < e; ++i) sum += i;
        });
        result.set_value(sum);
    });
    auto f = result.get_future();
    ASSERT_EQ(f.wait_for(5s), std::future_status::ready) << "deadlock: the caller must work on pieces too";
    EXPECT_EQ(f.get(), 4950u);
}
