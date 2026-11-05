import std;

int unused_thing = 42;              // 故意：无人使用的全局变量

int add(int a, int b) {
    int result = a + b;             // 故意：result 计算了却没被使用
    return a + b;
}

int main() {
    std::vector<int> v{1, 2, 3};
    for (int i = 0; i < v.size(); ++i) {   // 故意：int 与 size_t 比较
        std::print("{} ", v[i]);
    }
    std::println("");
    std::println("add={}", add(1, 2));
}
