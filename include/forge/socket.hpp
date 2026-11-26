// TCP sockets that close themselves.   (component CONN; issue CONN-1)
//
// You may add private members and helpers. Changing a public signature needs
// the lead's OK: other components (CLI, LOAD) call these functions.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace forge {

/// Owns one socket file descriptor and closes it in the destructor (RAII).
/// Move-only: two owners would close the same fd twice.
class Socket {
public:
    Socket() = default;                        // empty: fd() == -1
    explicit Socket(int fd) noexcept;          // takes ownership of fd
    ~Socket();                                 // closes the fd, if any
    Socket(Socket&& other) noexcept;           // takes other's fd; other becomes empty
    Socket& operator=(Socket&& other) noexcept;
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    int fd() const noexcept { return fd_; }
    bool valid() const noexcept { return fd_ != -1; }

private:
    int fd_ = -1;
};

/// socket + SO_REUSEADDR + bind + listen. `host` is an IPv4 address such as
/// "127.0.0.1" or "0.0.0.0" (std::invalid_argument if it isn't). Port 0 means
/// "any free port": ask local_port() which one you got.
Socket listen_tcp(const std::string& host, std::uint16_t port, int backlog = 128);

/// The local port a socket is bound to (getsockname).
std::uint16_t local_port(const Socket& socket);

/// Waits for the next client (accept). Retries on EINTR.
Socket accept_client(const Socket& listener);

/// Connects to host:port over IPv4. `host` may be a name ("localhost") or an
/// address. A name that doesn't resolve: std::runtime_error.
Socket connect_tcp(const std::string& host, std::uint16_t port);

/// Sends every byte (send can send fewer than asked: loop). Uses MSG_NOSIGNAL,
/// so a closed connection throws (EPIPE) instead of killing the process.
void send_all(const Socket& socket, std::string_view data);

/// One recv into `buffer`. Returns the number of bytes received, or 0 when
/// the other side closed the connection. Retries on EINTR.
std::size_t recv_some(const Socket& socket, std::span<char> buffer);

// All of these throw std::system_error when a system call fails.

}  // namespace forge
