import std;

int main() {
    std::vector<int> v{5, 3, 9, 1, 4, 8};

    // 迭代器分类探测
    std::println("vector::iterator = {}", std::is_same_v<
        std::iterator_traits<std::vector<int>::iterator>::iterator_category,
        std::random_access_iterator_tag>);
    std::println("list 双向 = {}", std::is_same_v<
        std::iterator_traits<std::list<int>::iterator>::iterator_category,
        std::bidirectional_iterator_tag>);
    std::println("forward_list 前向 = {}", std::is_same_v<
        std::iterator_traits<std::forward_list<int>::iterator>::iterator_category,
        std::forward_iterator_tag>);

    std::println("random_access_range = {}", std::ranges::random_access_range<std::vector<int>>);
    std::println("contiguous_range   = {}", std::ranges::contiguous_range<std::vector<int>>);
    std::println("sized_range        = {}", std::ranges::sized_range<std::vector<int>>);

    // 投影
    struct P { std::string name; int score; };
    std::vector<P> ps{{"a", 3}, {"b", 1}, {"c", 2}};
    std::ranges::sort(ps, {}, &P::score);
    for (auto& p : ps) std::print("{}({}) ", p.name, p.score);
    std::println("");

    // span + 哨兵
    auto n = std::ranges::distance(v);
    std::println("distance = {}", n);

    // 子范围
    auto sub = std::ranges::subrange(v.begin() + 1, v.begin() + 4);
    std::println("sub = {}", sub);

    // size 与 ssize
    std::println("size={} ssize={}", std::ranges::size(v), std::ranges::ssize(v));

    // view 不拥有
    auto sq = v | std::views::transform([](int x) { return x * x; });
    std::println("sq[0] = {}", sq[0]);
    std::println("view 大小 = {}", sizeof(sq));
    v[0] = 100;
    std::println("改容器后 sq[0] = {}", sq[0]);
}
