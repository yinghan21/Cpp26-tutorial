#include "geo.h"

#include <cstdio>
#include <vector>

int main() {
    Circle c{2.0};
    std::puts(describe(c).c_str());

    std::vector<int> v{1, 2, 3};
    std::printf("sum_all=%d\n", sum_all(v));
}
