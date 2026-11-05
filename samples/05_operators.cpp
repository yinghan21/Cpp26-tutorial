import std;

int main() {
    // ---- 算术与整数除法 ----
    std::println("7/2={} 7%2={} 7.0/2={}", 7 / 2, 7 % 2, 7.0 / 2);
    std::println("-7/2={} -7%2={}", -7 / 2, -7 % 2);   // 向零取整，余数跟随被除数

    // ---- 位运算 ----
    std::uint8_t flags = 0b0000'0101;
    std::println("flags={:08b} has_bit0={} has_bit1={}", flags, (flags & 0b0001) != 0, (flags & 0b0010) != 0);
    flags = static_cast<std::uint8_t>(flags | 0b0010);      // 置位
    flags = static_cast<std::uint8_t>(flags & ~0b0001);     // 清位
    flags = static_cast<std::uint8_t>(flags ^ 0b1000);      // 翻转
    std::println("after ops={:08b}", flags);
    std::println("1<<10={} 0xFF>>4={}", 1 << 10, 0xFF >> 4);

    // ---- 短路求值 ----
    int calls = 0;
    auto side = [&calls] { ++calls; return true; };
    bool r1 = false && side();      // side 不会调用
    bool r2 = true || side();       // side 不会调用
    std::println("short-circuit: r1={} r2={} calls={}", r1, r2, calls);

    // ---- ++i vs i++ ----
    int i = 5;
    std::println("i++={} now i={}", i++, i);
    std::println("++i={} now i={}", ++i, i);

    // ---- 三目与逗号 ----
    int a = 3, b = 8;
    std::println("max={}", a > b ? a : b);
    int c = (a++, b++);             // 逗号表达式：值为最后一个
    std::println("c={} a={} b={}", c, a, b);

    // ---- 整数提升与混合运算 ----
    std::uint8_t x = 200, y = 100;
    auto sum = x + y;                // int，不是 uint8_t
    std::println("x+y={} sizeof={}", sum, sizeof(sum));
    std::uint8_t bad = static_cast<std::uint8_t>(x + y);   // 300 -> 44
    std::println("回绕后={}", static_cast<int>(bad));

    // ---- 三路比较 ----
    std::println("3 <=> 5 = {}", (3 <=> 5) < 0 ? "less" : "not less");
    std::println("partial: {}", std::is_lt(3.0 <=> 5.0));

    // ---- 优先级陷阱 ----
    bool p = true, q = false;
    std::println("p && q || p = {}", p && q || p);   // && 高于 ||
    std::println("!p || q = {}", !p || q);           // ! 高于 ||
    int n = 10;
    std::println("n & 1 == 0 ? {}", (n & 1) == 0);   // 位与低于 ==
}
