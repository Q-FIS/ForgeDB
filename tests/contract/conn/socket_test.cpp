// Contract tests for CONN-1: the Socket class and its helpers.   (owner: lead)
// Switched on when src/conn/stub_socket.cpp is deleted.
#include <fcntl.h>
#include <sys/socket.h>

#include <gtest/gtest.h>

#include <string>
#include <thread>
#include <type_traits>

#include "forge/socket.hpp"

using namespace forge;

namespace {

bool is_open(int fd) { return ::fcntl(fd, F_GETFD) != -1; }

int raw_socket() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    EXPECT_NE(fd, -1);
    return fd;
}

std::string recv_exactly(const Socket& s, std::size_t n) {
    std::string out;
    char buf[65536];
    while (out.size() < n) {
        std::size_t got = recv_some(s, buf);
        if (got == 0) break;
        out.append(buf, got);
    }
    return out;
}

}  // namespace

TEST(Socket, IsEmptyByDefault) {
    Socket s;
    EXPECT_FALSE(s.valid());
    EXPECT_EQ(s.fd(), -1);
}

TEST(Socket, ClosesItsFdWhenDestroyed) {
    int fd = raw_socket();
    {
        Socket s(fd);
        EXPECT_EQ(s.fd(), fd);
        EXPECT_TRUE(s.valid());
    }
    EXPECT_FALSE(is_open(fd)) << "the destructor must close the fd";
}

TEST(Socket, IsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<Socket>, "copying would close the fd twice");
    static_assert(!std::is_copy_assignable_v<Socket>);
    static_assert(std::is_nothrow_move_constructible_v<Socket>);
    static_assert(std::is_nothrow_move_assignable_v<Socket>);
}

TEST(Socket, MovingTransfersOwnership) {
    int fd = raw_socket();
    Socket a(fd);
    Socket b(std::move(a));
    EXPECT_FALSE(a.valid()) << "a moved-from Socket must be empty";
    EXPECT_EQ(b.fd(), fd);
    EXPECT_TRUE(is_open(fd));
}

TEST(Socket, MoveAssignmentClosesTheOldFd) {
    int fd1 = raw_socket(), fd2 = raw_socket();
    Socket a(fd1), b(fd2);
    b = std::move(a);
    EXPECT_FALSE(is_open(fd2)) << "b's old fd must be closed";
    EXPECT_EQ(b.fd(), fd1);
    EXPECT_TRUE(is_open(fd1));
    EXPECT_FALSE(a.valid());
}

TEST(Socket, MoveAssigningToItselfKeepsTheFd) {
    int fd = raw_socket();
    Socket a(fd);
    Socket& same = a;
    a = std::move(same);
    EXPECT_EQ(a.fd(), fd);
    EXPECT_TRUE(is_open(fd));
}

TEST(Socket, ListeningOnPortZeroGivesARealPort) {
    Socket listener = listen_tcp("127.0.0.1", 0);
    EXPECT_TRUE(listener.valid());
    EXPECT_NE(local_port(listener), 0);
}

TEST(Socket, ListeningRejectsAHostThatIsNotAnAddress) {
    EXPECT_THROW(listen_tcp("not-an-address", 0), std::invalid_argument);
}

TEST(Socket, ListeningOnAPortInUseThrows) {
    Socket first = listen_tcp("127.0.0.1", 0);
    EXPECT_THROW(listen_tcp("127.0.0.1", local_port(first)), std::system_error);
}

TEST(Socket, ConnectAcceptAndTalkBothWays) {
    Socket listener = listen_tcp("127.0.0.1", 0);
    std::thread server([&] {
        Socket client = accept_client(listener);
        std::string got = recv_exactly(client, 5);
        send_all(client, got + " back");
    });
    Socket c = connect_tcp("localhost", local_port(listener));  // a name, not an address
    send_all(c, "hello");
    EXPECT_EQ(recv_exactly(c, 10), "hello back");
    server.join();
}

TEST(Socket, RecvReturnsZeroWhenTheOtherSideCloses) {
    Socket listener = listen_tcp("127.0.0.1", 0);
    std::thread server([&] { Socket client = accept_client(listener); });  // accept, then close
    Socket c = connect_tcp("127.0.0.1", local_port(listener));
    server.join();
    char buf[16];
    EXPECT_EQ(recv_some(c, buf), 0u);
}

TEST(Socket, SendAllDeliversTenMegabytesExactly) {
    std::string data(10 * 1024 * 1024, '\0');
    for (std::size_t i = 0; i < data.size(); ++i) data[i] = static_cast<char>(i * 7 + i / 4096);
    Socket listener = listen_tcp("127.0.0.1", 0);
    std::string received;
    std::thread server([&] {
        Socket client = accept_client(listener);
        char buf[65536];
        while (std::size_t n = recv_some(client, buf)) received.append(buf, n);
    });
    {
        Socket c = connect_tcp("127.0.0.1", local_port(listener));
        send_all(c, data);
    }  // closing makes the server's recv_some return 0
    server.join();
    EXPECT_EQ(received.size(), data.size());
    EXPECT_TRUE(received == data) << "the bytes must arrive in order, none lost";
}

TEST(Socket, SendingToAClosedConnectionThrowsInsteadOfKillingTheProcess) {
    // Without MSG_NOSIGNAL, the kernel sends SIGPIPE and this test program dies.
    Socket listener = listen_tcp("127.0.0.1", 0);
    std::thread server([&] { Socket client = accept_client(listener); });
    Socket c = connect_tcp("127.0.0.1", local_port(listener));
    server.join();
    std::string chunk(64 * 1024, 'x');
    EXPECT_THROW(
        {
            for (int i = 0; i < 1000; ++i) send_all(c, chunk);
        },
        std::system_error);
}

TEST(Socket, ConnectingToAPortWithNobodyListeningThrows) {
    std::uint16_t port;
    {
        Socket listener = listen_tcp("127.0.0.1", 0);
        port = local_port(listener);
    }  // closed: nobody listens there now
    EXPECT_THROW(connect_tcp("127.0.0.1", port), std::system_error);
}
