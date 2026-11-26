// Contract tests for HTTP-2: responses.   (owner: lead)
// Switched on when src/http/stub_response.cpp is deleted.
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "forge/http.hpp"

using namespace forge::http;

TEST(Response, ReasonPhrases) {
    EXPECT_EQ(reason_phrase(200), "OK");
    EXPECT_EQ(reason_phrase(201), "Created");
    EXPECT_EQ(reason_phrase(400), "Bad Request");
    EXPECT_EQ(reason_phrase(404), "Not Found");
    EXPECT_EQ(reason_phrase(405), "Method Not Allowed");
    EXPECT_EQ(reason_phrase(409), "Conflict");
    EXPECT_EQ(reason_phrase(413), "Content Too Large");
    EXPECT_EQ(reason_phrase(431), "Request Header Fields Too Large");
    EXPECT_EQ(reason_phrase(500), "Internal Server Error");
    EXPECT_EQ(reason_phrase(501), "Not Implemented");
    EXPECT_EQ(reason_phrase(503), "Service Unavailable");
    EXPECT_EQ(reason_phrase(299), "Unknown");
}

TEST(Response, SerializeWritesTheExactBytes) {
    Response r;
    r.status = 200;
    r.headers = {{"Content-Type", "text/plain"}, {"X-Two", "2"}};
    r.body = "hi";
    EXPECT_EQ(serialize(r, true),
              "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nX-Two: 2\r\n"
              "Content-Length: 2\r\nConnection: keep-alive\r\n\r\nhi");
    EXPECT_EQ(serialize(r, false),
              "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nX-Two: 2\r\n"
              "Content-Length: 2\r\nConnection: close\r\n\r\nhi");
}

TEST(Response, AnEmptyBodyHasLengthZero) {
    Response r;
    r.status = 404;
    EXPECT_EQ(serialize(r, false), "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
}

TEST(Response, BinaryBodiesAreKeptExactly) {
    Response r;
    r.body = std::string("a\0b\r\n\r\nc", 8);
    std::string out = serialize(r, true);
    EXPECT_NE(out.find("Content-Length: 8\r\n"), std::string::npos);
    EXPECT_EQ(out.substr(out.size() - 8), r.body);
}

TEST(Response, JsonAndTextHelpers) {
    Response j = Response::json(201, "{\"id\":\"a\"}");
    EXPECT_EQ(j.status, 201);
    EXPECT_EQ(j.body, "{\"id\":\"a\"}");
    ASSERT_EQ(j.headers.size(), 1u);
    EXPECT_EQ(j.headers[0], (Header{"Content-Type", "application/json"}));
    Response t = Response::text(200, "ok");
    ASSERT_EQ(t.headers.size(), 1u);
    EXPECT_EQ(t.headers[0], (Header{"Content-Type", "text/plain; charset=utf-8"}));
}

TEST(Response, ErrorIsValidJsonWhateverTheMessage) {
    std::string message = "no chunk \"a\\b\"\nline two\ttab \x01 control, and ₹ stays";
    Response e = Response::error(404, message);
    EXPECT_EQ(e.status, 404);
    ASSERT_EQ(e.headers.size(), 1u);
    EXPECT_EQ(e.headers[0], (Header{"Content-Type", "application/json"}));
    nlohmann::json parsed;
    ASSERT_NO_THROW(parsed = nlohmann::json::parse(e.body)) << "not valid JSON: " << e.body;
    EXPECT_EQ(parsed, (nlohmann::json{{"error", message}}));
}
