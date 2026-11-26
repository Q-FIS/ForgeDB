// Contract tests for SNAP-2: encode_chunk and decode_chunk.   (owner: lead)
// Switched on when src/snap/stub_encoding.cpp is deleted.
// The byte layout is in docs/FORMAT.md.
#include <gtest/gtest.h>

#include <limits>
#include <string>
#include <vector>

#include "forge/store.hpp"

using namespace forge;

namespace {

std::vector<std::byte> b(std::initializer_list<int> values) {
    std::vector<std::byte> out;
    for (int v : values) out.push_back(static_cast<std::byte>(v));
    return out;
}

}  // namespace

TEST(Encoding, TheExactBytes) {
    Chunk c{"a", "d", "", {1.0f}, {{"k", "v"}}};
    EXPECT_EQ(encode_chunk(c), b({1, 0, 0, 0, 'a',              // id
                                  1, 0, 0, 0, 'd',              // doc_id
                                  0, 0, 0, 0,                   // text (empty)
                                  1, 0, 0, 0, 0, 0, 0x80, 0x3f, // vector: 1 number, 1.0f
                                  1, 0, 0, 0,                   // meta: 1 pair
                                  1, 0, 0, 0, 'k', 1, 0, 0, 0, 'v'}));
}

TEST(Encoding, RoundTrips) {
    std::vector<Chunk> chunks = {
        {"a", "d", "", {1.0f}, {}},
        {"infosys:p12:c0003", "infosys_fy25", "Revenue ₹1,45,000 crore, नमस्ते",
         {0.5f, -1.25f, 1e-30f, std::numeric_limits<float>::max(), -0.0f},
         {{"company", "infosys"}, {"fy", "FY2025"}, {"empty", ""}}},
        {std::string("nul\0inside", 10), "d", std::string(100000, 'x'), std::vector<float>(1024, 0.25f), {}},
    };
    for (const auto& c : chunks) EXPECT_EQ(decode_chunk(encode_chunk(c)), c) << c.id;
}

TEST(Encoding, EveryShorterPrefixIsAnError) {
    Chunk c{"id", "doc", "text", {1, 2}, {{"k", "v"}}};
    auto bytes = encode_chunk(c);
    for (std::size_t n = 0; n < bytes.size(); ++n)
        EXPECT_THROW(decode_chunk(std::span(bytes).first(n)), std::runtime_error) << "first " << n << " bytes";
}

TEST(Encoding, ExtraBytesAtTheEndAreAnError) {
    auto bytes = encode_chunk(Chunk{"id", "doc", "text", {1}, {}});
    bytes.push_back(std::byte{0});
    EXPECT_THROW(decode_chunk(bytes), std::runtime_error);
}

TEST(Encoding, AHugeLengthIsAnErrorNotAnAllocation) {
    auto bytes = b({0xff, 0xff, 0xff, 0x7f, 'a', 'b'});
    EXPECT_THROW(decode_chunk(bytes), std::runtime_error);
    auto vec = b({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0xff, 0x7f});  // empty strings, then 2^31 floats
    EXPECT_THROW(decode_chunk(vec), std::runtime_error);
}
