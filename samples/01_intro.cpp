import std;

// 一个求平均的函数，三种时代三种写法
// 1) C 风格：指针 + 长度
double avg_c(const double* data, int n) {
    double s = 0;
    for (int i = 0; i < n; ++i) s += data[i];
    return n ? s / n : 0;
}

// 2) 现代：span 借用，容器/数组都能传
double avg(std::span<const double> xs) {
    if (xs.empty()) return 0;
    return std::ranges::fold_left(xs, 0.0, std::plus<>()) / xs.size();
}

int main() {
    std::vector<double> v{90, 80, 100, 70};

    std::println("C 风格: {:.2f}", avg_c(v.data(), static_cast<int>(v.size())));
    std::println("span  : {:.2f}", avg(v));

    // 现代管线：过滤 + 变换 + 求和，全是惰性视图，不产生中间容器
    double hi = std::ranges::fold_left(
        v | std::views::filter([](double x) { return x >= 80; })
          | std::views::transform([](double x) { return x * 1.1; }),
        0.0, std::plus<>());
    std::println("优秀者加成后总分: {:.1f}", hi);
}
