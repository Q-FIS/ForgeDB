#pragma once
// check(): turn a failed system call into a C++ exception that says what failed.
//
// System calls return -1 on failure and put the reason in `errno`.
// Wrap every one of them, so no failure is ever silently ignored:
//
//     int fd = check(::open(path, O_RDONLY), "open");
//     ssize_t n = check(::read(fd, buf, sizeof buf), "read");
//
// If the call failed, this throws std::system_error. Its .what() reads like
// "open: No such file or directory", and its .code().value() is the errno.
#include <sys/types.h>

int check(int result, const char* what);
ssize_t check(ssize_t result, const char* what);
