// Contract tests for KW-2: the keyword index.   (owner: lead)
// Switched on when src/kw/stub_keyword_index.cpp is deleted. Needs KW-1.
//
// These rules hold for both scorings: counting (KW-2) and BM25 (KW-3), so
// they keep passing when you switch. KW-3 adds its own exact-score tests.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "forge/keyword_index.hpp"

using namespace forge;

namespace {

std::unique_ptr<KeywordIndex> index_of(const std::vector<std::string>& texts) {
    auto idx = make_keyword_index();
    for (std::size_t i = 0; i < texts.size(); ++i) idx->add(static_cast<Row>(i), texts[i]);
    return idx;
}

std::vector<Row> rows(const std::vector<ScoredRow>& hits) {
    std::vector<Row> out;
    for (const auto& h : hits) out.push_back(h.row);
    return out;
}

}  // namespace

TEST(KeywordIndex, RowsArriveInOrder) {
    auto idx = make_keyword_index();
    EXPECT_EQ(idx->size(), 0u);
    idx->add(0, "first");
    EXPECT_THROW(idx->add(2, "skipped one"), std::invalid_argument);
    EXPECT_THROW(idx->add(0, "again"), std::invalid_argument);
    idx->add(1, "second");
    EXPECT_EQ(idx->size(), 2u);
}

TEST(KeywordIndex, OnlyRowsWithAQueryWordComeBack) {
    auto idx = index_of({"net profit rose", "revenue fell", "profit margin", "nothing here"});
    EXPECT_EQ(rows(idx->search("profit", 10)), (std::vector<Row>{0, 2}));
    EXPECT_TRUE(idx->search("zebra", 10).empty());
    EXPECT_TRUE(idx->search("", 10).empty());
    EXPECT_TRUE(idx->search("!!! ...", 10).empty()) << "a query with no words";
}

TEST(KeywordIndex, UsesTheTokenizer) {
    auto idx = index_of({"Revenue: ₹1,45,000 crore", "FY2025 results"});
    EXPECT_EQ(rows(idx->search("REVENUE", 10)), (std::vector<Row>{0}));
    EXPECT_EQ(rows(idx->search("1,45,000", 10)), (std::vector<Row>{0}));
    EXPECT_EQ(rows(idx->search("fy2025", 10)), (std::vector<Row>{1}));
}

TEST(KeywordIndex, MoreOfTheWordScoresHigher) {
    auto idx = index_of({"apple pear plum", "apple apple pear"});  // same length
    auto hits = idx->search("apple", 10);
    EXPECT_EQ(rows(hits), (std::vector<Row>{1, 0}));
    EXPECT_GT(hits[0].score, hits[1].score);
}

TEST(KeywordIndex, MoreDifferentQueryWordsScoresHigher) {
    auto idx = index_of({"apple pear plum", "apple banana plum", "kiwi lime plum"});  // same length
    EXPECT_EQ(rows(idx->search("apple banana", 10)), (std::vector<Row>{1, 0}));
}

TEST(KeywordIndex, EqualScoresAreOrderedByRow) {
    auto idx = index_of({"other", "profit", "words", "profit", "profit"});
    EXPECT_EQ(rows(idx->search("profit", 10)), (std::vector<Row>{1, 3, 4}));
}

TEST(KeywordIndex, KAndTheFilter) {
    auto idx = index_of({"profit a", "profit b", "profit c", "profit d"});
    EXPECT_EQ(idx->search("profit", 2).size(), 2u);
    EXPECT_TRUE(idx->search("profit", 0).empty());
    RowFilter even = [](Row r) { return r % 2 == 0; };
    EXPECT_EQ(rows(idx->search("profit", 10, even)), (std::vector<Row>{0, 2}));
}

TEST(KeywordIndex, ScoresArePositiveAndBestFirst) {
    auto idx = index_of({"profit profit profit loss", "profit", "loss loss", "profit loss"});
    auto hits = idx->search("profit loss", 10);
    ASSERT_EQ(hits.size(), 4u);
    for (std::size_t i = 0; i < hits.size(); ++i) {
        EXPECT_GT(hits[i].score, 0.0f);
        if (i > 0) {
            EXPECT_TRUE(better(hits[i - 1], hits[i]) || hits[i - 1] == hits[i]);
        }
    }
}
