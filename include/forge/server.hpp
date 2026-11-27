// The HTTP server: connections in, requests to the handler, responses out.
// (component CONN; issues CONN-2, CONN-3, CONN-5)
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include "forge/executor.hpp"
#include "forge/http.hpp"

namespace forge {

enum class IoMode {
    threads,  // blocking sockets, one pool task per connection (CONN-2)
    epoll,    // one event-loop thread for all connections (CONN-5)
};

struct ServerOptions {
    std::string host = "127.0.0.1";
    std::uint16_t port = 8080;  // 0: any free port (see Server::port())
    IoMode io = IoMode::threads;
    std::chrono::milliseconds idle_timeout{10'000};  // close a connection silent for this long
    std::size_t max_connections = 1024;              // more at once: the newest is refused (CONN-3)
    http::Limits limits;
};

/// Turns one request into one response. Runs on the Executor's threads, so
/// it must be safe to call from several threads at once.
using Handler = std::function<http::Response(const http::Request&)>;

class Server {
public:
    /// Binds and starts listening right away (so port() works before run()).
    /// Throws std::system_error if it can't (e.g. the port is taken).
    Server(ServerOptions options, Handler handler, Executor& executor);
    ~Server();
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /// Serves clients until stop() is called, then returns.
    void run();

    /// Makes run() return soon. Safe to call from any thread, and twice.
    void stop();

    /// The port it's listening on: the real one, even if options.port was 0.
    std::uint16_t port() const;

private:
    // CONN-2: your members go here.
};

}  // namespace forge
