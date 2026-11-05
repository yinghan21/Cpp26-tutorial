import std;

struct AoS { double x, y, z; };          // 结构体数组
struct SoA { std::vector<double> x, y, z; };   // 数组结构体

volatile double sink;

int main() {
    constexpr int N = 200'000;

    // ---- 1) vector vs list 遍历 ----
    std::vector<int> v(N);
    std::iota(v.begin(), v.end(), 0);
    std::list<int> l(v.begin(), v.end());

    auto bench = [](std::string_view name, auto&& fn) {
        auto t0 = std::chrono::steady_clock::now();
        fn();
        auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        std::println("{:<22} {:.2f} ms", name, ms);
        return ms;
    };

    bench("vector 顺序遍历", [&] { std::int64_t s = 0; for (int x : v) s += x; sink = static_cast<double>(s); });
    bench("list 顺序遍历", [&] { std::int64_t s = 0; for (int x : l) s += x; sink = static_cast<double>(s); });

    // ---- 2) reserve 的效果 ----
    bench("push_back 无 reserve", [&] { std::vector<int> t; for (int i = 0; i < N; ++i) t.push_back(i); sink = static_cast<double>(t.size()); });
    bench("push_back 有 reserve", [&] { std::vector<int> t; t.reserve(N); for (int i = 0; i < N; ++i) t.push_back(i); sink = static_cast<double>(t.size()); });

    // ---- 3) AoS vs SoA ----
    std::vector<AoS> aos(N);
    for (int i = 0; i < N; ++i) aos[static_cast<std::size_t>(i)] = AoS{1.0 * i, 2.0 * i, 3.0 * i};

    SoA soa{std::vector<double>(N), std::vector<double>(N), std::vector<double>(N)};
    for (int i = 0; i < N; ++i) { soa.x[static_cast<std::size_t>(i)] = 1.0 * i; soa.y[static_cast<std::size_t>(i)] = 2.0 * i; soa.z[static_cast<std::size_t>(i)] = 3.0 * i; }

    bench("AoS 只累加 x", [&] { double s = 0; for (const auto& a : aos) s += a.x; sink = s; });
    bench("SoA 只累加 x", [&] { double s = 0; for (double d : soa.x) s += d; sink = s; });

    // ---- 4) 排序：sort vs partial_sort ----
    std::vector<int> r(N);
    std::mt19937 rng(42);
    for (auto& x : r) x = static_cast<int>(rng() % 1000000);
    auto r2 = r;
    bench("sort 全排序", [&] { std::ranges::sort(r); });
    bench("partial_sort 前 10", [&] { std::ranges::partial_sort(r2, r2.begin() + 10); });

    // ---- 5) 字符串拼接 ----
    bench("+= 逐次拼接", [&] { std::string s; for (int i = 0; i < 20000; ++i) s += "x"; sink = static_cast<double>(s.size()); });
    bench("+= 预留后拼接", [&] { std::string s; s.reserve(20000); for (int i = 0; i < 20000; ++i) s += "x"; sink = static_cast<double>(s.size()); });
}
