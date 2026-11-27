// Contract tests for VEC-1: the flat vector index.   (owner: lead)
// Switched on when src/vec/stub_flat_index.cpp is deleted.
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include "forge/vector_index.hpp"

using namespace forge;

namespace {

std::vector<Row> rows(const std::vector<ScoredRow>& hits) {
    std::vector<Row> out;
    for (const auto& h : hits) out.push_back(h.row);
    return out;
}

/// An index of dimension 2 holding the given vectors as rows 0, 1, 2...
std::unique_ptr<VectorIndex> index_of(const std::vector<std::vector<float>>& vectors) {
    auto idx = make_flat_index(vectors.at(0).size());
    for (std::size_t i = 0; i < vectors.size(); ++i) idx->add(static_cast<Row>(i), vectors[i]);
    return idx;
}

}  // namespace

TEST(FlatIndex, DimensionMustBeAtLeastOne) {
    EXPECT_THROW(make_flat_index(0), std::invalid_argument);
    auto idx = make_flat_index(384);
    EXPECT_EQ(idx->dim(), 384u);
    EXPECT_EQ(idx->size(), 0u);
}

TEST(FlatIndex, BadAddsChangeNothing) {
    auto idx = make_flat_index(3);
    idx->add(0, std::vector<float>{1, 0, 0});
    EXPECT_THROW(idx->add(2, std::vector<float>{1, 0, 0}), std::invalid_argument) << "row out of order";
    EXPECT_THROW(idx->add(0, std::vector<float>{1, 0, 0}), std::invalid_argument) << "row already added";
    EXPECT_THROW(idx->add(1, std::vector<float>{1, 0}), std::invalid_argument) << "wrong dimension";
    EXPECT_THROW(idx->add(1, std::vector<float>{0, 0, 0}), std::invalid_argument) << "all zeros";
    EXPECT_EQ(idx->size(), 1u);
    idx->add(1, std::vector<float>{0, 1, 0});
    EXPECT_EQ(idx->size(), 2u);
}

TEST(FlatIndex, FindsTheNearestByCosine) {
    auto idx = index_of({{1, 0}, {0, 1}, {1, 1}, {-1, 0}, {3, 0.1f}});
    std::vector<float> q = {1, 0};
    auto hits = idx->search(q, 3);
    EXPECT_EQ(rows(hits), (std::vector<Row>{0, 4, 2}));
    EXPECT_NEAR(hits[0].score, 1.0, 1e-6);
    EXPECT_NEAR(hits[1].score, 3 / std::sqrt(9.01), 1e-6) << "cosine ignores length: {3, 0.1} is almost {1, 0}";
    EXPECT_NEAR(hits[2].score, 1 / std::sqrt(2.0), 1e-6);
}

TEST(FlatIndex, NegativeSimilaritiesComeLast) {
    auto idx = index_of({{1, 0}, {0, 1}, {-1, 0}});
    auto hits = idx->search(std::vector<float>{1, 0}, 10);
    EXPECT_EQ(rows(hits), (std::vector<Row>{0, 1, 2}));
    EXPECT_NEAR(hits[2].score, -1.0, 1e-6);
}

TEST(FlatIndex, KLimitsTheResults) {
    auto idx = index_of({{1, 0}, {0, 1}, {1, 1}});
    std::vector<float> q = {1, 0};
    EXPECT_EQ(idx->search(q, 0).size(), 0u);
    EXPECT_EQ(idx->search(q, 1).size(), 1u);
    EXPECT_EQ(idx->search(q, 100).size(), 3u) << "fewer than k when there aren't enough";
    EXPECT_TRUE(make_flat_index(2)->search(q, 5).empty()) << "an empty index";
}

TEST(FlatIndex, EqualScoresAreOrderedByRow) {
    auto idx = index_of({{0, 1}, {2, 0}, {0, 1}, {1, 0}, {5, 0}});
    EXPECT_EQ(rows(idx->search(std::vector<float>{1, 0}, 3)), (std::vector<Row>{1, 3, 4}));
}

TEST(FlatIndex, TheFilterDecidesWhichRowsMayAppear) {
    auto idx = index_of({{1, 0}, {0.9f, 0.1f}, {0.8f, 0.2f}, {0.7f, 0.3f}});
    RowFilter odd = [](Row r) { return r % 2 == 1; };
    EXPECT_EQ(rows(idx->search(std::vector<float>{1, 0}, 10, odd)), (std::vector<Row>{1, 3}));
    RowFilter none = [](Row) { return false; };
    EXPECT_TRUE(idx->search(std::vector<float>{1, 0}, 10, none).empty());
}

TEST(FlatIndex, BadQueriesThrow) {
    auto idx = index_of({{1, 0}});
    EXPECT_THROW(idx->search(std::vector<float>{1, 0, 0}, 1), std::invalid_argument) << "wrong dimension";
    EXPECT_THROW(idx->search(std::vector<float>{0, 0}, 1), std::invalid_argument) << "all zeros";
}

TEST(FlatIndex, MatchesABruteForceReferenceOnRandomData) {
    constexpr std::size_t kDim = 64, kRows = 3000, kQueries = 20, kK = 10;
    std::mt19937 rng(42);
    std::normal_distribution<float> normal;
    std::vector<std::vector<float>> data(kRows, std::vector<float>(kDim));
    for (auto& v : data)
        for (auto& x : v) x = normal(rng);
    auto idx = index_of(data);
    for (std::size_t q = 0; q < kQueries; ++q) {
        std::vector<float> query(kDim);
        for (auto& x : query) x = normal(rng);
        std::vector<std::pair<double, Row>> want;
        double qn = 0;
        for (float x : query) qn += double(x) * x;
        for (Row r = 0; r < kRows; ++r) {
            double dot = 0, vn = 0;
            for (std::size_t i = 0; i < kDim; ++i) {
                dot += double(query[i]) * data[r][i];
                vn += double(data[r][i]) * data[r][i];
            }
            want.push_back({-dot / std::sqrt(qn * vn), r});
        }
        std::sort(want.begin(), want.end());
        auto hits = idx->search(query, kK);
        ASSERT_EQ(hits.size(), kK);
        for (std::size_t i = 0; i < kK; ++i) {
            EXPECT_EQ(hits[i].row, want[i].second) << "query " << q << ", place " << i;
            EXPECT_NEAR(hits[i].score, -want[i].first, 1e-4) << "query " << q << ", place " << i;
        }
    }
}
