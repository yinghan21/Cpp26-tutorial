import std;

int main() {
    for (int i = 1; i <= 3; ++i) std::println("i={}", i);

    std::vector<int> v{10, 20, 30};
    for (int x : v) std::print("{} ", x);
    std::println("");

    int hp = 0;
    while (hp < 3) ++hp;
    std::println("hp={}", hp);

    int code = 2;
    switch (code) {
        case 1: std::println("one"); break;
        case 2: std::println("two"); break;
        default: std::println("other");
    }

    if (hp > 2 and code == 2) std::println("both true");
}