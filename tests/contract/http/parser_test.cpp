// Contract tests for HTTP-1: parse_request and the Request helpers.   (owner: lead)
// Switched on when src/http/stub_parser.cpp is deleted.
// (The limits, 413/431/501, are HTTP-4: you write those tests.)
#include <gtest/gtest.h>

#include <string>

#include "forge/http.hpp"

using namespace forge::http;

namespace {

ParseResult parse(std::string_view s) { return parse_request(s); }

void expect_error(std::string_view bytes, const char* why) {
    ParseResult r = parse(bytes);
    EXPECT_EQ(r.status, ParseStatus::error) << why << ": " << bytes;
    EXPECT_EQ(r.error_status, 400) << why;
    EXPECT_FALSE(r.error_message.empty()) << why << ": the error needs a message";
}

}  // namespace

TEST(Parser, ASimpleGet) {
    std::string raw = "GET /v1/chunks/abc HTTP/1.1\r\nHost: localhost\r\nAccept: */*\r\n\r\n";
    ParseResult r = parse(raw);
    ASSERT_EQ(r.status, ParseStatus::complete);
    EXPECT_EQ(r.consumed, raw.size());
    EXPECT_EQ(r.request.method, "GET");
    EXPECT_EQ(r.request.target, "/v1/chunks/abc");
    EXPECT_EQ(r.request.version, "HTTP/1.1");
    ASSERT_EQ(r.request.headers.size(), 2u);
    EXPECT_EQ(r.request.headers[0], (Header{"Host", "localhost"}));
    EXPECT_EQ(r.request.headers[1], (Header{"Accept", "*/*"}));
    EXPECT_EQ(r.request.body, "");
}

TEST(Parser, APostWithABody) {
    std::string raw = "POST /v1/search HTTP/1.1\r\nContent-Length: 13\r\n\r\n{\"k\": 5, \"\":1}";
    ParseResult r = parse(raw);
    ASSERT_EQ(r.status, ParseStatus::complete);
    EXPECT_EQ(r.request.body, "{\"k\": 5, \"\":1");
    EXPECT_EQ(r.consumed, raw.size() - 1) << "the byte after the body belongs to the next request";
}

TEST(Parser, HeaderValuesAreTrimmedAndMayBeEmpty) {
    ParseResult r = parse("GET / HTTP/1.1\r\nX-A:   a b \t\r\nX-Empty:\r\n\r\n");
    ASSERT_EQ(r.status, ParseStatus::complete);
    EXPECT_EQ(r.request.headers[0].value, "a b");
    EXPECT_EQ(r.request.headers[1].value, "");
}

TEST(Parser, EveryPrefixOfARequestNeedsMore) {
    std::string raw = "POST /x HTTP/1.1\r\nHost: a\r\nContent-Length: 5\r\n\r\nhello";
    for (std::size_t n = 0; n < raw.size(); ++n) {
        EXPECT_EQ(parse(raw.substr(0, n)).status, ParseStatus::need_more) << "first " << n << " bytes";
    }
    EXPECT_EQ(parse(raw).status, ParseStatus::complete);
}

TEST(Parser, TwoPipelinedRequests) {
    std::string first = "GET /a HTTP/1.1\r\n\r\n";
    std::string second = "POST /b HTTP/1.1\r\nContent-Length: 2\r\n\r\nhi";
    std::string both = first + second;
    ParseResult r1 = parse(both);
    ASSERT_EQ(r1.status, ParseStatus::complete);
    EXPECT_EQ(r1.consumed, first.size());
    EXPECT_EQ(r1.request.target, "/a");
    ParseResult r2 = parse(std::string_view(both).substr(r1.consumed));
    ASSERT_EQ(r2.status, ParseStatus::complete);
    EXPECT_EQ(r2.request.body, "hi");
    EXPECT_EQ(r2.consumed, second.size());
}

TEST(Parser, Http10IsAccepted) {
    ParseResult r = parse("GET / HTTP/1.0\r\n\r\n");
    ASSERT_EQ(r.status, ParseStatus::complete);
    EXPECT_EQ(r.request.version, "HTTP/1.0");
}

TEST(Parser, BadRequestsAre400) {
    expect_error("GET /\r\n\r\n", "no version");
    expect_error("GET / HTTP/1.1 extra\r\n\r\n", "too many parts in the request line");
    expect_error("get / HTTP/1.1\r\n\r\n", "methods are uppercase letters");
    expect_error(" / HTTP/1.1\r\n\r\n", "empty method");
    expect_error("GET index.html HTTP/1.1\r\n\r\n", "the target must start with /");
    expect_error("GET / HTTP/2.0\r\n\r\n", "only HTTP/1.0 and HTTP/1.1");
    expect_error("GET / http/1.1\r\n\r\n", "the version is case-sensitive");
    expect_error("GET / HTTP/1.1\r\nNoColonHere\r\n\r\n", "a header line without ':'");
    expect_error("GET / HTTP/1.1\r\nBad Name: x\r\n\r\n", "a space in a header name");
    expect_error("GET / HTTP/1.1\r\n: x\r\n\r\n", "an empty header name");
    expect_error("GET / HTTP/1.1\r\nA: b\r\n  folded\r\n\r\n", "a line starting with a space (old 'folding')");
    expect_error("POST / HTTP/1.1\r\nContent-Length: abc\r\n\r\n", "Content-Length not a number");
    expect_error("POST / HTTP/1.1\r\nContent-Length: -5\r\n\r\n", "a negative Content-Length");
    expect_error("POST / HTTP/1.1\r\nContent-Length: 1 2\r\n\r\n", "two numbers in Content-Length");
}

TEST(Request, HeaderLookupIgnoresCase) {
    ParseResult r = parse("GET / HTTP/1.1\r\ncontent-TYPE: text/plain\r\nX: 1\r\nX: 2\r\n\r\n");
    ASSERT_EQ(r.status, ParseStatus::complete);
    EXPECT_EQ(r.request.header("Content-Type"), "text/plain");
    EXPECT_EQ(r.request.header("x"), "1") << "the first one wins";
    EXPECT_EQ(r.request.header("Missing"), "");
}

TEST(Request, PathAndQuery) {
    Request r;
    r.target = "/v1/search?debug=1&x";
    EXPECT_EQ(r.path(), "/v1/search");
    EXPECT_EQ(r.query(), "debug=1&x");
    r.target = "/health";
    EXPECT_EQ(r.path(), "/health");
    EXPECT_EQ(r.query(), "");
    r.target = "/a?";
    EXPECT_EQ(r.path(), "/a");
    EXPECT_EQ(r.query(), "");
}

TEST(Request, KeepAliveRules) {
    auto keep = [](std::string version, std::string connection) {
        Request r;
        r.version = version;
        if (!connection.empty()) r.headers.push_back({"Connection", connection});
        return r.keep_alive();
    };
    EXPECT_TRUE(keep("HTTP/1.1", ""));
    EXPECT_FALSE(keep("HTTP/1.1", "close"));
    EXPECT_FALSE(keep("HTTP/1.1", "Close"));
    EXPECT_TRUE(keep("HTTP/1.1", "keep-alive"));
    EXPECT_FALSE(keep("HTTP/1.0", ""));
    EXPECT_TRUE(keep("HTTP/1.0", "Keep-Alive"));
}
