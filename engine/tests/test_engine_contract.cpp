// Tests of the CONTRACT in engine.hpp. They must pass for the stub AND for
// Group A's real engine, so Group B can rely on them.
#include <gtest/gtest.h>

#include <filesystem>
#include <vector>

#include "qfis/engine.hpp"

TEST(ChunkStore, GetReturnsWhatWasAppended) {
    auto dir = std::filesystem::temp_directory_path() / "qfis_contract_store";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    qfis::ChunkStore store(dir.string());
    store.append({7, "acme_ar_fy2025", 4, "Profit for the year 16,900"});
    auto chunk = store.get(7);
    ASSERT_TRUE(chunk.has_value());
    EXPECT_EQ(chunk->text, "Profit for the year 16,900");
    EXPECT_FALSE(store.get(8).has_value());
    EXPECT_EQ(store.size(), 1u);
}

TEST(VectorIndex, ReturnsAtMostKHits) {
    qfis::VectorIndex index(4);
    std::vector<float> v{0.5f, 0.5f, 0.5f, 0.5f};
    for (std::uint64_t id = 1; id <= 5; ++id) index.add(id, v);
    EXPECT_EQ(index.search(v, 3).size(), 3u);
    EXPECT_EQ(index.search(v, 10).size(), 5u);
}

TEST(VectorIndex, RejectsTheWrongDimension) {
    qfis::VectorIndex index(4);
    std::vector<float> wrong{1.0f, 0.0f};
    EXPECT_THROW(index.add(1, wrong), std::invalid_argument);
}

TEST(KeywordIndex, ReturnsAtMostKHits) {
    qfis::KeywordIndex index;
    index.add(1, "revenue from operations");
    index.add(2, "profit for the year");
    index.finish();
    EXPECT_LE(index.search("revenue", 1).size(), 1u);
}
