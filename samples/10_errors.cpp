import std;

// expected：要么拿到值，要么拿到错误信息
std::expected<int, std::string> parse_int(std::string_view s) {
    int v{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{} || ptr != s.data() + s.size())
        return std::unexpected(std::format("不是合法整数: {}", s));
    return v;
}

// optional：找不到就返回空
std::optional<int> find_hp(const std::map<std::string, int>& m, std::string_view who) {
    if (auto it = m.find(std::string{who}); it != m.end()) return it->second;
    return std::nullopt;
}

int main() {
    if (auto r = parse_int("123")) std::println("ok={}", *r);
    else std::println("err={}", r.error());

    if (auto r = parse_int("x1"); !r) std::println("err={}", r.error());

    std::map<std::string, int> hp{{"alice", 100}};
    std::println("alice={}", find_hp(hp, "alice").value_or(-1));
    std::println("bob={}", find_hp(hp, "bob").value_or(-1));

    // 异常：库函数越界会抛，catch 住就能继续跑
    try {
        std::vector<int> v{1, 2, 3};
        std::println("v.at(5)={}", v.at(5));
    } catch (const std::out_of_range& e) {
        std::println("caught: {}", e.what());
    }
}