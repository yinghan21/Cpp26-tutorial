import std;

// 函数模板
template <class T>
T twice(T v) { return v + v; }

// 类模板
template <class T>
struct Box {
    T value;
    explicit Box(T v) : value(std::move(v)) {}
    T get() const { return value; }
};

// concept 定义
template <class T>
concept Addable = requires(T a, T b) {
    { a + b } -> std::same_as<T>;
    { a += b } -> std::same_as<T&>;
};

// 用 concept 约束
template <Addable T>
T sum3(const std::vector<T>& v) {
    T acc{};
    for (const auto& x : v) acc += x;
    return acc;
}

// 约束下的重载
void describe(std::integral auto v) { std::println("integral {}", v); }
void describe(std::floating_point auto v) { std::println("floating {:.2f}", v); }

// 非类型模板参数
template <std::size_t N>
struct FixedBuffer {
    std::array<char, N> data{};
    static constexpr std::size_t capacity = N;
};

// 特化与偏特化
template <class T> struct TypeName { static constexpr const char* value = "unknown"; };
template <> struct TypeName<int> { static constexpr const char* value = "int"; };
template <class T> struct TypeName<std::vector<T>> { static constexpr const char* value = "vector<T>"; };

int main() {
    std::println("twice(2)={} twice(1.5)={} twice(string)={}", twice(2), twice(1.5), twice(std::string{"ab"}));

    Box<int> b{42};
    std::println("Box={}", b.get());

    std::vector<int> v{1, 2, 3};
    std::println("sum3 int={}", sum3(v));
    std::vector<std::string> vs{"a", "b"};
    std::println("sum3 string={}", sum3(vs));

    describe(1);
    describe(2.5);

    FixedBuffer<16> fb;
    std::println("capacity={} sizeof={}", FixedBuffer<16>::capacity, sizeof(fb));

    std::println("{} {} {}", TypeName<int>::value, TypeName<double>::value, TypeName<std::vector<int>>::value);

    static_assert(Addable<int>);
    static_assert(Addable<std::string>);
    static_assert(!Addable<std::vector<int>>);
}
