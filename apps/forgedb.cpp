// forgedb: the ForgeDB server.   (component APP)
//
// Placeholder: exits with code 77, which tells the black-box tests to report
// SKIPPED instead of FAILED. APP-4 replaces this with the real program.
#include <cstdio>

int main() {
    std::fputs("forgedb: not implemented yet (see issues/APP-4.md)\n", stderr);
    return 77;
}
