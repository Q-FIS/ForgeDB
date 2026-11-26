// A tiny HTTP client made of raw system calls, for tests ONLY.   (owner: lead)
//
// The server's tests use it instead of CONN's Socket, so they test the server
// and nothing else.
#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace forge::testing {

struct RawResponse {
    int status = 0;
    std::map<std::string, std::string> headers;  // names lowercased
    std::string body;
};

/// One TCP connection to 127.0.0.1:port, with a 5-second receive timeout.
class RawConnection {
public:
    explicit RawConnection(std::uint16_t port) {
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) throw std::runtime_error("socket failed");
        timeval tv{5, 0};
        ::setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0) {
            ::close(fd_);
            throw std::runtime_error("connect failed");
        }
    }
    ~RawConnection() { ::close(fd_); }
    RawConnection(const RawConnection&) = delete;
    RawConnection& operator=(const RawConnection&) = delete;

    void send(std::string_view data) {
        while (!data.empty()) {
            ssize_t n = ::send(fd_, data.data(), data.size(), MSG_NOSIGNAL);
            if (n <= 0) throw std::runtime_error("send failed");
            data.remove_prefix(static_cast<std::size_t>(n));
        }
    }

    /// Reads one whole response (status line, headers, Content-Length body).
    RawResponse read_response() {
        std::size_t end;
        while ((end = buf_.find("\r\n\r\n")) == std::string::npos) fill("the headers");
        RawResponse r;
        std::string head = buf_.substr(0, end);
        buf_.erase(0, end + 4);
        std::size_t line_end = head.find("\r\n");
        std::string status_line = head.substr(0, line_end);
        if (status_line.rfind("HTTP/1.1 ", 0) != 0) throw std::runtime_error("bad status line: " + status_line);
        r.status = std::stoi(status_line.substr(9, 3));
        std::size_t pos = line_end == std::string::npos ? head.size() : line_end + 2;
        while (pos < head.size()) {
            std::size_t next = head.find("\r\n", pos);
            if (next == std::string::npos) next = head.size();
            std::string line = head.substr(pos, next - pos);
            std::size_t colon = line.find(':');
            std::string name = line.substr(0, colon);
            for (char& c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            std::string value = colon == std::string::npos ? "" : line.substr(colon + 1);
            while (!value.empty() && value.front() == ' ') value.erase(0, 1);
            r.headers[name] = value;
            pos = next + 2;
        }
        std::size_t length = r.headers.count("content-length") ? std::stoul(r.headers["content-length"]) : 0;
        while (buf_.size() < length) fill("the body");
        r.body = buf_.substr(0, length);
        buf_.erase(0, length);
        return r;
    }

    /// True if the server closed the connection (recv returns 0) within 5 s.
    bool closed_by_server() {
        char c;
        if (!buf_.empty()) return false;
        ssize_t n = ::recv(fd_, &c, 1, 0);
        return n == 0 || (n < 0 && errno == ECONNRESET);
    }

private:
    void fill(const char* what) {
        char tmp[65536];
        ssize_t n = ::recv(fd_, tmp, sizeof tmp, 0);
        if (n <= 0) throw std::runtime_error(std::string("connection closed or timed out while reading ") + what);
        buf_.append(tmp, static_cast<std::size_t>(n));
    }
    int fd_ = -1;
    std::string buf_;
};

}  // namespace forge::testing
