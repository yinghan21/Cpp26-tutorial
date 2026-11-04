import std;

int main() {
    const std::string path = "demo.txt";

    // 写文件（离开作用域自动关闭，不用 fclose）
    {
        std::ofstream out{path};
        out << "line one\n" << "line two\n";
    }

    // 读文件
    std::ifstream in{path};
    std::string line;
    int n = 0;
    while (std::getline(in, line)) std::println("{}: {}", ++n, line);

    // 格式化
    auto text = std::format("hp={} name={}", 100, "玩家");
    std::println("{}", text);
    std::println("{:>8}|{:<8}|{:^8}", "right", "left", "mid");
    std::println("{:.2f}", 3.14159);
}