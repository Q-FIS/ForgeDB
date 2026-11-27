// A C++ client for ForgeDB.   (component CLI; issue CLI-1)
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "forge/socket.hpp"

namespace forge {

struct HttpReply {
    int status = 0;
    std::string body;
};

/// Talks HTTP/1.1 to a ForgeDB server over one keep-alive connection.
class Client {
public:
    /// Doesn't connect yet: the first request does.
    Client(std::string host, std::uint16_t port);

    /// Sends one request and reads the whole response. If the server had
    /// closed the kept-alive connection, reconnects once and retries.
    /// Throws std::system_error if it can't connect at all.
    HttpReply request(std::string_view method, std::string_view path, std::string_view body = {});

private:
    std::string host_;
    std::uint16_t port_;
    Socket socket_;
};

/// Percent-encodes an id for a URL path: every byte except A-Z a-z 0-9
/// - . _ ~ : becomes %XX (uppercase hex).
std::string url_encode(std::string_view text);

}  // namespace forge
