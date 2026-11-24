#include "check.hpp"

#include <cerrno>
#include <system_error>

int check(int result, const char* what) {
    if (result == -1) {
        throw std::system_error(errno, std::generic_category(), what);
    }
    return result;
}

ssize_t check(ssize_t result, const char* what) {
    if (result == -1) {
        throw std::system_error(errno, std::generic_category(), what);
    }
    return result;
}
