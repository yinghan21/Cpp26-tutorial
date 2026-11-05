---
title: 并发二：条件变量、future 与同步原语
tags: [cpp, cpp26, 标准库, 并发]
chapter: 4
order: 33
created: 2026-11-05
---

# 并发二：条件变量、future 与同步原语

> [!info] 本节目标
> - 掌握 `condition_variable` 的正确写法（谓词 + 防假唤醒）
> - 会写生产者-消费者队列
> - 会用 `std::async` / `future` / `promise` / `packaged_task`
> - 会用 `latch` / `barrier` / `counting_semaphore` 三件套
> - 知道 future 如何传递异常

> [!tip] 先看结论
> - `cv.wait(lk, pred)` **必须带谓词**，否则会假唤醒（spurious wakeup）。
> - `notify_one` 在**释放锁之后**调用更好（减少唤醒后的等待）。
> - `std::async` 默认可能延迟执行；要真正并行必须写 `std::launch::async`。
> - `latch` 一次性、`barrier` 可重复、`semaphore` 限流——三个别混用。

## 一、`condition_variable` 的正确写法

```cpp
class Queue {
public:
    void push(int v) {
        { std::lock_guard lk(m_); data_.push(v); }
        cv_.notify_one();                       // 锁外通知
    }
    std::optional<int> pop() {
        std::unique_lock lk(m_);
        cv_.wait(lk, [this] { return !data_.empty() || closed_; });   // 带谓词
        if (data_.empty()) return std::nullopt;
        int v = data_.front(); data_.pop();
        return v;
    }
    void close() {
        { std::lock_guard lk(m_); closed_ = true; }
        cv_.notify_all();
    }
private:
    std::mutex m_;
    std::condition_variable cv_;
    std::queue<int> data_;
    bool closed_ = false;
};
```

实测（1..5 乘 10 求和）：

```text
消费者累计 = 150
```

四个必须掌握的点：

| 点 | 原因 |
| --- | --- |
| 用 `unique_lock` 而不是 `lock_guard` | `wait` 需要临时解锁/重新加锁 |
| `wait` 必须带谓词 | 假唤醒会让循环在条件不满足时继续 |
| 修改共享状态要持锁 | 否则谓词读到中间状态 |
| `notify` 放在锁外 | 减少被唤醒线程立刻又阻塞 |

> [!warning] 经典的错误写法
> ```cpp
> if (data_.empty()) cv_.wait(lk);     // ❌ 假唤醒后直接往下走
> while (data_.empty()) cv_.wait(lk);  // ✅ 正确（等价于带谓词版本）
> ```

## 二、`async` / `future` / `promise`

```cpp
auto fut = std::async(std::launch::async, [] { return 6 * 7; });
std::println("async 结果 = {}", fut.get());     // 42

std::promise<std::string> prom;
auto pfut = prom.get_future();
std::thread producer([&prom] { prom.set_value("来自 promise"); });
std::println("{}", pfut.get());
```

| 工具 | 用途 |
| --- | --- |
| `std::async` | 起一个异步任务，返回 `future` |
| `std::future<T>` | 等待 + 取值（`get()` 只能调一次） |
| `std::promise<T>` | 手动设置值 |
| `std::packaged_task<T()>` | 把可调用对象包装成"能取 future 的任务" |

`std::launch` 策略：

| 策略 | 行为 |
| --- | --- |
| `launch::async` | **一定**新开线程 |
| `launch::deferred` | 直到 `get()`/`wait()` 才在当前线程执行 |
| `async \| deferred`（默认） | 实现自己决定 → 可能根本不并行 |

> [!warning] 不写 `launch::async` 可能等于没并行
> 默认策略允许延迟执行，`get()` 时才跑。要真并发必须显式 `std::launch::async`。

### `packaged_task`

```cpp
std::packaged_task<int(int, int)> task([](int a, int b) { return a + b; });
auto f = task.get_future();
std::thread t(std::move(task), 2, 3);
std::println("{}", f.get());    // 5
t.join();
```

`packaged_task` 是线程池的核心组件：任务和结果分开传递。

## 三、future 与异常

```cpp
auto ex = std::async([]() -> int { throw std::runtime_error("任务失败"); });
try { ex.get(); }
catch (const std::exception& e) { std::println("捕获到异步异常: {}", e.what()); }
```

实测：

```text
捕获到异步异常: 任务失败
```

异常被存进 shared state，`get()` 时重新抛出。**这就是为什么 `get()` 一定要在 try 里**。

## 四、`latch` / `barrier` / `semaphore`

```cpp
// latch：一次性倒计时（等待所有线程就绪）
std::latch ready(2);
ready.count_down();     // 每个线程完成时调用
ready.wait();           // 主线程等待

// barrier：可重复的阶段同步
std::barrier sync_point(2, []() noexcept { std::println("阶段完成"); });
sync_point.arrive_and_wait();

// semaphore：限流
std::counting_semaphore<2> sem(2);
sem.acquire();   // 拿许可
sem.release();   // 还许可
```

实测：

```text
线程 2 就绪
线程 1 就绪
两个线程都已就绪
阶段1 线程 1
占用许可的线程 1
阶段1 线程 2
阶段完成，进入下一阶段
阶段2 线程 2
阶段2 线程 1
阶段完成，进入下一阶段
占用许可的线程 2
```

| 原语 | 可重复 | 典型场景 |
| --- | --- | --- |
| `latch` | ❌ 一次 | 启动等待、一次性汇合 |
| `barrier` | ✅ | 多阶段并行算法 |
| `counting_semaphore` | ✅ | 连接池限流、生产者-消费者 |

`binary_semaphore` 是 `counting_semaphore<1>` 的别名。

## 五、线程池骨架

```cpp
class ThreadPool {
public:
    explicit ThreadPool(unsigned n) {
        for (unsigned i = 0; i < n; ++i)
            workers_.emplace_back([this](std::stop_token) { worker_loop(); });
    }
    ~ThreadPool() { { std::lock_guard lk(m_); closed_ = true; } cv_.notify_all(); }

    template <class F>
    auto submit(F&& f) {
        using R = std::invoke_result_t<F>;
        auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
        auto fut = task->get_future();
        { std::lock_guard lk(m_); tasks_.emplace([task] { (*task)(); }); }
        cv_.notify_one();
        return fut;
    }
private:
    void worker_loop() {
        while (true) {
            std::function<void()> job;
            {
                std::unique_lock lk(m_);
                cv_.wait(lk, [this] { return closed_ || !tasks_.empty(); });
                if (closed_ && tasks_.empty()) return;
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
```

用到的东西正好是本节 + 上一节的全部知识：`jthread`、`mutex`、`cv`、`packaged_task`、`future`。

## 六、并发调试建议

| 现象 | 可能原因 | 手段 |
| --- | --- | --- |
| 偶发崩溃 | 数据竞争 | 加锁/原子；用 TSan（Linux） |
| 死锁 | 加锁顺序 | `scoped_lock`、减少嵌套锁 |
| 卡住不动 | 忘记 `notify` 或谓词永远不成立 | 打印状态、加超时 `wait_for` |
| 结果不稳定 | 竞争条件 | 缩小临界区之外还要检查"逻辑竞争" |
| 性能没提升 | 锁粒度过大/线程过多 | 减小临界区、用线程池 |

## 完整示例（已实测）

```powershell
.\build.ps1 samples\33_async.cpp
```

```text
消费者累计 = 150
async 结果 = 42
来自 promise
捕获到异步异常: 任务失败
线程 2 就绪
线程 1 就绪
两个线程都已就绪
阶段1 线程 1
占用许可的线程 1
阶段1 线程 2
阶段完成，进入下一阶段
阶段2 线程 2
阶段2 线程 1
阶段完成，进入下一阶段
占用许可的线程 2
```

（顺序每次运行可能不同，这是并发的正常现象。）

## 常见错误与排查

| 现象 | 报错/后果 | 原因 | 改法 |
| --- | --- | --- | --- |
| `cv.wait` 直接往下走 | 无（逻辑错） | 假唤醒 | 带谓词 |
| 程序结束仍阻塞 | 卡死 | 忘记 `notify_all` | 关闭时通知所有 |
| `future.get()` 抛异常 | `std::runtime_error` | 任务内部抛了 | try/catch |
| `get()` 调两次 | `std::future_error` | `get` 只能一次 | 保存结果 |
| `async` 没有并行 | 无 | 用了默认策略 | 显式 `launch::async` |
| `future` 析构阻塞 | 卡住 | `async` 返回的 future 析构会等待 | 明确 `get()` 或保存 |
| `packaged_task` 拷贝 | `C2280` | 它不可拷贝 | `std::move` |

## 练习

1. 实现一个容量为 1 的"单槽"队列（生产者阻塞等待消费者取走）。
2. 用 `std::async` 并行计算 4 个数组的和，再合并结果。
3. 用 `packaged_task` 把一个 lambda 提交到 `std::thread`，取回结果。
4. 用 `latch` 让主线程等待 5 个线程都完成初始化再开始。
5. 用 `counting_semaphore` 限制同时最多 3 个任务访问某个资源。
6. 用 `barrier` 实现两阶段的并行计算（第一阶段各自算，同步后第二阶段汇总）。

> [!success]- 参考答案
> 1.
> ```cpp
> class Slot {
> public:
>     void put(int v) {
>         std::unique_lock lk(m_);
>         cv_.wait(lk, [this] { return !full_; });
>         value_ = v; full_ = true; cv_.notify_all();
>     }
>     int take() {
>         std::unique_lock lk(m_);
>         cv_.wait(lk, [this] { return full_; });
>         full_ = false; cv_.notify_all();
>         return value_;
>     }
> private:
>     std::mutex m_; std::condition_variable cv_; int value_{}; bool full_ = false;
> };
> ```
> 2.
> ```cpp
> std::vector<std::future<int>> fs;
> for (auto& part : parts) fs.push_back(std::async(std::launch::async, [&part] { return std::accumulate(part.begin(), part.end(), 0); }));
> int total = 0;
> for (auto& f : fs) total += f.get();
> ```
> 3.
> ```cpp
> std::packaged_task<int()> task([]{ return 99; });
> auto fut = task.get_future();
> std::thread t(std::move(task));
> std::println("{}", fut.get());
> t.join();
> ```
> 4.
> ```cpp
> std::latch done(5);
> for (int i = 0; i < 5; ++i) std::jthread([&done] { /* init */ done.count_down(); });
> done.wait();
> ```
> 5.
> ```cpp
> std::counting_semaphore<3> sem(3);
> auto job = [&sem] { sem.acquire(); /* 访问资源 */ sem.release(); };
> ```
> 6.
> ```cpp
> std::barrier bar(2);
> auto worker = [&bar](int id) {
>     int local = id * 10;
>     bar.arrive_and_wait();       // 阶段 1 同步
>     (void)local;                 // 阶段 2：使用所有人的结果
> };
> ```

---

上一节：[[32 并发一：线程、互斥与原子]] ｜ 下一节：[[34 协程与 generator]]
