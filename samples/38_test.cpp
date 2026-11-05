import std;

// ---- 极简测试框架 ----
struct TestStats { int passed = 0; int failed = 0; };

TestStats g_stats;

void check(bool cond, std::string_view expr, std::string_view file, int line) {
    if (cond) {
        ++g_stats.passed;
    } else {
        ++g_stats.failed;
        std::println("FAIL {} ({}:{})", expr, file, line);
    }
}

#define CHECK(expr) check((expr), #expr, __FILE__, __LINE__)

// ---- 被测代码 ----
constexpr int add(int a, int b) { return a + b; }
int div_safe(int a, int b) {
    if (b == 0) throw std::invalid_argument("div by zero");
    return a / b;
}

// ---- 表驱动测试 ----
struct Case { int a, b, expect; };

void test_add_table() {
    const std::array<Case, 4> cases{{{1, 2, 3}, {0, 0, 0}, {-1, 1, 0}, {100, -50, 50}}};
    for (const auto& c : cases) CHECK(add(c.a, c.b) == c.expect);
}

// ---- 调用栈 ----
void deep(int n, std::string_view tag) {
    if (n == 0) {
        auto trace = std::stacktrace::current();
        std::println("调用栈深度 {} 帧 (tag={})", trace.size(), tag);
        for (const auto& frame : trace | std::views::take(3)) {
            std::println("  {}", frame.description());
        }
        return;
    }
    deep(n - 1, tag);
}

int main() {
    // ---- 断言风格 ----
    static_assert(add(1, 2) == 3, "编译期就能验证");

    CHECK(add(1, 2) == 3);
    CHECK(add(0, 0) == 0);
    CHECK(add(1, 2) == 4);          // 故意失败，演示输出

    test_add_table();

    // ---- 异常测试 ----
    bool threw = false;
    try { div_safe(1, 0); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
    CHECK(div_safe(10, 2) == 5);

    std::println("通过 {} 项，失败 {} 项", g_stats.passed, g_stats.failed);

    // ---- 调用栈 ----
    deep(3, "demo");

    // ---- 源码位置 ----
    auto loc = std::source_location::current();
    std::println("当前代码位置: {}:{}", loc.file_name(), loc.line());
}
