// mycatpp: mycat again, in C++20, with an RAII class for file descriptors.
// Same behaviour as mycat (the same test checks it). See README.md, Part 3.
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <charconv>
#include <iostream>
#include <span>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

// Owns one open file descriptor, and closes it in the destructor.
class Fd {
public:
    Fd() = default;                       // empty: holds no file (fd_ = -1)
    explicit Fd(int fd) : fd_(fd) {}      // takes ownership of an open fd
    ~Fd();                                // closes fd_, if it's open
    Fd(Fd&& other) noexcept;              // move: take other's fd, leave other empty
    Fd& operator=(Fd&& other) noexcept;   // move-assign: close yours first, then take other's
    Fd(const Fd&) = delete;               // no copies (README, Part 3, step 5 shows why)
    Fd& operator=(const Fd&) = delete;
    int get() const { return fd_; }

private:
    int fd_ = -1;
};

Fd::~Fd() {
    // TODO
}

Fd::Fd(Fd&& other) noexcept {
    (void)other;
    // TODO (std::exchange helps)
}

Fd& Fd::operator=(Fd&& other) noexcept {
    (void)other;
    // TODO
    return *this;
}

// Opens path for reading. Throws std::system_error if it can't.
Fd open_read(const char* path) {
    (void)path;
    // TODO
    return Fd();
}

// Writes every byte of data to fd. Throws std::system_error on failure.
void write_all(int fd, std::span<const char> data) {
    (void)fd;
    (void)data;
    // TODO
}

// Copies everything from `in` to standard output, using buffer.
void copy_fd(int in, std::span<char> buffer) {
    (void)in;
    (void)buffer;
    // TODO
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    // TODO: like mycat, but:
    //  - read -b with std::from_chars (bad size: message + exit code 2)
    //  - the buffer is a std::vector<char>
    //  - try/catch (const std::system_error& e) around each file;
    //    print "mycat++: " << e.what() to std::cerr and carry on
    //  - no close() anywhere here: Fd does it
    return 1;
}
