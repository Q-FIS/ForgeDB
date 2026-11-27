// Contract tests for HTTP-3: the Router.   (owner: lead)
// Switched on when src/http/stub_router.cpp is deleted. Needs HTTP-2.
#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "forge/http.hpp"

using namespace forge::http;

namespace {

Request req(std::string method, std::string target) {
    Request r;
    r.method = std::move(method);
    r.target = std::move(target);
    r.version = "HTTP/1.1";
    return r;
}

/// Each handler replies with its name and the params it got: "get-chunk id=abc".
RouteHandler named(std::string name) {
    return [name](const Request&, const Params& params) {
        std::string body = name;
        for (const auto& [k, v] : params) body += " " + k + "=" + v;
        return Response::text(200, body);
    };
}

std::string header(const Response& r, std::string_view name) {
    for (const auto& h : r.headers)
        if (h.name == name) return h.value;
    return "";
}

Router api() {
    Router r;
    r.add("GET", "/health", named("health"));
    r.add("POST", "/v1/chunks", named("put"));
    r.add("GET", "/v1/chunks/{id}", named("get-chunk"));
    r.add("POST", "/v1/search", named("search"));
    r.add("DELETE", "/v1/chunks/{id}", named("delete-chunk"));
    r.add("GET", "/v1/docs/{doc}/chunks/{n}", named("two-params"));
    return r;
}

}  // namespace

TEST(Router, CallsTheMatchingRoute) {
    Router r = api();
    EXPECT_EQ(r.handle(req("GET", "/health")).body, "health");
    EXPECT_EQ(r.handle(req("POST", "/v1/chunks")).body, "put");
    EXPECT_EQ(r.handle(req("POST", "/v1/search")).body, "search");
}

TEST(Router, TakesParametersFromThePath) {
    Router r = api();
    EXPECT_EQ(r.handle(req("GET", "/v1/chunks/abc")).body, "get-chunk id=abc");
    EXPECT_EQ(r.handle(req("DELETE", "/v1/chunks/abc")).body, "delete-chunk id=abc");
    EXPECT_EQ(r.handle(req("GET", "/v1/docs/infy/chunks/7")).body, "two-params doc=infy n=7");
}

TEST(Router, PercentDecodesParameters) {
    Router r = api();
    EXPECT_EQ(r.handle(req("GET", "/v1/chunks/infosys%3Ap12")).body, "get-chunk id=infosys:p12");
    EXPECT_EQ(r.handle(req("GET", "/v1/chunks/a%2Fb%20c")).body, "get-chunk id=a/b c");
    EXPECT_EQ(r.handle(req("GET", "/v1/chunks/%e2%82%b9")).body, "get-chunk id=₹");
}

TEST(Router, ABadPercentEscapeIs400) {
    Router r = api();
    EXPECT_EQ(r.handle(req("GET", "/v1/chunks/%zz")).status, 400);
    EXPECT_EQ(r.handle(req("GET", "/v1/chunks/abc%4")).status, 400);
}

TEST(Router, IgnoresTheQuery) {
    EXPECT_EQ(api().handle(req("GET", "/health?verbose=1")).body, "health");
}

TEST(Router, UnknownPathsAre404) {
    Router r = api();
    for (const char* path : {"/nope", "/health/", "/v1/chunks/a/b", "/v1/chunks/", "/v1", "/"}) {
        Response res = r.handle(req("GET", path));
        EXPECT_EQ(res.status, 404) << path;
        EXPECT_NE(res.body.find("\"error\""), std::string::npos) << "a JSON error body, for " << path;
    }
}

TEST(Router, AWrongMethodIs405WithAnAllowHeader) {
    Router r = api();
    Response a = r.handle(req("PUT", "/v1/chunks/abc"));
    EXPECT_EQ(a.status, 405);
    EXPECT_EQ(header(a, "Allow"), "GET, DELETE") << "the methods that would work, in the order added";
    Response b = r.handle(req("GET", "/v1/search"));
    EXPECT_EQ(b.status, 405);
    EXPECT_EQ(header(b, "Allow"), "POST");
}

TEST(Router, MethodsAreCaseSensitive) {
    EXPECT_EQ(api().handle(req("get", "/health")).status, 405);
}

TEST(Router, TheFirstMatchingRouteWins) {
    Router r;
    r.add("GET", "/v1/chunks/latest", named("literal"));
    r.add("GET", "/v1/chunks/{id}", named("param"));
    EXPECT_EQ(r.handle(req("GET", "/v1/chunks/latest")).body, "literal");
    EXPECT_EQ(r.handle(req("GET", "/v1/chunks/other")).body, "param id=other");
}

TEST(Router, ExceptionsFromAHandlerAreNotCaught) {
    Router r;
    r.add("GET", "/boom", [](const Request&, const Params&) -> Response { throw std::runtime_error("boom"); });
    EXPECT_THROW(r.handle(req("GET", "/boom")), std::runtime_error);
}
