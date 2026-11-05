import std;

int main() {
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};

    // ---- 常用算法 ----
    std::println("count_if even = {}", std::ranges::count_if(v, [](int x) { return x % 2 == 0; }));
    std::println("contains 5 = {}", std::ranges::contains(v, 5));
    std::println("max = {}", std::ranges::max(v));

    // ---- 排序家族 ----
    std::vector<int> a{5, 1, 4, 2, 3};
    std::ranges::sort(a);
    std::println("sort          {}", a);
    std::ranges::nth_element(a, a.begin() + 2);
    std::println("nth_element   {}", a);
    std::ranges::partial_sort(a, a.begin() + 3);
    std::println("partial_sort  {}", a);

    // ---- 删除-擦除 ----
    std::vector<int> b{1, 2, 3, 4, 5, 6};
    std::erase_if(b, [](int x) { return x % 3 == 0; });
    std::println("erase_if      {}", b);

    // ---- C++23 views ----
    std::println("enumerate     {}", v | std::views::enumerate | std::views::take(3));
    std::println("chunk(3)      {}", v | std::views::chunk(3));
    std::println("zip           {}", std::views::zip(v, std::views::iota(10)) | std::views::take(3));
    std::println("adjacent<2>   {}", v | std::views::adjacent<2> | std::views::take(3));
    std::println("repeat        {}", std::views::repeat(7, 4));

    // ---- ranges::to 生成容器 ----
    auto squares = v
                 | std::views::filter([](int x) { return x % 2 == 1; })
                 | std::views::transform([](int x) { return x * x; })
                 | std::ranges::to<std::vector>();
    std::println("to<vector>    {}", squares);

    // ---- fold ----
    std::println("fold_left sum = {}", std::ranges::fold_left(v, 0, std::plus<>()));
    std::println("fold_right    = {}", std::ranges::fold_right(v, 0, std::plus<>()));

    // ---- 管线性能：视图不分配 ----
    std::vector<int> big(100000);
    std::ranges::iota(big, 0);
    std::int64_t total = 0;
    for (int x : big | std::views::filter([](int x) { return x % 2 == 0; })
                     | std::views::transform([](int x) { return x * 3; })) {
        total += x;
    }
    std::println("big total = {}", total);
}
