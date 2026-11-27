// Contract tests for SNAP-1: the in-memory Store.   (owner: lead)
// Switched on when src/snap/stub_memory_store.cpp is deleted.
#include <gtest/gtest.h>

#include <string>

#include "forge/store.hpp"

using namespace forge;

namespace {

Chunk chunk(std::string id, std::string text = "some text") {
    return Chunk{std::move(id), "doc", std::move(text), {1, 2, 3}, {{"k", "v"}}};
}

}  // namespace

TEST(MemoryStore, StartsEmpty) {
    auto s = make_memory_store();
    EXPECT_EQ(s->size(), 0u);
    EXPECT_EQ(s->find("anything"), std::nullopt);
    EXPECT_THROW(s->at(0), std::out_of_range);
}

TEST(MemoryStore, RowsCountFromZero) {
    auto s = make_memory_store();
    EXPECT_EQ(s->add(chunk("a")), 0u);
    EXPECT_EQ(s->add(chunk("b")), 1u);
    EXPECT_EQ(s->add(chunk("c")), 2u);
    EXPECT_EQ(s->size(), 3u);
}

TEST(MemoryStore, FindsByRowAndById) {
    auto s = make_memory_store();
    s->add(chunk("a", "first"));
    s->add(chunk("b", "second"));
    EXPECT_EQ(s->at(1), chunk("b", "second"));
    EXPECT_EQ(s->find("a"), Row{0});
    EXPECT_EQ(s->find("b"), Row{1});
    EXPECT_EQ(s->find("c"), std::nullopt);
    EXPECT_THROW(s->at(2), std::out_of_range);
}

TEST(MemoryStore, FindWorksWithAPartOfALongerString) {
    // A string_view needn't end where the id ends: find must not read past it.
    auto s = make_memory_store();
    s->add(chunk("abc"));
    std::string longer = "abcdef";
    EXPECT_EQ(s->find(std::string_view(longer).substr(0, 3)), Row{0});
    EXPECT_EQ(s->find(std::string_view(longer).substr(0, 2)), std::nullopt);
}

TEST(MemoryStore, ADuplicateIdChangesNothing) {
    auto s = make_memory_store();
    s->add(chunk("a", "original"));
    EXPECT_THROW(s->add(chunk("a", "impostor")), AlreadyExists);
    EXPECT_EQ(s->size(), 1u);
    EXPECT_EQ(s->at(0).text, "original");
}

TEST(MemoryStore, ManyChunks) {
    auto s = make_memory_store();
    for (int i = 0; i < 20000; ++i) s->add(chunk("id-" + std::to_string(i)));
    EXPECT_EQ(s->size(), 20000u);
    EXPECT_EQ(s->find("id-12345"), Row{12345});
    EXPECT_EQ(s->at(19999).id, "id-19999");
}
