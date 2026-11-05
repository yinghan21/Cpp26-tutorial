import std;

// ---- 线程池：任务队列 + cv + jthread + packaged_task ----
class ThreadPool {
public:
    explicit ThreadPool(unsigned n = std::max(2u, std::thread::hardware_concurrency())) {
        for (unsigned i = 0; i < n; ++i) {
            workers_.emplace_back([this](std::stop_token st) { worker_loop(st); });
        }
    }
    ~ThreadPool() { shutdown(); }

    template <class F>
    auto submit(F&& f) {
        using R = std::invoke_result_t<F>;
        auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
        auto fut = task->get_future();
        {
            std::lock_guard lk(m_);
            if (closed_) throw std::runtime_error("线程池已关闭");
            tasks_.emplace([task] { (*task)(); });
        }
        cv_.notify_one();
        return fut;
    }

    void shutdown() {
        {
            std::lock_guard lk(m_);
            if (closed_) return;
            closed_ = true;
        }
        cv_.notify_all();
        workers_.clear();          // jthread 析构 = request_stop + join
    }

    std::size_t pending() {
        std::lock_guard lk(m_);
        return tasks_.size();
    }

    unsigned thread_count() const { return static_cast<unsigned>(workers_.size()); }

private:
    void worker_loop(std::stop_token st) {
        while (true) {
            std::function<void()> job;
            {
                std::unique_lock lk(m_);
                cv_.wait(lk, [&] { return closed_ || !tasks_.empty() || st.stop_requested(); });
                if (tasks_.empty()) {
                    if (closed_ || st.stop_requested()) return;
                    continue;
                }
                job = std::move(tasks_.front());
                tasks_.pop();
            }
            job();
        }
    }

    std::vector<std::jthread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex m_;
    std::condition_variable cv_;
    bool closed_ = false;
};

int main() {
    // ---- 1) 基本用法 ----
    ThreadPool pool(4);
    std::println("线程数 {}", pool.thread_count());

    auto f1 = pool.submit([] { std::this_thread::sleep_for(std::chrono::milliseconds(20)); return 1; });
    auto f2 = pool.submit([] { return 2; });
    auto f3 = pool.submit([] { return 3; });
    std::println("结果 {} {} {}", f1.get(), f2.get(), f3.get());

    // ---- 2) 异常传播 ----
    auto bad = pool.submit([]() -> int { throw std::runtime_error("任务炸了"); });
    try {
        bad.get();
    } catch (const std::exception& e) {
        std::println("捕获到任务异常: {}", e.what());
    }

    // ---- 3) 批量任务：并行 vs 串行 ----
    auto work = [](int i) {
        std::int64_t sum = 0;
        for (int k = 0; k < 200000; ++k) sum += (i * k) % 97;
        return sum;
    };

    constexpr int N = 64;
    auto t0 = std::chrono::steady_clock::now();
    std::int64_t serial = 0;
    for (int i = 0; i < N; ++i) serial += work(i);
    auto t1 = std::chrono::steady_clock::now();

    std::vector<std::future<std::int64_t>> futures;
    auto t2 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) futures.push_back(pool.submit([work, i] { return work(i); }));
    std::int64_t parallel = 0;
    for (auto& f : futures) parallel += f.get();
    auto t3 = std::chrono::steady_clock::now();

    std::println("串行={} 并行={} 结果一致={}", serial, parallel, serial == parallel);
    std::println("串行 {:.1f} ms | 并行 {:.1f} ms",
                 std::chrono::duration<double, std::milli>(t1 - t0).count(),
                 std::chrono::duration<double, std::milli>(t3 - t2).count());
    std::println("剩余任务 {}", pool.pending());

    pool.shutdown();
    std::println("优雅关闭完成");
}
