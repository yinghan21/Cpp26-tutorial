import std;

// 函数模板
template <class T>
T my_max(T a, T b) { return a < b ? b : a; }

// concept：限制 T 只能是数字
template <class T>
concept Number = std::is_arithmetic_v<T>;

template <Number T>
T twice(T v) { return v + v; }

// 类模板
template <class T>
class Box {
public:
    void put(T v) { v_ = std::move(v); }
    const T& get() const { return v_; }
private:
    T v_{};
};

int main() {
    std::println("max int={} max double={}", my_max(3, 7), my_max(1.5, 0.5));
    std::println("twice int={} twice double={}", twice(21), twice(1.25));

    Box<int> a;
    a.put(42);
    Box<std::string> b;
    b.put("hello");
    std::println("box a={} box b={}", a.get(), b.get());
}