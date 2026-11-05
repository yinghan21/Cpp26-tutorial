import std;

int main() {
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7};

    // enumerate
    for (auto [i, x] : v | std::views::enumerate) std::print("{}:{} ", i, x);
    std::println("");

    // chunk
    for (auto c : v | std::views::chunk(3)) std::print("{} ", c);
    std::println("");

    // zip
    std::vector<std::string> names{"a", "b", "c"};
    for (auto [n, x] : std::views::zip(names, v)) std::print("{}{} ", n, x);
    std::println("");

    // adjacent
    for (auto t : v | std::views::adjacent<2>) {
        std::print("({},{}) ", std::get<0>(t), std::get<1>(t));
    }
    std::println("");

    // join_with
    std::vector<std::vector<int>> vv{{1, 2}, {3}, {4, 5}};
    for (int x : vv | std::views::join_with(0)) std::print("{} ", x);
    std::println("");

    // cartesian_product
    std::vector<char> cs{'x', 'y'};
    for (auto [a, b] : std::views::cartesian_product(cs, std::vector<int>{1, 2})) {
        std::print("{}{} ", a, b);
    }
    std::println("");

    // repeat
    for (int x : std::views::repeat(7, 3)) std::print("{} ", x);
    std::println("");

    // split
    std::string s = "a,b,,c";
    for (auto part : s | std::views::split(',')) std::print("[{}]", std::string(part.begin(), part.end()));
    std::println("");

    // ranges::to
    auto evens = v | std::views::filter([](int x) { return x % 2 == 0; })
                  | std::ranges::to<std::vector>();
    std::println("evens = {}", evens);

    // fold_left / fold_right
    std::println("fold_left = {}", std::ranges::fold_left(v, 0, std::plus<>()));
    std::println("fold_left string = {}", std::ranges::fold_left(v, std::string{}, [](std::string acc, int x) {
        return acc + std::to_string(x);
    }));
    std::println("fold_right = {}", std::ranges::fold_right(v, 0, std::plus<>()));

    // stride / chunk_by / take_while / drop_while / reverse
    for (int x : v | std::views::stride(2)) std::print("{} ", x);
    std::println("");
    for (auto c : v | std::views::chunk_by([](int a, int b) { return b == a + 1; })) std::print("{} ", c);
    std::println("");

    // partial_sort / nth_element
    std::vector<int> w{9, 1, 8, 2, 7, 3, 6, 4, 5};
    std::ranges::partial_sort(w, w.begin() + 3);
    std::println("partial_sort = {}", w);
    std::ranges::nth_element(w, w.begin() + 4);
    std::println("nth_element = {}", w);
    std::println("nth = {}", w[4]);
}
