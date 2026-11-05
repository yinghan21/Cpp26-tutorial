import std;

enum class Color : std::uint8_t { red, green, blue };
enum class Flag { on = 1, off = 0 };

const char* name_of(Color c) {
    switch (c) {
        case Color::red: return "red";
        case Color::green: return "green";
        case Color::blue: return "blue";
    }
    return "?";
}

// overloaded：多个 lambda 合成一个访问器
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

int main() {
    // ---- enum class ----
    Color c = Color::green;
    std::println("{} underlying={}", name_of(c), static_cast<int>(std::to_underlying(c)));

    // ---- variant ----
    using Value = std::variant<int, double, std::string>;
    std::vector<Value> vals{42, 3.14, std::string{"hi"}};
    for (const auto& v : vals) {
        std::visit(overloaded{
            [](int x) { std::println("int {}", x); },
            [](double x) { std::println("double {}", x); },
            [](const std::string& x) { std::println("string {}", x); }
        }, v);
    }
    std::println("size_of variant={} 最大成员={}", sizeof(Value), sizeof(std::string));
    std::println("holds_alternative<string>? {}", std::holds_alternative<std::string>(vals[2]));

    // ---- optional 单子操作 ----
    std::optional<int> o = 4;
    auto doubled = o.transform([](int x) { return x * 2; });
    auto as_text = doubled.transform([](int x) { return std::format("<{}>", x); });
    std::println("{}", as_text.value_or("none"));
    std::optional<int> none;
    std::println("空值 or_else -> {}", none.or_else([] { return std::optional<int>{-1}; }).value());

    // ---- any ----
    std::any box = 42;
    std::println("any is int? {} value={}", box.type() == typeid(int), std::any_cast<int>(box));
    try {
        std::println("{}", std::any_cast<std::string>(box));
    } catch (const std::bad_any_cast&) {
        std::println("any_cast 类型不对会抛 bad_any_cast");
    }

    // ---- tuple ----
    std::tuple<int, std::string, double> t{1, "one", 1.5};
    auto [id, label, score] = t;
    std::println("{} {} {}", id, label, score);
    std::println("get<1>={}", std::get<1>(t));
    std::apply([](auto&&... args) { ((std::print("{} ", args)), ...); }, t);
    std::println("");
}
