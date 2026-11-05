import std;

struct Item { std::string name; int price; };

int main() {
    std::vector<int> v{5, 2, 8, 1};

    // ---- 迭代器基本形态 ----
    auto first = v.begin();
    auto last = v.end();
    std::println("first={} last-first={}", *first, last - first);
    std::println("++前 {}", *first);
    ++first;
    std::println("++后 {}", *first);

    // ---- range-for 的展开 ----
    for (auto it = v.begin(); it != v.end(); ++it) std::print("{} ", *it);
    std::println("");

    // ---- ranges 概念 ----
    static_assert(std::ranges::range<decltype(v)>);
    static_assert(std::ranges::sized_range<decltype(v)>);
    static_assert(std::ranges::random_access_range<decltype(v)>);
    std::println("range 概念检查通过");

    // ---- 投影 projection ----
    std::vector<Item> items{{"pen", 3}, {"book", 12}, {"cup", 5}};
    std::ranges::sort(items, {}, &Item::price);          // 按 price 排序
    for (const auto& it : items) std::print("{}({}) ", it.name, it.price);
    std::println("");
    std::ranges::sort(items, std::greater<>{}, &Item::price);   // 降序
    for (const auto& it : items) std::print("{}({}) ", it.name, it.price);
    std::println("");

    // ---- view 惰性求值 ----
    auto doubled = v | std::views::transform([](int x) { return x * 2; });
    std::println("未遍历前不计算；遍历结果 {}", doubled);
    std::println("filter+take 管线: {}", v | std::views::filter([](int x) { return x % 2 == 0; })
                                                 | std::views::take(2));

    // ---- view 不拥有数据 ----
    std::vector<int> src{1, 2, 3};
    auto view = src | std::views::transform([](int x) { return x + 1; });
    src.push_back(4);
    std::println("源变化后 view 也跟着变: {}", view);
}
