import std;

struct Widget {
    static int created;                 // 静态成员：全类共享
    int id;
    std::string name;

    // 成员初始化列表：按声明顺序初始化
    explicit Widget(std::string n) : id(++created), name(std::move(n)) {
        std::println("Widget({}) 构造", name);
    }
    // 委托构造
    Widget() : Widget("anon") {}
    // 拷贝构造 / 拷贝赋值 / 析构 都显式定义，方便观察调用时机
    Widget(const Widget& o) : id(++created), name(o.name) { std::println("Widget({}) 拷贝", name); }
    Widget& operator=(const Widget& o) { name = o.name; std::println("Widget({}) 拷贝赋值", name); return *this; }
    ~Widget() { std::println("Widget({}) 析构", name); }

    int get_id() const { return id; }        // const 成员函数
};

int Widget::created = 0;

class Buffer {
public:
    explicit Buffer(std::size_t n) : data_(n, 0) {}   // explicit 阻止隐式转换
    std::size_t size() const { return data_.size(); }
private:
    std::vector<int> data_;
};

int main() {
    {
        Widget a{"a"};              // 构造
        Widget b = a;               // 拷贝构造
        Widget c;                   // 委托构造 → Widget("anon")
        b = a;                      // 拷贝赋值
        std::println("三个对象的 id: {} {} {} created={}", a.get_id(), b.get_id(), c.get_id(), Widget::created);
    }   // 逆序析构

    Buffer buf{4};                  // 显式构造 OK
    // Buffer bad = 4;              // 错误：explicit 阻止隐式转换
    std::println("buf.size={}", buf.size());

    // 类内/外都可以调用 const 成员函数；非 const 对象可调两者
    const Widget cw{"const"};
    std::println("cw.get_id={}", cw.get_id());
}
