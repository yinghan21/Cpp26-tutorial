import std;

struct Resource {
    std::string name;
    explicit Resource(std::string n) : name(std::move(n)) { std::println("acquire {}", name); }
    ~Resource() { std::println("release {}", name); }
};

struct Node {
    std::string name;
    std::shared_ptr<Node> next;          // 强引用
    std::weak_ptr<Node> prev;            // 弱引用，打破循环
    explicit Node(std::string n) : name(std::move(n)) {}
    ~Node() { std::println("~Node {}", name); }
};

int main() {
    // ---- unique_ptr：独占所有权 ----
    {
        auto p = std::make_unique<Resource>("unique");
        std::println("unique ptr name={}", p->name);
        auto q = std::move(p);                    // 转移所有权
        std::println("p 为空? {}", p == nullptr);
    }   // 析构在此处

    // ---- 自定义删除器 ----
    {
        auto deleter = [](Resource* r) { std::println("custom deleter for {}", r->name); delete r; };
        std::unique_ptr<Resource, decltype(deleter)> up{new Resource("custom"), deleter};
    }

    // ---- shared_ptr：共享所有权 ----
    auto a = std::make_shared<Resource>("shared");
    std::println("use_count={}", a.use_count());
    {
        auto b = a;
        std::println("拷贝后 use_count={}", a.use_count());
    }
    std::println("离开作用域后 use_count={}", a.use_count());

    // ---- weak_ptr 打破循环引用 ----
    {
        auto n1 = std::make_shared<Node>("n1");
        auto n2 = std::make_shared<Node>("n2");
        n1->next = n2;
        n2->prev = n1;                    // weak，不增加引用计数
        std::println("n1 use={} n2 use={}", n1.use_count(), n2.use_count());
        if (auto locked = n2->prev.lock()) std::println("通过 weak 访问 {}", locked->name);
    }

    // ---- enable_shared_from_this ----
    struct Self : std::enable_shared_from_this<Self> {
        std::string tag = "self";
        void greet() { std::println("shared_from_this use={}", shared_from_this().use_count()); }
    };
    auto s = std::make_shared<Self>();
    s->greet();

    // ---- make_shared vs new ----
    std::println("shared_ptr<int> 大小={} unique_ptr<int> 大小={}",
                 sizeof(std::shared_ptr<int>), sizeof(std::unique_ptr<int>));
}
