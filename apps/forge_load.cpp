// forge-load: the load generator.   (component LOAD)
//
// Placeholder: exits with code 77, which tells the black-box tests to report
// SKIPPED instead of FAILED. LOAD-2 replaces this with the real program.
#include <cstdio>

int main() {
    std::fputs("forge_load: not implemented yet (see issues/LOAD-2.md)\n", stderr);
    return 77;
}
