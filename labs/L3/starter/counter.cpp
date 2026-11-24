// counter MODE THREADS ITERS: see README.md.
#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "usage: counter none|mutex|atomic THREADS ITERS\n";
        return 2;
    }
    // TODO: your code here.
    return 0;
}
