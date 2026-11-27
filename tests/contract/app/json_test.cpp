// Contract tests for APP-2: the API's JSON.   (owner: lead)
// Switched on when src/app/stub_json.cpp is deleted.
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <string>

#include "forge/app.hpp"

using namespace forge;
using nlohmann::json;

namespace {

const char* kChunk = R"({"id": "infosys:p12", "doc_id": "infosys_fy25", "text": "Revenue ₹1,45,000 crore",
                         "vector": [0.5, -1, 2e-3], "meta": {"company": "infosys", "fy": "FY2025"}})";

void expect_bad_chunk(const std::string& body, const char* why) {
    EXPECT_THROW(chunk_from_json(body), std::invalid_argument) << why << ": " << body;
}

void expect_bad_search(const std::string& body, const char* why) {
    EXPECT_THROW(search_from_json(body), std::invalid_argument) << why << ": " << body;
}

/// kChunk with one field replaced (or removed, if value is null).
std::string chunk_with(const std::string& key, json value) {
    json j = json::parse(kChunk);
    if (value.is_null()) j.erase(key);
    else j[key] = value;
    return j.dump();
}

}  // namespace

TEST(Json, ReadsAChunk) {
    Chunk c = chunk_from_json(kChunk);
    EXPECT_EQ(c.id, "infosys:p12");
    EXPECT_EQ(c.doc_id, "infosys_fy25");
    EXPECT_EQ(c.text, "Revenue ₹1,45,000 crore");
    EXPECT_EQ(c.vector, (std::vector<float>{0.5f, -1.0f, 0.002f}));
    EXPECT_EQ(c.meta, (std::map<std::string, std::string>{{"company", "infosys"}, {"fy", "FY2025"}}));
}

TEST(Json, MetaIsOptional) {
    EXPECT_TRUE(chunk_from_json(chunk_with("meta", nullptr)).meta.empty());
}

TEST(Json, BadChunks) {
    expect_bad_chunk("not json", "invalid JSON");
    expect_bad_chunk("[1, 2]", "not an object");
    expect_bad_chunk(chunk_with("id", nullptr), "no id");
    expect_bad_chunk(chunk_with("id", ""), "empty id");
    expect_bad_chunk(chunk_with("id", 5), "id not a string");
    expect_bad_chunk(chunk_with("doc_id", nullptr), "no doc_id");
    expect_bad_chunk(chunk_with("text", nullptr), "no text");
    expect_bad_chunk(chunk_with("text", json::array()), "text not a string");
    expect_bad_chunk(chunk_with("vector", nullptr), "no vector");
    expect_bad_chunk(chunk_with("vector", json::array()), "empty vector");
    expect_bad_chunk(chunk_with("vector", json::array({1, "2"})), "a vector entry that isn't a number");
    expect_bad_chunk(chunk_with("vector", json::array({1, true})), "true is not a number");
    expect_bad_chunk(chunk_with("meta", json{{"k", 1}}), "a meta value that isn't a string");
    expect_bad_chunk(chunk_with("meta", "x"), "meta not an object");
    expect_bad_chunk(chunk_with("colour", "red"), "an unknown field");
}

TEST(Json, AChunkSurvivesTheRoundTrip) {
    Chunk c = chunk_from_json(kChunk);
    std::string out = chunk_to_json(c);
    json j = json::parse(out);
    EXPECT_EQ(j.size(), 5u) << "exactly id, doc_id, text, vector and meta: " << out;
    EXPECT_EQ(j["id"], "infosys:p12");
    EXPECT_TRUE(j["meta"].is_object());
    EXPECT_EQ(chunk_from_json(out), c);
    Chunk empty_meta = chunk_from_json(chunk_with("meta", nullptr));
    EXPECT_TRUE(json::parse(chunk_to_json(empty_meta))["meta"].is_object()) << "meta is {} even when empty";
}

TEST(Json, ReadsSearchRequests) {
    SearchRequest v = search_from_json(R"({"vector": [1, 0]})");
    EXPECT_EQ(v.mode, SearchMode::vector) << "vector is the default mode";
    EXPECT_EQ(v.k, 10u) << "10 is the default k";
    EXPECT_EQ(v.vector, (std::vector<float>{1, 0}));
    EXPECT_TRUE(v.filter.empty());

    SearchRequest k = search_from_json(R"({"mode": "keyword", "text": "net profit", "k": 3,
                                           "filter": {"doc_id": "infy", "meta": {"fy": "FY2025"}}})");
    EXPECT_EQ(k.mode, SearchMode::keyword);
    EXPECT_EQ(k.text, "net profit");
    EXPECT_EQ(k.k, 3u);
    EXPECT_EQ(k.filter.doc_id, "infy");
    EXPECT_EQ(k.filter.meta, (std::map<std::string, std::string>{{"fy", "FY2025"}}));

    SearchRequest h = search_from_json(R"({"mode": "hybrid", "text": "profit", "vector": [0.5], "k": 1000})");
    EXPECT_EQ(h.mode, SearchMode::hybrid);
    EXPECT_EQ(h.k, 1000u);
}

TEST(Json, BadSearchRequests) {
    expect_bad_search("{", "invalid JSON");
    expect_bad_search("[]", "not an object");
    expect_bad_search(R"({"mode": "magic", "vector": [1]})", "unknown mode");
    expect_bad_search(R"({"vector": [1], "k": 0})", "k = 0");
    expect_bad_search(R"({"vector": [1], "k": 1001})", "k = 1001");
    expect_bad_search(R"({"vector": [1], "k": 2.5})", "k not an integer");
    expect_bad_search(R"({"vector": [1], "k": "5"})", "k a string");
    expect_bad_search(R"({"vector": [1], "k": true})", "k a boolean");
    expect_bad_search(R"({"mode": "vector"})", "vector mode without a vector");
    expect_bad_search(R"({"mode": "vector", "vector": []})", "an empty vector");
    expect_bad_search(R"({"mode": "keyword"})", "keyword mode without text");
    expect_bad_search(R"({"mode": "keyword", "text": ""})", "keyword mode with empty text");
    expect_bad_search(R"({"mode": "hybrid", "text": "x"})", "hybrid without a vector");
    expect_bad_search(R"({"mode": "hybrid", "vector": [1]})", "hybrid without text");
    expect_bad_search(R"({"vector": [1], "limit": 5})", "an unknown field");
    expect_bad_search(R"({"vector": [1], "filter": {"company": "x"}})", "an unknown field in filter");
    expect_bad_search(R"({"vector": [1], "filter": {"meta": {"fy": 2025}}})", "a meta value that isn't a string");
    expect_bad_search(R"({"vector": [1], "filter": "infy"})", "filter not an object");
}

TEST(Json, WritesHits) {
    std::vector<SearchHit> hits = {{"a", "d1", "first", 0.5f}, {"b", "d2", "second \"quoted\"", -0.25f}};
    json j = json::parse(hits_to_json(hits, 1.5));
    EXPECT_EQ(j["took_ms"], 1.5);
    ASSERT_EQ(j["hits"].size(), 2u);
    EXPECT_EQ(j["hits"][0], (json{{"id", "a"}, {"doc_id", "d1"}, {"text", "first"}, {"score", 0.5}}));
    EXPECT_EQ(j["hits"][1]["text"], "second \"quoted\"");
    EXPECT_EQ(j["hits"][1]["score"], -0.25);
    EXPECT_TRUE(json::parse(hits_to_json({}, 0))["hits"].is_array()) << "no hits is [], not null";
}
