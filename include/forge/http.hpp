// HTTP/1.1: requests, responses, routing.   (component HTTP; issues HTTP-1, HTTP-2, HTTP-3)
//
// You may add private members and helpers. Changing a public signature needs
// the lead's OK: CONN and APP call these.
#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace forge::http {

struct Header {
    std::string name;
    std::string value;

    bool operator==(const Header&) const = default;
};

struct Request {
    std::string method;   // "GET", "POST", ...
    std::string target;   // "/v1/search?debug=1": the path, plus the query if any
    std::string version;  // "HTTP/1.1" or "HTTP/1.0"
    std::vector<Header> headers;
    std::string body;

    /// The value of the first header with this name (case-insensitive), or "".
    std::string_view header(std::string_view name) const;
    /// The target without the query: "/v1/search".
    std::string_view path() const;
    /// The part after '?', or "".
    std::string_view query() const;
    /// Should the connection stay open after the response? HTTP/1.1: yes,
    /// unless "Connection: close". HTTP/1.0: no, unless "Connection: keep-alive".
    bool keep_alive() const;
};

// ---- Parsing (HTTP-1, limits in HTTP-4) ----

enum class ParseStatus {
    complete,   // `request` is filled in; `consumed` bytes belong to it
    need_more,  // the request isn't all here yet: read more bytes and try again
    error,      // the bytes are not a valid request: reply `error_status` and close
};

struct ParseResult {
    ParseStatus status = ParseStatus::need_more;
    std::size_t consumed = 0;  // when complete: how many bytes the request used
    Request request;           // when complete
    int error_status = 0;      // when error: 400, 413, 431 or 501
    std::string error_message; // when error: a short reason, for the response body
};

struct Limits {
    std::size_t max_header_bytes = 8 * 1024;    // request line + headers; more -> 431
    std::size_t max_body_bytes = 1024 * 1024;   // Content-Length above this -> 413
};

/// Parses ONE request from the start of `bytes`. Bytes after it (a second,
/// pipelined request) are left alone: see `consumed`.
ParseResult parse_request(std::string_view bytes, const Limits& limits = {});

// ---- Responses (HTTP-2) ----

struct Response {
    int status = 200;
    std::vector<Header> headers;  // don't add Content-Length or Connection: serialize() does
    std::string body;

    /// A response with "Content-Type: application/json".
    static Response json(int status, std::string body);
    /// A response with "Content-Type: text/plain; charset=utf-8".
    static Response text(int status, std::string body);
    /// A JSON error: {"error": "<message>"}, with the message escaped for JSON.
    static Response error(int status, std::string_view message);
};

/// "OK" for 200, "Not Found" for 404, ...; "Unknown" for codes it doesn't know.
std::string_view reason_phrase(int status);

/// The exact bytes to send:
///   HTTP/1.1 <status> <reason>\r\n
///   <each header in response.headers, in order>\r\n
///   Content-Length: <body size>\r\n
///   Connection: keep-alive (or close)\r\n
///   \r\n
///   <body>
std::string serialize(const Response& response, bool keep_alive);

// ---- Routing (HTTP-3) ----

/// Values taken from the path, e.g. {"id": "abc"} for "/v1/chunks/{id}".
/// Values are percent-decoded ("%3A" becomes ":").
using Params = std::map<std::string, std::string, std::less<>>;
using RouteHandler = std::function<Response(const Request&, const Params&)>;

class Router {
public:
    /// `pattern` is a path whose segments are either literal ("v1") or a
    /// parameter in braces ("{id}"), e.g. "/v1/chunks/{id}".
    void add(std::string method, std::string pattern, RouteHandler handler);

    /// Calls the first route (in the order added) whose method and pattern
    /// match. No pattern matches the path: 404. A pattern matches but not
    /// the method: 405, with an "Allow" header listing the methods that
    /// would work ("GET, POST", in the order added). A bad percent-escape in
    /// a parameter: 400. Exceptions from a handler are not caught here.
    Response handle(const Request& request) const;

private:
    struct Route {
        std::string method;
        std::vector<std::string> segments;
        RouteHandler handler;
    };
    std::vector<Route> routes_;
};

}  // namespace forge::http
