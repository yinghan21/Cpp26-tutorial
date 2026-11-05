import std;

int sum_raw(const int* data, int n) {
    int s = 0;
    for (int i = 0; i < n; ++i) s += data[i];
    return s;
}

int sum_span(std::span<const int> xs) {
    int s = 0;
    for (int x : xs) s += x;
    return s;
}

int main() {
    // ---- 原生数组与退化 ----
    int raw[4]{1, 2, 3, 4};
    std::println("sizeof(raw)={} 元素数={}", sizeof(raw), sizeof(raw) / sizeof(raw[0]));
    int* p = raw;                                  // 退化成指针，长度信息丢失
    std::println("sizeof(p)={}", sizeof(p));
    std::println("sum_raw={} sum_span={}", sum_raw(raw, 4), sum_span(raw));

    // ---- std::array ----
    std::array<int, 4> arr{4, 3, 2, 1};
    std::ranges::sort(arr);
    std::println("array size={} front={} back={}", arr.size(), arr.front(), arr.back());
    std::println("{}", arr);                       // 直接打印容器

    // ---- span 借用 vector，不拷贝 ----
    std::vector<int> v{10, 20, 30};
    std::span<int> sv{v};
    sv[0] = 99;                                    // 改 span 就是改 vector
    std::println("v[0]={} span size={}", v[0], sv.size());

    // ---- 越界：at() 抛异常，[] 不检查 ----
    try {
        std::println("{}", v.at(10));
    } catch (const std::out_of_range& e) {
        std::println("at 抛异常: {}", e.what());
    }

    // ---- 二维数组 ----
    int grid[2][3]{{1, 2, 3}, {4, 5, 6}};
    std::println("grid[1][2]={} 行长={}", grid[1][2], sizeof(grid[0]) / sizeof(int));

    // ---- span 也能接数组 ----
    std::println("span of array sum={}", sum_span(sv));
}
