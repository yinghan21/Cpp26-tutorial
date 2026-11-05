import std;

struct Point {
    int x, y;
};

// 自定义 formatter：让 Point 也能被 print/format 使用
template <>
struct std::formatter<Point> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const Point& p, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({},{})", p.x, p.y);
    }
};

int main() {
    // ---- 基础格式 ----
    std::println("默认 {} | 精度 {:.3f} | 宽度 {:>8} | 居中 {:^8} | 填充 {:*<8}", 3.14159, 3.14159, 42, 42, "ab");
    std::println("十六进制 {:x} | 二进制 {:b} | 八进制 {:o} | 带前缀 {:#x}", 255, 5, 8, 255);
    std::println("科学计数 {:e} | 固定 2 位 {:.2f}", 12345.678, 0.256);
    std::println("字符串截断 {:.3}", "abcdefgh");
    std::println("字符 {:c} | 大括号 {{}}" , 'A');

    // ---- 顺序/占位 ----
    std::println("{1} {0} {1}", "a", "b");         // 手动指定下标

    // ---- 容器直接打印 ----
    std::vector<int> v{1, 2, 3};
    std::map<std::string, int> m{{"a", 1}};
    std::println("vector={} map={}", v, m);

    // ---- 自定义类型 ----
    std::println("point={}", Point{3, 4});

    // ---- format / format_to ----
    auto s = std::format("{}-{}", 1, 2);
    std::println("format 得到字符串: {}", s);

    std::string out;
    std::format_to(std::back_inserter(out), "{:04d}", 42);
    std::println("format_to: {}", out);

    // ---- 输出到 stderr ----
    std::println(std::cerr, "这行去 stderr");

    // ---- 与 cout 混用：osyncstream 保证整行不被打断 ----
    {
        std::osyncstream os(std::cout);
        os << "osyncstream 输出一行\n";
    }

    // ---- 性能：format vs 手写拼接 ----
    auto t0 = std::chrono::steady_clock::now();
    std::string acc;
    for (int i = 0; i < 20000; ++i) acc += std::format("{}-{}-{}", i, i * 2, i * 3);
    auto t1 = std::chrono::steady_clock::now();
    std::println("20000 次 format 耗时 {:.2f} ms，总长 {}",
                 std::chrono::duration<double, std::milli>(t1 - t0).count(), acc.size());
}
