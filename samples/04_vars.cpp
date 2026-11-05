import std;

int main() {
    // ---- 类型宽度 ----
    std::println("char={} short={} int={} long={} long long={}",
                 sizeof(char), sizeof(short), sizeof(int), sizeof(long), sizeof(long long));
    std::println("float={} double={} long double={}",
                 sizeof(float), sizeof(double), sizeof(long double));
    std::println("int64_t={} uintptr_t={} size_t={}",
                 sizeof(std::int64_t), sizeof(std::uintptr_t), sizeof(std::size_t));

    // ---- auto 推导 ----
    auto a = 42;            // int
    auto b = 42u;           // unsigned int
    auto c = 42L;           // long
    auto d = 42.0;          // double
    auto e = 42.0f;         // float
    auto f = "hello";       // const char*
    auto g = std::string{"hello"};   // std::string（注意不是 const char*）
    std::println("{} {} {} {} {} {} {}", sizeof(a), sizeof(b), sizeof(c), sizeof(d), sizeof(e), sizeof(f), g.size());

    // ---- 四种初始化 ----
    int i1 = 10;
    int i2(20);
    int i3{30};
    int i4{};               // 值初始化为 0
    std::println("{} {} {} {}", i1, i2, i3, i4);

    // ---- 窄化：花括号初始化会挡住窄化 ----
    // int bad{3.14};        // error C2397: 从 double 到 int 需要收缩转换
    int ok = static_cast<int>(3.14);   // 显式转换：告诉编译器"我知道会截断"

    // ---- const / constexpr / consteval / constinit ----
    const int run_time = 7;                       // 运行期常量
    constexpr int compile_time = run_time + 3;    // 编译期常量
    static_assert(compile_time == 10, "编译期可验证");
    std::println("{} {}", run_time, compile_time);

    // ---- 数字分隔符 ----
    int million = 1'000'000;
    int hex = 0xFF'FF;
    int bin = 0b1010'1010;
    std::println("{} {} {}", million, hex, bin);

    // ---- 溢出与回绕 ----
    std::int8_t s8 = 127;
    // s8 + 1 是有符号溢出（UB），演示无符号回绕：
    std::uint8_t u8 = 255;
    u8 = static_cast<std::uint8_t>(u8 + 1);
    std::println("u8 回绕 = {}", static_cast<int>(u8));

    // ---- 浮点相等：永远不要用 ==  ----
    double x = 0.1 + 0.2;
    std::println("0.1+0.2 == 0.3 ? {}", x == 0.3);
    std::println("差值 = {:.20f}", x - 0.3);

    // ---- decltype ----
    decltype(a) copy_of_a = 1;
    decltype(auto) ded = (a);     // 括号 -> 引用
    std::println("{} {}", copy_of_a, ded);
    (void)ok; (void)hex; (void)bin; (void)s8;
}
