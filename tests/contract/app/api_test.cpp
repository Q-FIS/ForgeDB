// Contract tests for APP-3: the Api.   (owner: lead)
// Switched on when src/app/stub_api.cpp is deleted. Needs HTTP-3, APP-2 and POOL-3.
// Uses the fakes in tests/support/fakes.hpp for the Store and the indexes.
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>

#include "forge/app.hpp"
#include "support/fakes.hpp"

using namespace forge;
using namespace forge::testing;
using nlohmann::json;

namespace {

struct TestApi {
    TestApi() {
        auto s = std::make_unique<FakeStore>();
        store = s.get();
        engine = std::make_unique<Engine>(std::move(s), std::make_unique<FakeVectorIndex>(3),
                                          std::make_unique<FakeKeywordIndex>());
        api = std::make_unique<Api>(*engine);
    }

    http::Response call(std::string method, std::string target, std::string body = "") {
        http::Request r;
        r.method = std::move(method);
        r.target = std::move(target);
        r.version = "HTTP/1.1";
        r.body = std::move(body);
        http::Response res = api->handle(r);
        EXPECT_EQ(header(res, "Content-Type"), "application/json") << r.method << " " << r.target;
        return res;
    }

    static std::string header(const http::Response& r, std::string_view name) {
        for (const auto& h : r.headers)
            if (h.name == name) return h.value;
        return "";
    }

    FakeStore* store;
    std::unique_ptr<Engine> engine;
    std::unique_ptr<Api> api;
};

json body(const http::Response& r) { return json::parse(r.body); }

const std::string kA = R"({"id": "a:1", "doc_id": "d1", "text": "net profit rose", "vector": [1, 0, 0]})";
const std::string kB = R"({"id": "b/2", "doc_id": "d2", "text": "revenue fell", "vector": [0, 1, 0], "meta": {"fy": "FY2025"}})";

}  // namespace

TEST(Api, Health) {
    TestApi t;
    auto r = t.call("GET", "/health");
    EXPECT_EQ(r.status, 200);
    EXPECT_EQ(body(r), (json{{"status", "ok"}, {"chunks", 0}, {"dim", 3}}));
}

TEST(Api, PutAndGet) {
    TestApi t;
    auto r = t.call("POST", "/v1/chunks", kA);
    EXPECT_EQ(r.status, 201);
    EXPECT_EQ(body(r), (json{{"id", "a:1"}, {"row", 0}}));
    EXPECT_EQ(t.call("POST", "/v1/chunks", kB).status, 201);
    EXPECT_EQ(body(t.call("GET", "/health"))["chunks"], 2);

    auto g = t.call("GET", "/v1/chunks/a:1");
    EXPECT_EQ(g.status, 200);
    json expected = json::parse(kA);
    expected["meta"] = json::object();  // meta is {} when the chunk had none
    EXPECT_EQ(body(g), expected);
    EXPECT_EQ(body(t.call("GET", "/v1/chunks/b%2F2"))["meta"]["fy"], "FY2025") << "percent-decoded id";
}

TEST(Api, PutErrors) {
    TestApi t;
    t.call("POST", "/v1/chunks", kA);
    auto dup = t.call("POST", "/v1/chunks", kA);
    EXPECT_EQ(dup.status, 409);
    EXPECT_TRUE(body(dup).contains("error"));
    EXPECT_EQ(t.call("POST", "/v1/chunks", "{oops").status, 400) << "invalid JSON";
    EXPECT_EQ(t.call("POST", "/v1/chunks", R"({"id": "x", "doc_id": "d", "text": "t", "vector": [1, 0]})").status, 400)
        << "wrong dimension (the Engine's std::invalid_argument)";
    EXPECT_EQ(t.call("POST", "/v1/chunks", "").status, 400) << "no body";
    EXPECT_EQ(body(t.call("GET", "/health"))["chunks"], 1);
}

TEST(Api, GetAnUnknownIdIs404) {
    TestApi t;
    auto r = t.call("GET", "/v1/chunks/nope");
    EXPECT_EQ(r.status, 404);
    EXPECT_TRUE(body(r).contains("error"));
}

TEST(Api, Search) {
    TestApi t;
    t.call("POST", "/v1/chunks", kA);
    t.call("POST", "/v1/chunks", kB);
    auto v = t.call("POST", "/v1/search", R"({"vector": [0.1, 1, 0], "k": 1})");
    EXPECT_EQ(v.status, 200);
    json j = body(v);
    ASSERT_EQ(j["hits"].size(), 1u);
    EXPECT_EQ(j["hits"][0]["id"], "b/2");
    EXPECT_EQ(j["hits"][0]["doc_id"], "d2");
    EXPECT_EQ(j["hits"][0]["text"], "revenue fell");
    EXPECT_TRUE(j["took_ms"].is_number());
    auto k = t.call("POST", "/v1/search", R"({"mode": "keyword", "text": "profit"})");
    EXPECT_EQ(body(k)["hits"][0]["id"], "a:1");
    auto f = t.call("POST", "/v1/search", R"({"vector": [1, 0, 0], "filter": {"meta": {"fy": "FY2025"}}})");
    ASSERT_EQ(body(f)["hits"].size(), 1u);
    EXPECT_EQ(body(f)["hits"][0]["id"], "b/2");
}

TEST(Api, BadSearchesAre400) {
    TestApi t;
    EXPECT_EQ(t.call("POST", "/v1/search", R"({"vector": [1, 0]})").status, 400) << "wrong dimension";
    EXPECT_EQ(t.call("POST", "/v1/search", R"({"vector": [1, 0, 0], "k": 0})").status, 400);
    EXPECT_EQ(t.call("POST", "/v1/search", R"({"mode": "keyword"})").status, 400);
    EXPECT_EQ(t.call("POST", "/v1/search", "nope").status, 400);
}

TEST(Api, UnknownPathsAndWrongMethods) {
    TestApi t;
    EXPECT_EQ(t.call("GET", "/nope").status, 404);
    auto r = t.call("GET", "/v1/search");
    EXPECT_EQ(r.status, 405);
    EXPECT_EQ(TestApi::header(r, "Allow"), "POST");
    EXPECT_EQ(t.call("DELETE", "/v1/chunks/a:1").status, 405);
}

TEST(Api, UnexpectedErrorsBecome500NotACrash) {
    TestApi t;
    t.call("POST", "/v1/chunks", kA);
    t.store->explode = true;  // every Store call now throws std::runtime_error
    auto r = t.call("GET", "/v1/chunks/a:1");
    EXPECT_EQ(r.status, 500);
    EXPECT_TRUE(body(r).contains("error"));
}
