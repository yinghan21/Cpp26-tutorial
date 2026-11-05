import std;

int main() {
    // ---- thread 生命周期 ----
    {
        std::thread t([] { std::println("普通线程运行"); });
        t.join();                                  // 必须 join 或 detach
    }

    // ---- jthread + stop_token：自动 join，可请求停止 ----
    {
        std::jthread t([](std::stop_token st) {
            int i = 0;
            while (!st.stop_requested() && i < 3) {
                std::println("jthread 迭代 {}", i++);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        t.request_stop();                          // 请求停止
    }

    // ---- 数据竞争：不加锁会丢计数 ----
    int counter = 0;
    auto add = [&counter] { for (int i = 0; i < 100000; ++i) ++counter; };
    std::thread a(add), b(add);
    a.join(); b.join();
    std::println("无锁计数（通常小于 200000）= {}", counter);

    // ---- mutex 修复 ----
    int safe = 0;
    std::mutex m;
    auto add_safe = [&] { for (int i = 0; i < 100000; ++i) { std::lock_guard lk(m); ++safe; } };
    std::thread c(add_safe), d(add_safe);
    c.join(); d.join();
    std::println("加锁计数 = {}", safe);

    // ---- atomic ----
    std::atomic<int> at{0};
    auto add_atomic = [&at] { for (int i = 0; i < 100000; ++i) at.fetch_add(1, std::memory_order_relaxed); };
    std::thread e(add_atomic), f(add_atomic);
    e.join(); f.join();
    std::println("atomic 计数 = {}", at.load());

    // ---- compare_exchange ----
    std::atomic<int> cas{5};
    int expected = 5;
    bool swapped = cas.compare_exchange_strong(expected, 10);
    std::println("CAS 成功={} 现在值={}", swapped, cas.load());

    // ---- 硬件并发数 ----
    std::println("hardware_concurrency={}", std::thread::hardware_concurrency());

    // ---- 并行求和 vs 串行 ----
    std::vector<int> data(1'000'000, 1);
    auto t0 = std::chrono::steady_clock::now();
    std::int64_t seq_sum = std::accumulate(data.begin(), data.end(), std::int64_t{0});
    auto t1 = std::chrono::steady_clock::now();

    std::atomic<std::int64_t> par_sum{0};
    unsigned n = std::max(2u, std::thread::hardware_concurrency());
    std::vector<std::jthread> pool;
    std::size_t chunk = data.size() / n;
    auto t2 = std::chrono::steady_clock::now();
    for (unsigned k = 0; k < n; ++k) {
        pool.emplace_back([&, k](std::stop_token) {
            auto begin = data.begin() + static_cast<std::ptrdiff_t>(k * chunk);
            auto end = (k + 1 == n) ? data.end() : begin + static_cast<std::ptrdiff_t>(chunk);
            std::int64_t local = std::accumulate(begin, end, std::int64_t{0});
            par_sum.fetch_add(local, std::memory_order_relaxed);
        });
    }
    pool.clear();                                   // jthread 析构 = join
    auto t3 = std::chrono::steady_clock::now();

    std::println("串行={} 并行={} (线程数 {})", seq_sum, par_sum.load(), n);
    std::println("串行 {:.2f} ms | 并行 {:.2f} ms",
                 std::chrono::duration<double, std::milli>(t1 - t0).count(),
                 std::chrono::duration<double, std::milli>(t3 - t2).count());
}
