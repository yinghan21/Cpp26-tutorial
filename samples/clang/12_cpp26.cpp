// C++26 语言特性（clang 22 实测通过；MSVC 19.51 还不支持）
// 故意不 include 任何头文件：用 static_assert 在编译期验证，main 只返回 0。

// 1) 包索引 t...[N]
template <class... T>
constexpr auto first(T... t) { return t...[0]; }

// 2) = delete("原因")
struct Connection {
    void send() = delete("Connection::send 已废弃，用 send_async()");
    void send_async() {}
};

// 3) 占位符 _：忽略返回值，可以重复定义（但不能再引用 _）
constexpr int probe() {
    auto _ = 42;
    auto _ = 7;
    return first(7, 8, 9);
}

static_assert(probe() == 7, "pack indexing failed");

int main() {
    Connection c;
    c.send_async();
    return 0;
}