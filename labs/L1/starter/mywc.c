// mywc: print "<lines> <words> <bytes> <file>" using only open/read/write/close.
// See README.md, Part 2.
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Whitespace = space, \t, \n, \r, \v, \f.
static int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    (void)is_space;
    // TODO:
    // 1. exactly one argument, else a message and exit code 2
    // 2. open it; on error print "mywc: <file>: <reason>" and exit 1
    // 3. read() in a loop into a buffer; for each byte update lines, words, bytes
    //    (keep `in_word` OUTSIDE the read loop: a word can be split between two reads)
    // 4. close; snprintf the result into a small array, and write() it to fd 1
    return 1;
}
