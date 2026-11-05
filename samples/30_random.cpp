import std;

int main() {
    // ---- 引擎与分布 ----
    std::mt19937_64 rng(std::random_device{}());     // 真随机种子
    std::uniform_int_distribution<int> dice(1, 6);

    std::array<int, 7> hist{};
    for (int i = 0; i < 6000; ++i) ++hist[static_cast<std::size_t>(dice(rng))];
    std::println("骰子 6000 次分布（1..6）: {}", std::span(hist).subspan(1));

    // ---- 可复现：固定种子 ----
    std::mt19937 fixed(42);
    std::uniform_int_distribution<int> d100(1, 100);
    std::print("固定种子序列: ");
    for (int i = 0; i < 5; ++i) std::print("{} ", d100(fixed));
    std::println("");

    // ---- 其他分布 ----
    std::normal_distribution<double> normal(0.0, 1.0);
    double acc = 0;
    for (int i = 0; i < 10000; ++i) acc += normal(rng);
    std::println("正态分布 10000 次均值 ≈ {:.3f}", acc / 10000);

    // ---- 洗牌 ----
    std::vector<int> cards{1, 2, 3, 4, 5};
    std::shuffle(cards.begin(), cards.end(), rng);
    std::println("洗牌后: {}", cards);

    // ---- 采样（不重复） ----
    std::vector<int> pool(10);
    std::iota(pool.begin(), pool.end(), 1);
    std::vector<int> picked(3);
    std::ranges::sample(pool, picked.begin(), 3, rng);
    std::println("从 1..10 抽 3 个: {}", picked);

    // ---- 模偏差对比 ----
    std::println("--- 模偏差 ---");
    std::mt19937 bad(7);
    std::array<int, 3> bad_hist{};
    for (int i = 0; i < 30000; ++i) ++bad_hist[bad() % 3];       // 有偏差
    std::println("rand()%3 分布: {}", bad_hist);

    std::mt19937 good(7);
    std::uniform_int_distribution<int> d3(0, 2);
    std::array<int, 3> good_hist{};
    for (int i = 0; i < 30000; ++i) ++good_hist[static_cast<std::size_t>(d3(good))];
    std::println("uniform_int 分布: {}", good_hist);

    // ---- 引擎信息 ----
    std::println("mt19937_64 最小值={} 最大值={}", std::mt19937_64::min(), std::mt19937_64::max());
}
