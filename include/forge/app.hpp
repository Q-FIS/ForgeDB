// The forgedb program: its settings, the API's JSON, and the API itself.
// (component APP; issues APP-1, APP-2, APP-3)   The API is in docs/API.md.
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "forge/engine.hpp"
#include "forge/http.hpp"
#include "forge/server.hpp"
#include "forge/types.hpp"

namespace forge {

// ---- Settings, from the command line (APP-1) ----

struct Config {
    std::string host = "127.0.0.1";
    std::uint16_t port = 8080;
    std::filesystem::path data_dir = "forge-data";
    std::size_t dim = 0;            // required: the length of every vector
    std::size_t threads = 4;        // worker threads
    std::size_t queue = 256;        // tasks that may wait for a worker
    IoMode io = IoMode::threads;
    std::string fsync = "batch";    // "always", "batch" or "never" (WAL-3)
    std::chrono::milliseconds idle_timeout{10'000};
};

/// A bad command line. what() is the message to show the user.
struct UsageError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// Reads the arguments (without the program name), e.g.
///   {"--port", "9000", "--dim", "384", "--data-dir", "/tmp/x"}
/// Every flag and its range is in issue APP-1. Throws UsageError.
Config parse_args(std::span<const std::string_view> args);

/// The text `forgedb --help` prints.
std::string usage();

// ---- The API's JSON (APP-2). All throw std::invalid_argument on bad input ----

Chunk chunk_from_json(std::string_view body);
std::string chunk_to_json(const Chunk& chunk);
SearchRequest search_from_json(std::string_view body);
std::string hits_to_json(const std::vector<SearchHit>& hits, double took_ms);

// ---- The API (APP-3) ----

/// Every endpoint of docs/API.md, on top of an Engine.
class Api {
public:
    explicit Api(Engine& engine);

    /// Never throws: every error becomes a JSON error response
    /// (400, 404, 405, 409, 500 or 501).
    http::Response handle(const http::Request& request) const;

private:
    // APP-3: your members go here, e.g. an http::Router and the Engine&.
};

}  // namespace forge
