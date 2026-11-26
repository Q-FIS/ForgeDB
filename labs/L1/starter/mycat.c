// mycat: print files to standard output using only open/read/write/close.
// See README.md, Part 1.
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Write all n bytes of buf to fd. write() may write fewer bytes than asked,
// so loop until everything is out. Returns 0 on success, -1 on error.
static int write_all(int fd, const char *buf, size_t n) {
    (void)fd;
    (void)buf;
    (void)n;
    // TODO
    return -1;
}

// Copy everything from in_fd to standard output (fd 1), using buf (size bytes).
// Returns 0 on success, -1 on error (errno says why).
static int copy_fd(int in_fd, char *buf, size_t size) {
    (void)in_fd;
    (void)buf;
    (void)size;
    // TODO: loop read() until it returns 0; write_all() exactly what you read.
    return -1;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    (void)copy_fd;
    (void)write_all;
    // TODO:
    // 1. read "-b SIZE" if it's there (strtol); a bad size: message + exit code 2
    // 2. malloc the buffer
    // 3. no files: copy standard input (fd 0)
    // 4. each file: open, copy_fd, close; on any error print
    //    "mycat: <file>: <reason>" to stderr and carry on
    // 5. free; exit code 1 if anything failed, else 0
    return 1;
}
