import std;

// ---- 生产者-消费者队列 ----
class Queue {
public:
    void push(int v) {
        {
            std::lock_guard lk(m_);
            data_.push(v);
        }
        cv_.notify_one();
    }
    std::optional<int> pop() {
        std::unique_lock lk(m_);
        cv_.wait(lk, [this] { return !data_.empty() || closed_; });
        if (data_.empty()) return std::nullopt;
        int v = data_.front();
        data_.pop();
        return v;
    }
    void close() {
        {
            std::lock_guard lk(m_);
            closed_ = true;
        }
        cv_.notify_all();
    }
private:
    std::mutex m_;
    std::condition_variable cv_;
    std::queue<int> data_;
    bool closed_ = false;
};

int main() {
    // ---- 1) 生产者消费者 ----
    Queue q;
    std::jthread consumer([&q](std::stop_token) {
        int sum = 0;
        while (auto v = q.pop()) sum += *v;
        std::println("消费者累计 = {}", sum);
    });
    for (int i = 1; i <= 5; ++i) q.push(i * 10);
    q.close();

    // ---- 2) async / future ----
    auto fut = std::async(std::launch::async, [] { return 6 * 7; });
    std::println("async 结果 = {}", fut.get());

    // ---- 3) promise ----
    std::promise<std::string> prom;
    auto pfut = prom.get_future();
    std::thread producer([&prom] { prom.set_value("来自 promise"); });
    std::println("{}", pfut.get());
    producer.join();

    // ---- 4) future 的异常传递 ----
    auto ex = std::async([]() -> int { throw std::runtime_error("任务失败"); });
    try {
        ex.get();
    } catch (const std::exception& e) {
        std::println("捕获到异步异常: {}", e.what());
    }

    // ---- 5) latch：等待所有线程就绪 ----
    std::latch ready(2);
    auto work = [&ready](int id) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5 * id));
        std::println("线程 {} 就绪", id);
        ready.count_down();
    };
    std::jthread t1([&] { work(1); });
    std::jthread t2([&] { work(2); });
    ready.wait();
    std::println("两个线程都已就绪");

    // ---- 6) barrier：分阶段同步 ----
    std::barrier sync_point(2, []() noexcept { std::println("阶段完成，进入下一阶段"); });
    auto phase_job = [&sync_point](int id) {
        std::println("阶段1 线程 {}", id);
        sync_point.arrive_and_wait();
        std::println("阶段2 线程 {}", id);
        sync_point.arrive_and_wait();
    };
    std::jthread b1([&] { phase_job(1); });
    std::jthread b2([&] { phase_job(2); });

    // ---- 7) counting_semaphore：限流 ----
    std::counting_semaphore<2> sem(2);
    std::mutex print_m;
    auto limited = [&](int id) {
        sem.acquire();
        {
            std::lock_guard lk(print_m);
            std::println("占用许可的线程 {}", id);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        sem.release();
    };
    std::jthread s1([&] { limited(1); });
    std::jthread s2([&] { limited(2); });
}
