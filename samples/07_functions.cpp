import std;

// 默认参数 + 重载
int area(int w, int h = 1) { return w * h; }
double area(double w) { return w * w; }

// constexpr 函数
constexpr int square(int x) { return x * x; }
static_assert(square(5) == 25);

// 返回多值：结构体 / tuple / optional
struct DivResult { int quot; int rem; };
DivResult divmod(int a, int b) { return {a / b, a % b}; }

std::optional<int> parse_positive(std::string_view s) {
    int value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc{} || value <= 0) return std::nullopt;
    return value;
}

// 函数指针 / std::function / 泛型 lambda
int twice(int x) { return x * 2; }

int apply_all(const std::vector<int>& v, const std::function<int(int)>& f) {
    int s = 0;
    for (int x : v) s += f(x);
    return s;
}

int main() {
    std::println("area(3)={} area(3,4)={} area(2.5)={}", area(3), area(3, 4), area(2.5));

    auto [q, r] = divmod(17, 5);
    std::println("17 = 5*{} + {}", q, r);

    std::println("parse 42 -> {}", parse_positive("42").value_or(-1));
    std::println("parse -1 -> {}", parse_positive("-1").value_or(-1));

    int (*fp)(int) = &twice;
    std::println("fp(21)={}", fp(21));

    std::vector<int> v{1, 2, 3};
    std::println("apply twice = {}", apply_all(v, twice));
    std::println("apply lambda = {}", apply_all(v, [](int x) { return x + 1; }));

    int base = 10;
    auto cap = [base](int x) { return x + base; };   // 值捕获
    std::println("cap(5)={}", cap(5));
}
