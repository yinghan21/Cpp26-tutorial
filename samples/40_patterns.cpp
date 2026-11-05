import std;

// ---------- 1) PIMPL ----------
class Engine {
public:
    Engine();
    ~Engine();
    void run();
private:
    struct Impl;
    std::unique_ptr<Impl> p_;
};

struct Engine::Impl {
    int counter = 0;
    void run() { std::println("Engine::Impl::run counter={}", ++counter); }
};

Engine::Engine() : p_(std::make_unique<Impl>()) {}
Engine::~Engine() = default;
void Engine::run() { p_->run(); }

// ---------- 2) CRTP：静态多态 ----------
template <class Derived>
struct ShapeBase {
    double area() const { return static_cast<const Derived*>(this)->area_impl(); }
};

struct Square : ShapeBase<Square> {
    double s;
    explicit Square(double side) : s(side) {}
    double area_impl() const { return s * s; }
};

struct Circle : ShapeBase<Circle> {
    double r;
    explicit Circle(double radius) : r(radius) {}
    double area_impl() const { return 3.141592653589793 * r * r; }
};

// ---------- 3) NVI：非虚接口 ----------
class Document {
public:
    void render() const {          // 非虚，负责通用流程
        std::print("[doc] ");
        render_body();
        std::println("");
    }
    virtual ~Document() = default;
private:
    virtual void render_body() const = 0;   // 私有虚函数
};

struct TextDoc : Document {
    void render_body() const override { std::print("plain text"); }
};

// ---------- 4) 类型擦除 ----------
class AnyDrawable {
public:
    template <class T>
    AnyDrawable(T obj) : p_(std::make_unique<Model<T>>(std::move(obj))) {}
    void draw() const { p_->draw(); }
private:
    struct Concept { virtual ~Concept() = default; virtual void draw() const = 0; };
    template <class T>
    struct Model : Concept {
        T obj;
        explicit Model(T o) : obj(std::move(o)) {}
        void draw() const override { obj.draw(); }
    };
    std::unique_ptr<Concept> p_;
};

struct Button { void draw() const { std::println("draw Button"); } };
struct Label  { void draw() const { std::println("draw Label"); } };

// ---------- 5) 策略模式用 lambda ----------
double apply_strategy(const std::vector<double>& xs, const std::function<double(double, double)>& op) {
    double acc = xs.empty() ? 0 : xs[0];
    for (std::size_t i = 1; i < xs.size(); ++i) acc = op(acc, xs[i]);
    return acc;
}

int main() {
    // PIMPL
    Engine e;
    e.run();
    e.run();

    // CRTP
    Square sq{3};
    Circle ci{1};
    std::println("Square area={} Circle area={:.3f}", sq.area(), ci.area());

    // NVI
    TextDoc doc;
    doc.render();

    // 类型擦除
    std::vector<AnyDrawable> drawables;
    drawables.push_back(Button{});
    drawables.push_back(Label{});
    for (const auto& d : drawables) d.draw();

    // 策略（lambda）
    std::vector<double> xs{1, 2, 3, 4};
    std::println("sum={} product={}", apply_strategy(xs, std::plus<>()), apply_strategy(xs, std::multiplies<>()));

    // variant 代替继承
    struct Rect { double w, h; };
    struct Tri  { double b, h; };
    using Shape = std::variant<Rect, Tri>;
    std::vector<Shape> shapes{Rect{2, 3}, Tri{4, 5}};
    for (const auto& s : shapes) {
        double area = std::visit([](const auto& x) {
            if constexpr (std::same_as<std::decay_t<decltype(x)>, Rect>) return x.w * x.h;
            else return x.b * x.h / 2;
        }, s);
        std::println("variant area={}", area);
    }
}
