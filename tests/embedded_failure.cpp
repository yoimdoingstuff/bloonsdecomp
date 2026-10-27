#include <cstdio>
#include <cstdlib>
namespace bloons {
[[noreturn]] void platformFailure(const char *message) {
    std::fprintf(stderr,"%s\n",message); std::abort();
}
}
