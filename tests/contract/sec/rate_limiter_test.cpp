// Contract tests for SEC-1: RateLimiter and constant_time_equals.   (owner: lead)
// Switched on when src/sec/stub_rate_limiter.cpp is deleted.
// Run these on the tsan preset too: ctest --preset tsan -R contract.sec
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "forge/security.hpp"

using forge::RateLimiter;
using namespace std::chrono_literals;

namespace {
const RateLimiter::Clock::time_point t0 = RateLimiter::Clock::time_point{} + 1000s;  // a fake clock
}

TEST(RateLimiter, BadSettingsAreRefused) {
    EXPECT_THROW(RateLimiter(0, 5), std::invalid_argument);
    EXPECT_THROW(RateLimiter(-1, 5), std::invalid_argument);
    EXPECT_THROW(RateLimiter(10, 0.5), std::invalid_argument);
}

TEST(RateLimiter, ANewKeyHasAFullBurst) {
    RateLimiter r(1, 5);
    for (int i = 0; i < 5; ++i) EXPECT_TRUE(r.allow("alice", t0)) << "request " << i;
    EXPECT_FALSE(r.allow("alice", t0)) << "the 6th request at the same moment is over the burst";
}

TEST(RateLimiter, KeysHaveSeparateBuckets) {
    RateLimiter r(1, 2);
    EXPECT_TRUE(r.allow("alice", t0));
    EXPECT_TRUE(r.allow("alice", t0));
    EXPECT_FALSE(r.allow("alice", t0));
    EXPECT_TRUE(r.allow("bob", t0)) << "bob's bucket is not alice's";
    EXPECT_EQ(r.tracked_keys(), 2u);
}

TEST(RateLimiter, TokensRefillWithTime) {
    RateLimiter r(2, 2);  // 2 tokens a second
    EXPECT_TRUE(r.allow("k", t0));
    EXPECT_TRUE(r.allow("k", t0));
    EXPECT_FALSE(r.allow("k", t0));
    EXPECT_FALSE(r.allow("k", t0 + 400ms)) << "0.8 tokens: not enough yet";
    EXPECT_TRUE(r.allow("k", t0 + 500ms)) << "1 token after half a second";
    EXPECT_FALSE(r.allow("k", t0 + 500ms));
    EXPECT_TRUE(r.allow("k", t0 + 1500ms));
    EXPECT_TRUE(r.allow("k", t0 + 1500ms)) << "a full second later: 2 more tokens";
    EXPECT_FALSE(r.allow("k", t0 + 1500ms));
}

TEST(RateLimiter, TheBucketNeverHoldsMoreThanTheBurst) {
    RateLimiter r(10, 3);
    EXPECT_TRUE(r.allow("k", t0));
    int allowed = 0;
    for (int i = 0; i < 10; ++i) allowed += r.allow("k", t0 + 1h);
    EXPECT_EQ(allowed, 3) << "an hour of refilling still caps at the burst";
}

TEST(RateLimiter, ARefusedRequestTakesNoToken) {
    RateLimiter r(1, 1);
    EXPECT_TRUE(r.allow("k", t0));
    for (int i = 0; i < 100; ++i) EXPECT_FALSE(r.allow("k", t0 + 500ms));
    EXPECT_TRUE(r.allow("k", t0 + 1s)) << "refused requests must not use up tokens";
}

TEST(RateLimiter, RetryAfterSaysWhenATokenWillBeThere) {
    RateLimiter r(4, 1);  // a token every 250 ms
    EXPECT_EQ(r.retry_after("new", t0), RateLimiter::Clock::duration::zero());
    EXPECT_TRUE(r.allow("k", t0));
    auto wait = r.retry_after("k", t0 + 100ms);
    EXPECT_GE(wait, 140ms);
    EXPECT_LE(wait, 160ms);
    EXPECT_EQ(r.retry_after("k", t0 + 250ms), RateLimiter::Clock::duration::zero());
}

TEST(RateLimiter, IdleKeysAreForgotten) {
    RateLimiter r(1, 5);
    r.allow("old", t0);
    r.allow("recent", t0 + 50s);
    r.forget_idle(t0 + 70s, 60s);
    EXPECT_EQ(r.tracked_keys(), 1u) << "'old' was last seen 70 s ago, 'recent' 20 s ago";
    for (int i = 0; i < 5; ++i) EXPECT_TRUE(r.allow("old", t0 + 70s)) << "a forgotten key starts full again";
}

TEST(RateLimiter, ManyThreadsShareOneLimiterExactly) {
    RateLimiter r(1, 1000);
    std::atomic<int> allowed{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t)
        threads.emplace_back([&] {
            for (int i = 0; i < 500; ++i) allowed += r.allow("shared", t0);
        });
    for (auto& t : threads) t.join();
    EXPECT_EQ(allowed, 1000) << "4,000 tries at one instant: exactly the burst may pass";
}

TEST(ConstantTimeEquals, ComparesLikeEquals) {
    EXPECT_TRUE(forge::constant_time_equals("", ""));
    EXPECT_TRUE(forge::constant_time_equals("s3cret-key", "s3cret-key"));
    EXPECT_FALSE(forge::constant_time_equals("s3cret-key", "s3cret-kez"));
    EXPECT_FALSE(forge::constant_time_equals("s3cret-key", "x3cret-key"));
    EXPECT_FALSE(forge::constant_time_equals("short", "shorter"));
    EXPECT_FALSE(forge::constant_time_equals("", "a"));
    EXPECT_TRUE(forge::constant_time_equals(std::string("a\0b", 3), std::string("a\0b", 3)));
    EXPECT_FALSE(forge::constant_time_equals(std::string("a\0b", 3), std::string("a\0c", 3)));
}
