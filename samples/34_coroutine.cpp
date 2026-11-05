import std;

// ---- 用 std::generator 写惰性序列 ----
std::generator<int> fibonacci(int count) {
    int a = 0, b = 1;
    for (int i = 0; i < count; ++i) {
        co_yield a;
        std::tie(a, b) = std::pair{b, a + b};
    }
}

std::generator<int> take_even(std::generator<int> src) {
    for (int v : src) if (v % 2 == 0) co_yield v;
}

// ---- 手写最小协程：Task ----
struct Task {
    struct promise_type {
        int value{};
        Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(int v) { value = v; }
        void unhandled_exception() { std::terminate(); }
    };
    std::coroutine_handle<promise_type> h;
    ~Task() { if (h) h.destroy(); }
    int result() { return h.promise().value; }
};

Task make_task() { co_return 7; }

int main() {
    // ---- generator 基础 ----
    std::print("fibonacci(10): ");
    for (int v : fibonacci(10)) std::print("{} ", v);
    std::println("");

    // ---- 管线：惰性求值 ----
    std::print("偶数斐波那契: ");
    for (int v : take_even(fibonacci(20))) std::print("{} ", v);
    std::println("");

    // ---- 只取前 3 个（惰性，不会算完 20 个） ----
    int taken = 0;
    for (int v : fibonacci(1000)) {
        std::print("{} ", v);
        if (++taken == 3) break;
    }
    std::println("(只取了 3 个)");

    // ---- 手写 Task ----
    auto t = make_task();
    std::println("Task 结果 = {}", t.result());

    // ---- 与容器/迭代器对比 ----
    std::vector<int> vec;
    for (int v : fibonacci(5)) vec.push_back(v);
    std::println("收集到容器: {}", vec);
}
