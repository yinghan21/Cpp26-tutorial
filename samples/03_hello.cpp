import std;

int main() {
    std::println("你好，C++26！");
    std::println("1 + 2 = {}", 1 + 2);
    std::print("不换行");
    std::print(" 继续\n");
    std::println("{:.2f} | {:>6} | {:^6} | {:x} | {:08.3f}", 3.14159, 42, 42, 255, 3.5);
    std::println("结果是 {}", 1);
}
