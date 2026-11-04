import std;

int main() {
    std::vector<int> v{5, 3, 9, 1, 4, 8};

    std::ranges::sort(v);
    std::print("sorted:");
    for (int x : v) std::print(" {}", x);
    std::println("");

    auto even = v | std::views::filter([](int x) { return x % 2 == 0; });
    std::print("even:");
    for (int x : even) std::print(" {}", x);
    std::println("");

    auto squared = v | std::views::transform([](int x) { return x * x; });
    std::print("squared:");
    for (int x : squared) std::print(" {}", x);
    std::println("");

    int total = std::ranges::fold_left(v, 0, std::plus{});
    std::println("total={}", total);

    auto [lo, hi] = std::ranges::minmax(v);
    std::println("min={} max={}", lo, hi);
}