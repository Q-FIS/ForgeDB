// Contract tests for CONN-2: the blocking HTTP server.   (owner: lead)
// Switched on when src/conn/stub_server.cpp is deleted. Needs HTTP-1 and HTTP-2.
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "forge/server.hpp"
#include "support/raw_http.hpp"

using namespace forge;
using forge::testing::RawConnection;
using namespace std::chrono_literals;

namespace {

/// Runs every task on a new thread; joins them all when destroyed.
class SpawnExecutor final : public Executor {
public:
    ~SpawnExecutor() override {
        std::lock_guard lock(mutex_);
        for (auto& t : threads_) t.join();
    }
    bool submit(std::function<void()> task) override {
        std::lock_guard lock(mutex_);
        threads_.emplace_back(std::move(task));
        return true;
    }

private:
    std::mutex mutex_;
    std::vector<std::thread> threads_;
};

/// Refuses every task, like a full thread pool.
class FullExecutor final : public Executor {
public:
    bool submit(std::function<void()>) override { return false; }
};

/// Replies "<method> <target> <body>", or throws if the path is /throw.
http::Response echo(const http::Request& r) {
    if (r.path() == "/throw") throw std::runtime_error("handler failed on purpose");
    http::Response res;
    res.status = 200;
    res.body = r.method + " " + r.target + " " + r.body;
    return res;
}

/// A Server running on its own thread, on a free port; stopped at the end.
struct RunningServer {
    explicit RunningServer(Executor& ex, Handler handler = echo, std::chrono::milliseconds idle = 2000ms)
        : server(make_options(idle), std::move(handler), ex), thread([this] { server.run(); }) {}
    ~RunningServer() {
        server.stop();
        thread.join();
    }
    static ServerOptions make_options(std::chrono::milliseconds idle) {
        ServerOptions o;
        o.port = 0;
        o.idle_timeout = idle;
        return o;
    }
    std::uint16_t port() const { return server.port(); }
    Server server;
    std::thread thread;
};

}  // namespace

TEST(Server, PortZeroGivesARealPortBeforeRun) {
    InlineExecutor ex;
    ServerOptions o;
    o.port = 0;
    Server s(o, echo, ex);
    EXPECT_NE(s.port(), 0);
}

TEST(Server, AnswersAGet) {
    SpawnExecutor ex;
    RunningServer srv(ex);
    RawConnection c(srv.port());
    c.send("GET /hello?x=1 HTTP/1.1\r\nHost: x\r\n\r\n");
    auto r = c.read_response();
    EXPECT_EQ(r.status, 200);
    EXPECT_EQ(r.body, "GET /hello?x=1 ");
}

TEST(Server, PassesTheBodyOfAPost) {
    SpawnExecutor ex;
    RunningServer srv(ex);
    RawConnection c(srv.port());
    c.send("POST /data HTTP/1.1\r\nContent-Length: 11\r\n\r\nhello world");
    EXPECT_EQ(c.read_response().body, "POST /data hello world");
}

TEST(Server, KeepsTheConnectionOpenForMoreRequests) {
    SpawnExecutor ex;
    RunningServer srv(ex);
    RawConnection c(srv.port());
    for (int i = 0; i < 3; ++i) {
        c.send("GET /n" + std::to_string(i) + " HTTP/1.1\r\n\r\n");
        auto r = c.read_response();
        EXPECT_EQ(r.body, "GET /n" + std::to_string(i) + " ");
        EXPECT_EQ(r.headers["connection"], "keep-alive");
    }
}

TEST(Server, ClosesAfterConnectionClose) {
    SpawnExecutor ex;
    RunningServer srv(ex);
    RawConnection c(srv.port());
    c.send("GET / HTTP/1.1\r\nConnection: close\r\n\r\n");
    auto r = c.read_response();
    EXPECT_EQ(r.status, 200);
    EXPECT_EQ(r.headers["connection"], "close");
    EXPECT_TRUE(c.closed_by_server());
}

TEST(Server, PutsARequestTogetherFromPieces) {
    SpawnExecutor ex;
    RunningServer srv(ex);
    RawConnection c(srv.port());
    for (const char* piece : {"POST /pie", "ces HTTP/1.1\r\nContent-Len", "gth: 4\r\n\r\nab", "cd"}) {
        c.send(piece);
        std::this_thread::sleep_for(50ms);
    }
    EXPECT_EQ(c.read_response().body, "POST /pieces abcd");
}

TEST(Server, AnswersPipelinedRequestsInOrder) {
    SpawnExecutor ex;
    RunningServer srv(ex);
    RawConnection c(srv.port());
    c.send("GET /a HTTP/1.1\r\n\r\nPOST /b HTTP/1.1\r\nContent-Length: 1\r\n\r\nxGET /c HTTP/1.1\r\n\r\n");
    EXPECT_EQ(c.read_response().body, "GET /a ");
    EXPECT_EQ(c.read_response().body, "POST /b x");
    EXPECT_EQ(c.read_response().body, "GET /c ");
}

TEST(Server, AnswersABadRequestWith400AndCloses) {
    SpawnExecutor ex;
    RunningServer srv(ex);
    RawConnection c(srv.port());
    c.send("THIS IS NOT HTTP\r\n\r\n");
    EXPECT_EQ(c.read_response().status, 400);
    EXPECT_TRUE(c.closed_by_server());
}

TEST(Server, AHandlerThatThrowsGives500AndTheServerKeepsGoing) {
    SpawnExecutor ex;
    RunningServer srv(ex);
    {
        RawConnection c(srv.port());
        c.send("GET /throw HTTP/1.1\r\n\r\n");
        EXPECT_EQ(c.read_response().status, 500);
    }
    RawConnection c(srv.port());
    c.send("GET /fine HTTP/1.1\r\n\r\n");
    EXPECT_EQ(c.read_response().status, 200);
}

TEST(Server, WhenTheExecutorIsFullItAnswers503AndCloses) {
    FullExecutor ex;
    RunningServer srv(ex);
    RawConnection c(srv.port());
    c.send("GET / HTTP/1.1\r\n\r\n");
    auto r = c.read_response();
    EXPECT_EQ(r.status, 503);
    EXPECT_EQ(r.headers["retry-after"], "1");
    EXPECT_TRUE(c.closed_by_server());
}

TEST(Server, ClosesAConnectionThatStaysSilent) {
    SpawnExecutor ex;
    RunningServer srv(ex, echo, 300ms);
    RawConnection c(srv.port());
    auto start = std::chrono::steady_clock::now();
    EXPECT_TRUE(c.closed_by_server()) << "an idle connection must be closed after idle_timeout";
    EXPECT_LT(std::chrono::steady_clock::now() - start, 3s);
}

TEST(Server, ServesManyClientsAtOnce) {
    SpawnExecutor ex;
    RunningServer srv(ex);
    std::atomic<int> ok{0};
    std::vector<std::thread> clients;
    for (int t = 0; t < 10; ++t) {
        clients.emplace_back([&, t] {
            RawConnection c(srv.port());
            for (int i = 0; i < 20; ++i) {
                std::string path = "/c" + std::to_string(t) + "/" + std::to_string(i);
                c.send("GET " + path + " HTTP/1.1\r\n\r\n");
                if (c.read_response().body == "GET " + path + " ") ++ok;
            }
        });
    }
    for (auto& t : clients) t.join();
    EXPECT_EQ(ok, 200);
}

TEST(Server, StopMakesRunReturn) {
    SpawnExecutor ex;
    ServerOptions o;
    o.port = 0;
    o.idle_timeout = 300ms;
    Server s(o, echo, ex);
    auto done = std::async(std::launch::async, [&] { s.run(); });
    std::this_thread::sleep_for(100ms);
    s.stop();
    EXPECT_EQ(done.wait_for(3s), std::future_status::ready) << "run() must return soon after stop()";
}
