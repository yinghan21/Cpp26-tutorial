import std;

struct Vec2 {
    double x{}, y{};

    // 三路比较：一行得到 == != < <= > >=
    auto operator<=>(const Vec2&) const = default;

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2 operator*(double k) const { return {x * k, y * k}; }
    Vec2 operator-() const { return {-x, -y}; }
    double operator[](std::size_t i) const { return i == 0 ? x : y; }
    double& operator[](std::size_t i) { return i == 0 ? x : y; }
};

struct Counter {
    int n = 0;
    int operator()() { return ++n; }       // 函数对象
    explicit operator bool() const { return n > 0; }   // 显式转换
};

int main() {
    Vec2 a{1, 2}, b{3, 4};
    auto c = a + b;
    std::println("a+b = ({}, {})", c.x, c.y);
    c += Vec2{1, 1};
    std::println("+=  -> ({}, {})", c.x, c.y);
    auto d = c * 2.0;
    std::println("*2  -> ({}, {})", d.x, d.y);
    auto e = -a;
    std::println("-a  -> ({}, {})", e.x, e.y);

    // <=> 带来的全套比较
    std::println("a<b {} a==b {} a>=b {}", a < b, a == b, a >= b);
    std::println("按 y 排序用投影: {}", std::vector<Vec2>{{3,1},{1,9}} | std::views::transform([](const Vec2& v) { return v.y; }));

    // operator[] 读写
    Vec2 f{5, 6};
    f[0] = 50;
    std::println("f[0]={} f[1]={}", f[0], f[1]);

    // 函数对象
    Counter counter;
    std::println("counter: {} {} {}", counter(), counter(), counter());
    std::println("bool(counter)={}", static_cast<bool>(counter));

    // 排序：默认按 x 再按 y
    std::vector<Vec2> vs{{2,1},{1,5},{1,2}};
    std::ranges::sort(vs);
    for (const auto& v : vs) std::print("({},{}) ", v.x, v.y);
    std::println("");
}
