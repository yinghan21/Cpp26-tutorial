import std;

// ---- 参数包与 sizeof... ----
template <class... Ts>
std::size_t count_args(Ts... args) { return sizeof...(args); }

// ---- 四种折叠表达式 ----
template <class... Ts> auto sum_unary_right(Ts... xs) { return (xs + ...); }          // 一元右折叠
template <class... Ts> auto sum_unary_left(Ts... xs) { return (... + xs); }           // 一元左折叠
template <class... Ts> auto sum_binary(Ts... xs) { return (0 + ... + xs); }           // 二元左折叠（可空包）
template <class... Ts> auto all_true(Ts... xs) { return (true && ... && xs); }        // 二元右折叠

// ---- 完美转发 ----
template <class... Ts>
void forward_to(Ts&&... args) {
    std::println("转发 {} 个参数", sizeof...(args));
    std::vector<std::string> out;
    (out.push_back(std::format("{}", std::forward<Ts>(args))), ...);   // 折叠 + 逗号
    std::println("{}", out);
}

// ---- 变参构造函数：emplace 为什么快 ----
struct Person {
    std::string name;
    int age;
    Person(std::string n, int a) : name(std::move(n)), age(a) { std::println("构造 {}({})", name, age); }
    Person(const Person&) { std::println("拷贝"); }
    Person(Person&&) noexcept { std::println("移动"); }
};

int main() {
    std::println("count_args={}", count_args(1, "a", 3.0));

    std::println("一元右折叠 sum={}", sum_unary_right(1, 2, 3, 4));
    std::println("一元左折叠 sum={}", sum_unary_left(1, 2, 3, 4));
    std::println("二元折叠 sum={}", sum_binary());
    std::println("all_true={}", all_true(true, true, false));

    forward_to(1, "text", 2.5);

    std::vector<Person> people;
    people.reserve(4);
    people.push_back(Person{"push", 1});        // 构造 + 移动
    people.emplace_back("emplace", 2);          // 原地构造，无移动
}
