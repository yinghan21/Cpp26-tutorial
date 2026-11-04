import std;

int main() {
    int hp = 100;
    double speed = 1.5;
    bool alive = true;
    auto name = std::string{"玩家一号"};
    constexpr int kMaxLevel = 99;

    std::println("hp={} speed={} alive={}", hp, speed, alive);
    std::println("name={} len={} max={}", name, name.size(), kMaxLevel);

    auto level = 3;
    auto int_div = level / 2;      // 整数除法
    auto float_div = level / 2.0;  // 浮点除法
    std::println("{} vs {}", int_div, float_div);

    // 类型范围：int 装不下就换更宽的类型（别学 C 里硬塞）
    std::println("int max={} long long max={}",
                 std::numeric_limits<int>::max(),
                 std::numeric_limits<long long>::max());
    long long big = 3000000000LL;
    std::println("big={}", big);
}