import std;

int counter() {
    static int calls = 0;      // 函数内静态：只初始化一次，跨调用保持
    return ++calls;
}

struct Tracer {
    std::string name;
    explicit Tracer(std::string n) : name(std::move(n)) { std::println("+ {}", name); }
    ~Tracer() { std::println("- {}", name); }
};

int main() {
    // ---- 局部静态 ----
    std::println("{} {} {}", counter(), counter(), counter());

    // ---- static 的三种含义 ----
    // 1) 局部静态：上面
    // 2) 文件内链接：namespace 作用域的 static（只能本 TU 访问），模块时代用匿名命名空间
    // 3) 类静态成员：见第 16 节

    // ---- 悬垂引用演示：指向已销毁的对象 ----
    const std::string& dangling = [] {
        std::string local = "temporary";
        return std::cref(local).get();   // 返回局部变量的引用（错误示范）
    }();
    std::println("dangling 读到的内容不可信（长度 {}）", dangling.size());   // UB 演示

    // ---- 作用域与遮蔽 ----
    int x = 1;
    {
        int x = 2;                 // 遮蔽外层 x
        std::println("inner x={}", x);
    }
    std::println("outer x={}", x);

    // ---- 构造/析构顺序（栈展开） ----
    {
        Tracer a{"a"};
        Tracer b{"b"};
        std::println("body");
    }
    std::println("after scope");

    // ---- RAII：异常也能保证析构 ----
    try {
        Tracer c{"c"};
        throw std::runtime_error("boom");
    } catch (const std::exception& e) {
        std::println("caught: {}", e.what());
    }
}
