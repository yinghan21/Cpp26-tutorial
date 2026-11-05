import std;

struct Tracked {
    static inline int copies = 0;
    static inline int moves = 0;
    std::vector<int> data{1, 2, 3};      // 3 个 int 的堆数据

    Tracked() = default;
    Tracked(const Tracked& o) : data(o.data) { ++copies; }
    Tracked(Tracked&& o) noexcept : data(std::move(o.data)) { ++moves; }
};

struct NotNoexcept {
    static inline int copies = 0;
    static inline int moves = 0;
    std::vector<int> data{1, 2, 3};

    NotNoexcept() = default;
    NotNoexcept(const NotNoexcept& o) : data(o.data) { ++copies; }
    NotNoexcept(NotNoexcept&& o) : data(std::move(o.data)) { ++moves; }   // 没有 noexcept
};

int main() {
    // ---- value categories ----
    int a = 1;                  // a 是左值
    int&& rref = std::move(a);  // std::move 只是转换成右值引用
    std::println("a={} rref={} 同一对象={}", a, rref, &a == &rref);

    // ---- 移动构造 ----
    Tracked t;
    Tracked moved = std::move(t);
    Tracked copied = moved;
    std::println("moves={} copies={} 移动后 data.size={}", Tracked::moves, Tracked::copies, t.data.size());

    // ---- noexcept 决定 vector 扩容用移动还是拷贝 ----
    std::vector<Tracked> v1;
    v1.reserve(1);
    for (int i = 0; i < 10; ++i) v1.push_back(Tracked{});
    std::println("noexcept 移动: moves={} copies={}", Tracked::moves, Tracked::copies);

    std::vector<NotNoexcept> v2;
    v2.reserve(1);
    for (int i = 0; i < 10; ++i) v2.push_back(NotNoexcept{});
    std::println("非 noexcept 移动: moves={} copies={}", NotNoexcept::moves, NotNoexcept::copies);

    // ---- 零法则：资源都交给成员管理 ----
    struct ZeroRule {
        std::vector<int> data;
        std::string name;
        // 不写析构、拷贝、移动 —— 编译器生成正确的版本
    };
    ZeroRule z1{{1,2,3}, "x"};
    ZeroRule z2 = z1;                 // 深拷贝，安全
    z2.data.push_back(4);
    std::println("零法则: z1={} z2={}", z1.data, z2.data);
}
