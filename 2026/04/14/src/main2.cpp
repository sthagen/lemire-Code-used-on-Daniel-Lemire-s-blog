#include <fmt/core.h>

int main() {
    for(double i = 0; i <= 10; i += 0.31232) {
        fmt::print("{}\n", i);
    }
    return 0;
}