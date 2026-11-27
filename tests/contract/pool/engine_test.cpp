// Contract tests for POOL-3: the Engine.   (owner: lead)
// Switched on when src/pool/stub_engine.cpp is deleted.
// Uses the fakes in tests/support/fakes.hpp, so it doesn't wait for SNAP, VEC or KW.
// Run these on the tsan preset too: ctest --preset tsan -R engine
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "forge/engine.hpp"
#include "support/fakes.hpp"

using namespace forge;
using namespace forge::testing;

namespace {

struct Parts {
    FakeStore* store;
    FakeVectorIndex* vectors;
    FakeKeywordIndex* keywords;
};

/// An Engine over fakes, dimension 3; `parts` lets the test look inside.
std::unique_ptr<Engine> make_engine(Parts& parts) {
    auto s = std::make_unique<FakeStore>();
    auto v = std::make_unique<FakeVectorIndex>(3);
    auto k = std::make_unique<FakeKeywordIndex>();
    parts = {s.get(), v.get(), k.get()};
    return std::make_unique<Engine>(std::move(s), std::move(v), std::move(k));
}

Chunk chunk(std::string id, std::string doc, std::string text, std::vector<float> vec,
            std::map<std::string, std::string> meta = {}) {
    return Chunk{std::move(id), std::move(doc), std::move(text), std::move(vec), std::move(meta)};
}

std::vector<std::string> ids(const std::vector<SearchHit>& hits) {
    std::vector<std::string> out;
    for (const auto& h : hits) out.push_back(h.id);
    return out;
}

SearchRequest vector_search(std::vector<float> v, std::size_t k = 10) {
    SearchRequest r;
    r.mode = SearchMode::vector;
    r.vector = std::move(v);
    r.k = k;
    return r;
}

SearchRequest keyword_search(std::string text, std::size_t k = 10) {
    SearchRequest r;
    r.mode = SearchMode::keyword;
    r.text = std::move(text);
    r.k = k;
    return r;
}

void expect_unchanged(const Parts& p, std::size_t n) {
    EXPECT_EQ(p.store->size(), n);
    EXPECT_EQ(p.vectors->size(), n);
    EXPECT_EQ(p.keywords->size(), n);
}

}  // namespace

TEST(Engine, ThePartsMustHoldTheSameNumberOfRows) {
    auto s = std::make_unique<FakeStore>();
    s->add(chunk("a", "d", "t", {1, 0, 0}));
    EXPECT_THROW(Engine(std::move(s), std::make_unique<FakeVectorIndex>(3), std::make_unique<FakeKeywordIndex>()),
                 std::invalid_argument);
}

TEST(Engine, PutAddsToAllThreeParts) {
    Parts p;
    auto e = make_engine(p);
    EXPECT_EQ(e->dim(), 3u);
    EXPECT_EQ(e->put(chunk("a", "d1", "alpha", {1, 0, 0})), 0u);
    EXPECT_EQ(e->put(chunk("b", "d1", "beta", {0, 1, 0})), 1u);
    EXPECT_EQ(e->size(), 2u);
    expect_unchanged(p, 2);
}

TEST(Engine, GetReturnsACopy) {
    Parts p;
    auto e = make_engine(p);
    Chunk c = chunk("a", "d1", "alpha", {1, 2, 3}, {{"fy", "FY2025"}});
    e->put(c);
    EXPECT_EQ(e->get("a"), c);
    EXPECT_EQ(e->get("missing"), std::nullopt);
}

TEST(Engine, BadChunksChangeNothing) {
    Parts p;
    auto e = make_engine(p);
    e->put(chunk("a", "d", "t", {1, 0, 0}));
    EXPECT_THROW(e->put(chunk("", "d", "t", {1, 0, 0})), std::invalid_argument) << "empty id";
    EXPECT_THROW(e->put(chunk("b", "d", "t", {1, 0})), std::invalid_argument) << "wrong dimension";
    EXPECT_THROW(e->put(chunk("c", "d", "t", {0, 0, 0})), std::invalid_argument) << "all zeros";
    EXPECT_THROW(e->put(chunk("a", "d", "other", {0, 1, 0})), AlreadyExists) << "same id";
    expect_unchanged(p, 1);
    EXPECT_EQ(e->get("a")->text, "t");
}

TEST(Engine, VectorSearchReturnsHitsBestFirst) {
    Parts p;
    auto e = make_engine(p);
    e->put(chunk("x", "d1", "text x", {1, 0, 0}));
    e->put(chunk("y", "d2", "text y", {0, 1, 0}));
    e->put(chunk("z", "d1", "text z", {1, 1, 0}));
    auto hits = e->search(vector_search({1, 0.1f, 0}, 2));
    ASSERT_EQ(hits.size(), 2u);
    EXPECT_EQ(ids(hits), (std::vector<std::string>{"x", "z"}));
    EXPECT_EQ(hits[0].doc_id, "d1");
    EXPECT_EQ(hits[0].text, "text x");
    EXPECT_GT(hits[0].score, hits[1].score);
}

TEST(Engine, KeywordSearch) {
    Parts p;
    auto e = make_engine(p);
    e->put(chunk("x", "d", "profit rose", {1, 0, 0}));
    e->put(chunk("y", "d", "revenue fell", {0, 1, 0}));
    EXPECT_EQ(ids(e->search(keyword_search("profit"))), (std::vector<std::string>{"x"}));
    EXPECT_TRUE(e->search(keyword_search("zebra")).empty());
}

TEST(Engine, FiltersApplyToBothKindsOfSearch) {
    Parts p;
    auto e = make_engine(p);
    e->put(chunk("a", "infy", "profit", {1, 0, 0}, {{"fy", "FY2024"}}));
    e->put(chunk("b", "tcs", "profit", {1, 0, 0}, {{"fy", "FY2025"}}));
    e->put(chunk("c", "infy", "profit", {1, 0, 0}, {{"fy", "FY2025"}}));
    SearchRequest v = vector_search({1, 0, 0});
    v.filter.doc_id = "infy";
    EXPECT_EQ(ids(e->search(v)), (std::vector<std::string>{"a", "c"}));
    SearchRequest k = keyword_search("profit");
    k.filter.meta = {{"fy", "FY2025"}};
    EXPECT_EQ(ids(e->search(k)), (std::vector<std::string>{"b", "c"}));
    k.filter.doc_id = "infy";
    EXPECT_EQ(ids(e->search(k)), (std::vector<std::string>{"c"}));
}

TEST(Engine, BadSearchesThrow) {
    Parts p;
    auto e = make_engine(p);
    e->put(chunk("a", "d", "t", {1, 0, 0}));
    EXPECT_THROW(e->search(vector_search({1, 0, 0}, 0)), std::invalid_argument) << "k = 0";
    EXPECT_THROW(e->search(vector_search({1, 0, 0}, 1001)), std::invalid_argument) << "k = 1001";
    EXPECT_THROW(e->search(vector_search({1, 0}, 5)), std::invalid_argument) << "wrong dimension";
    EXPECT_THROW(e->search(vector_search({}, 5)), std::invalid_argument) << "no vector";
    EXPECT_THROW(e->search(keyword_search("", 5)), std::invalid_argument) << "no text";
    EXPECT_NO_THROW(e->search(vector_search({1, 0, 0}, 1000)));
}

TEST(Engine, SearchingAnEmptyEngineFindsNothing) {
    Parts p;
    auto e = make_engine(p);
    EXPECT_TRUE(e->search(vector_search({1, 0, 0})).empty());
    EXPECT_TRUE(e->search(keyword_search("anything")).empty());
}

TEST(Engine, ManyThreadsPuttingAndSearchingAtOnce) {
    // The fakes are not thread-safe: only the Engine's lock protects them.
    // ThreadSanitizer (the tsan preset) reports any access without the lock.
    Parts p;
    auto e = make_engine(p);
    constexpr int kWriters = 4, kEach = 250;
    std::atomic<bool> writing{true};
    std::atomic<int> searches{0};
    std::vector<std::thread> threads;
    for (int w = 0; w < kWriters; ++w)
        threads.emplace_back([&, w] {
            for (int i = 0; i < kEach; ++i) {
                std::string id = "w" + std::to_string(w) + "-" + std::to_string(i);
                e->put(chunk(id, "d", "word" + std::to_string(i % 7), {1, float(w), float(i)}));
            }
        });
    for (int r = 0; r < 4; ++r)
        threads.emplace_back([&] {
            // A short pause between searches: with readers back to back, a
            // shared_mutex may never let a writer in ("writer starvation").
            for (int i = 0; i < 300 && writing; ++i) {
                e->search(vector_search({1, 1, 1}, 5));
                e->search(keyword_search("word3", 5));
                e->get("w0-0");
                ++searches;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    for (int w = 0; w < kWriters; ++w) threads[static_cast<std::size_t>(w)].join();
    writing = false;
    for (std::size_t r = kWriters; r < threads.size(); ++r) threads[r].join();
    EXPECT_EQ(e->size(), std::size_t{kWriters * kEach});
    expect_unchanged(p, kWriters * kEach);
    for (int w = 0; w < kWriters; ++w) EXPECT_TRUE(e->get("w" + std::to_string(w) + "-249").has_value());
    EXPECT_GT(searches, 0);
}
