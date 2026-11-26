// Contract tests for KW-5: rrf_merge.   (owner: lead)
// Switched on when src/kw/stub_rrf.cpp is deleted.
#include <gtest/gtest.h>

#include <vector>

#include "forge/keyword_index.hpp"

using namespace forge;

TEST(Rrf, MergesTwoRankings) {
    std::vector<ScoredRow> by_vector = {{5, 0.9f}, {7, 0.8f}};
    std::vector<ScoredRow> by_keyword = {{7, 12.0f}, {9, 3.0f}};
    auto merged = rrf_merge(by_vector, by_keyword, 10);
    ASSERT_EQ(merged.size(), 3u);
    EXPECT_EQ(merged[0].row, 7u);  // 1/62 + 1/61: in both lists
    EXPECT_EQ(merged[1].row, 5u);  // 1/61: first in one list
    EXPECT_EQ(merged[2].row, 9u);  // 1/62
    EXPECT_NEAR(merged[0].score, 1.0 / 62 + 1.0 / 61, 1e-7);
    EXPECT_NEAR(merged[1].score, 1.0 / 61, 1e-7);
    EXPECT_NEAR(merged[2].score, 1.0 / 62, 1e-7);
}

TEST(Rrf, OnlyThePositionCountsNotTheScore) {
    auto a = rrf_merge({{1, 1000.0f}}, {}, 5);
    auto b = rrf_merge({{1, 0.001f}}, {}, 5);
    EXPECT_EQ(a, b);
}

TEST(Rrf, KAndC) {
    std::vector<ScoredRow> a = {{1, 1}, {2, 1}, {3, 1}};
    EXPECT_EQ(rrf_merge(a, {}, 2).size(), 2u);
    EXPECT_TRUE(rrf_merge(a, a, 0).empty());
    EXPECT_NEAR(rrf_merge(a, {}, 1, 10.0)[0].score, 1.0 / 11, 1e-7);
}

TEST(Rrf, EqualScoresAreOrderedByRow) {
    auto merged = rrf_merge({{8, 1.0f}}, {{3, 1.0f}}, 10);  // both 1/61
    ASSERT_EQ(merged.size(), 2u);
    EXPECT_EQ(merged[0].row, 3u);
    EXPECT_EQ(merged[1].row, 8u);
}

TEST(Rrf, EmptyListsGiveNothing) {
    EXPECT_TRUE(rrf_merge({}, {}, 10).empty());
}
