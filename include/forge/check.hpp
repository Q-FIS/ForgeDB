// check(): turn a failed system call into an exception.   (owner: lead)
//
//     int fd = check(::socket(AF_INET, SOCK_STREAM, 0), "socket");
//
// Almost every system call returns -1 on failure and sets errno. check()
// returns the result unchanged, or throws std::system_error (which carries
// errno and a readable message, e.g. "socket: Too many open files").
#pragma once

#include <sys/types.h>

#include <cerrno>
#include <system_error>

namespace forge {

inline int check(int result, const char* what) {
    if (result == -1) throw std::system_error(errno, std::generic_category(), what);
    return result;
}

inline ssize_t check(ssize_t result, const char* what) {
    if (result == -1) throw std::system_error(errno, std::generic_category(), what);
    return result;
}

}  // namespace forge
