import std;

// ---- 编译期计算 ----
constexpr int factorial(int n) { return n <= 1 ? 1 : n * factorial(n - 1); }
consteval int square(int x) { return x * x; }          // 必须编译期
constinit int global_ready = 42;                       // 保证静态初始化

// 编译期容器（C++20 起 vector/string 可在 constexpr 中用）
constexpr std::size_t count_even(std::size_t n) {
    std::vector<int> v;
    for (std::size_t i = 0; i < n; ++i) v.push_back(static_cast<int>(i));
    return std::ranges::count_if(v, [](int x) { return x % 2 == 0; });
}

// if constexpr：按类型分支
template <class T>
std::string describe(T v) {
    if constexpr (std::integral<T>) return std::format("int {} ({} bit)", v, std::bit_width(static_cast<std::make_unsigned_t<T>>(v)));
    else if constexpr (std::floating_point<T>) return std::format("float {:.3f}", v);
    else return "other";
}

// 编译期查表
constexpr std::array<int, 8> fib_table = [] {
    std::array<int, 8> t{};
    t[0] = 0; t[1] = 1;
    for (std::size_t i = 2; i < t.size(); ++i) t[i] = t[i - 1] + t[i - 2];
    return t;
}();

int main() {
    static_assert(factorial(5) == 120);
    static_assert(square(7) == 49);
    static_assert(count_even(10) == 5);
    static_assert(fib_table[7] == 13);

    std::println("factorial(10)={}", factorial(10));
    std::println("square(12)={}", square(12));
    std::println("global_ready={}", global_ready);
    std::println("fib_table={}", fib_table);

    // 运行期 vs 编译期耗时对比
    auto t0 = std::chrono::steady_clock::now();
    std::int64_t sum = 0;
    for (int i = 0; i < 1000; ++i) sum += factorial(10);
    auto t1 = std::chrono::steady_clock::now();
    std::println("运行期 1000 次 factorial(10) = {} 耗时 {:.3f} ms", sum,
                 std::chrono::duration<double, std::milli>(t1 - t0).count());

    std::println("{} | {} | {}", describe(42), describe(2.5), describe(std::string{"x"}));
}
