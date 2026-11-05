import std;

// ---- 异常：抛与接 ----
int parse_div(int a, int b) {
    if (b == 0) throw std::invalid_argument("除数不能为 0");
    return a / b;
}

// ---- expected：不抛异常的错误传递 ----
std::expected<int, std::string> safe_div(int a, int b) {
    if (b == 0) return std::unexpected("除零");
    return a / b;
}

// ---- error_code：系统错误码 ----
std::error_code try_read(const std::filesystem::path& p) {
    std::error_code ec;
    (void)std::filesystem::file_size(p, ec);
    return ec;
}

// ---- noexcept 与移动 ----
struct MoveOnly {
    std::vector<int> data;
    MoveOnly() = default;
    MoveOnly(MoveOnly&&) noexcept = default;
    MoveOnly& operator=(MoveOnly&&) noexcept = default;
    MoveOnly(const MoveOnly&) = delete;
};

int main() {
    // 1) 异常
    try {
        std::println("10/2={}", parse_div(10, 2));
        std::println("10/0={}", parse_div(10, 0));
    } catch (const std::invalid_argument& e) {
        std::println("捕获: {}", e.what());
    }

    // 2) expected 基本用法
    auto r1 = safe_div(10, 2);
    auto r2 = safe_div(10, 0);
    std::println("r1 has_value={} value={}", r1.has_value(), r1.value_or(-1));
    std::println("r2 has_value={} error={}", r2.has_value(), r2.error());

    // 3) expected 链式操作
    auto chained = safe_div(100, 5)
        .and_then([](int v) { return safe_div(v, 5); })
        .transform([](int v) { return v * 10; })
        .or_else([](const std::string& e) -> std::expected<int, std::string> {
            std::println("or_else 处理: {}", e);
            return std::unexpected(e);
        });
    std::println("chained={}", chained.value_or(-1));

    // 4) error_code
    auto ec = try_read("nonexistent_file_xyz.txt");
    std::println("error_code value={} message={}", ec.value(), ec.message());

    // 5) noexcept
    MoveOnly a;
    a.data = {1, 2, 3};
    std::vector<MoveOnly> v;
    v.push_back(std::move(a));
    std::println("noexcept 移动后 v[0].size={}", v[0].data.size());

    // 6) 断言
    int x = 5;
    if (x <= 0) std::unreachable();   // 契约式检查：违反即快速失败
    static_assert(sizeof(int) == 4, "int 必须是 4 字节");
    std::println("assert 通过");
}
