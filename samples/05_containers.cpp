import std;

int sum_all(std::span<const int> xs) {
    int s = 0;
    for (int x : xs) s += x;
    return s;
}

int main() {
    // vector：动态数组
    std::vector<int> v{5, 3, 9};
    v.push_back(1);
    std::println("size={} sum={}", v.size(), sum_all(v));

    // string
    auto s = std::string{"hello"};
    s += ", world";
    std::println("{} (len={})", s, s.size());

    // map：按 key 有序
    std::map<std::string, int> hp{{"alice", 100}, {"bob", 80}};
    hp["carol"] = 60;
    std::println("bob={} size={}", hp.at("bob"), hp.size());

    // optional：可能有值
    std::optional<int> maybe = std::nullopt;
    std::println("maybe has? {}", maybe.has_value());
    maybe = 42;
    std::println("maybe={}", *maybe);

    // variant：多选一
    std::variant<int, std::string> box = 7;
    box = std::string{"hi"};
    std::println("is string? {}", std::holds_alternative<std::string>(box));
}