import std;

struct Shape {
    std::string name;
    explicit Shape(std::string n) : name(std::move(n)) {}
    virtual ~Shape() = default;                 // 虚析构：必须
    virtual double area() const = 0;            // 纯虚函数 → 抽象类
    virtual std::string describe() const { return std::format("{} area={:.2f}", name, area()); }
};

struct Circle : Shape {
    double r;
    explicit Circle(double radius) : Shape("circle"), r(radius) {}
    double area() const override { return 3.141592653589793 * r * r; }
};

struct Rect : Shape {
    double w, h;
    Rect(double width, double height) : Shape("rect"), w(width), h(height) {}
    double area() const override { return w * h; }
};

int main() {
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Circle>(1.0));
    shapes.push_back(std::make_unique<Rect>(2.0, 3.0));

    for (const auto& s : shapes) std::println("{}", s->describe());

    // ---- 虚函数表：有虚函数的类多一个 vptr ----
    std::println("sizeof(Shape)={} sizeof(Circle)={} sizeof(double)={} sizeof(std::string)={}",
                 sizeof(Shape), sizeof(Circle), sizeof(double), sizeof(std::string));

    // ---- 对象切片 ----
    Circle c{2.0};
    Shape& ref = c;                 // 引用：多态保留
    std::println("通过引用 area={:.2f}", ref.area());
    // Shape sliced = c;            // 错误：抽象类不能实例化（若可实例化则会切片）

    // ---- dynamic_cast ----
    Shape* p = shapes[0].get();
    if (auto* pc = dynamic_cast<Circle*>(p)) std::println("是圆，半径 {}", pc->r);
    if (dynamic_cast<Rect*>(p) == nullptr) std::println("不是矩形");

    // ---- 用 variant 做静态多态（无 vtable） ----
    using Any = std::variant<Circle, Rect>;
    std::vector<Any> objs{Circle{1.0}, Rect{2.0, 3.0}};
    for (const auto& o : objs) {
        double a = std::visit([](const auto& x) { return x.area(); }, o);
        std::println("variant area={:.2f}", a);
    }
}
