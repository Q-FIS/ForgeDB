// queue_test PRODUCERS CONSUMERS ITEMS: see README.md.
#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "usage: queue_test PRODUCERS CONSUMERS ITEMS\n";
        return 2;
    }
    // TODO: your code here.
    return 0;
}
