import std;

int add(int a, int b) { return a + b; }

void damage(int& hp, int d) { hp -= d; }                     // 引用：能改调用方
void show(const std::string& name) { std::println("name={}", name); }  // 只读引用：不拷贝

int power(int base, int exp = 2) {                           // 默认参数
    int r = 1;
    for (int i = 0; i < exp; ++i) r *= base;
    return r;
}

std::tuple<int, int> divide(int a, int b) { return {a / b, a % b}; }   // 返回多个值

int main() {
    std::println("add={}", add(2, 3));

    int hp = 100;
    damage(hp, 30);
    std::println("hp={}", hp);

    show("玩家一号");

    std::println("power(5)={} power(5,3)={}", power(5), power(5, 3));

    auto [q, r] = divide(7, 2);
    std::println("7/2={} remainder={}", q, r);
}