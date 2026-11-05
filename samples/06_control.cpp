import std;

int main() {
    // if 带初始化语句
    if (auto it = std::vector<int>{1,2,3}.size(); it == 3) {
        std::println("if-init: size={}", it);
    }

    // switch + enum class + fallthrough
    enum class Cmd { start, stop, pause };
    for (auto c : {Cmd::start, Cmd::pause, Cmd::stop}) {
        switch (c) {
            case Cmd::start: std::println("start"); [[fallthrough]];
            case Cmd::pause: std::println("  (shared branch)"); break;
            case Cmd::stop:  std::println("stop"); break;
        }
    }

    // while / do-while / for
    int n = 3;
    while (n-- > 0) std::print("w{} ", n);
    int m = 0;
    do { ++m; } while (m < 3);
    std::println("do-m={}", m);

    // 范围 for 的真实展开
    std::vector<std::string> words{"a", "bb", "ccc"};
    for (const auto& w : words) std::print("[{}:{}]", w, w.size());
    std::println("");

    // break / continue
    for (int i = 0; i < 10; ++i) {
        if (i % 2 == 0) continue;      // 跳过偶数
        if (i > 7) break;              // 提前退出
        std::print("{} ", i);
    }
    std::println("");

    // 结构化绑定 + 提前返回
    std::map<std::string, int> hp{{"alice", 100}, {"bob", 80}};
    for (const auto& [name, value] : hp) std::println("{}={}", name, value);
}
