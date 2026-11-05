---
title: 协程与 generator
tags: [cpp, cpp26, 标准库, 协程]
chapter: 4
order: 34
created: 2026-11-05
---

# 协程与 `generator`

> [!info] 本节目标
> - 理解协程"可暂停的函数"这个本质
> - 会用 `std::generator<T>` 写惰性序列
> - 看懂 `promise_type` 的五个必备成员
> - 知道协程与线程、迭代器的区别
> - 会用它写流式处理

> [!tip] 先看结论
> - 协程是**函数能被暂停和恢复**的机制，运行在同一线程上，不涉及并发。
> - `co_yield` 交出值并挂起；`co_return` 结束；`co_await` 等待（本节不深入）。
> - `std::generator<T>` 是标准给出的"惰性序列"开箱即用实现。
> - 惰性意味着"你要一个才算一个"——可以表示无限序列。

## 一、协程是什么

```text
普通函数：调用 → 执行到底 → 返回
协程：    调用 → 执行到 co_yield → 挂起（保存现场）→ 返回一个值
                → 下次要值 → 恢复（从挂起处继续）
```

三个关键字：

| 关键字 | 作用 |
| --- | --- |
| `co_yield v` | 产出一个值并挂起 |
| `co_return v` | 结束协程（可带最终值） |
| `co_await expr` | 等待一个 awaitable（异步场景） |

一个函数里出现其中任何一个，它就是协程。

## 二、`std::generator<T>`：开箱即用

```cpp
std::generator<int> fibonacci(int count) {
    int a = 0, b = 1;
    for (int i = 0; i < count; ++i) {
        co_yield a;
        std::tie(a, b) = std::pair{b, a + b};
    }
}

for (int v : fibonacci(10)) std::print("{} ", v);
```

实测：

```text
fibonacci(10): 0 1 1 2 3 5 8 13 21 34
偶数斐波那契: 0 2 8 34 144 610 2584
0 1 1 (只取了 3 个)
Task 结果 = 7
收集到容器: [0, 1, 1, 2, 3]
```

三个观察：

1. `generator` 可以直接用在 range-for 里（它是 range）。
2. `fibonacci(1000)` 只取 3 个就 `break`——**后面的 997 个根本没算**，这就是惰性。
3. 可以 `for` 收集到容器里（`vector`）。

## 三、协程的组成：`promise_type`

`std::generator` 的背后是一套协议。手写最小版本：

```cpp
struct Task {
    struct promise_type {
        int value{};
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend() noexcept { return {}; }   // 立刻开始
        std::suspend_always final_suspend() noexcept { return {}; }    // 结束时挂起
        void return_value(int v) { value = v; }
        void unhandled_exception() { std::terminate(); }
    };
    std::coroutine_handle<promise_type> h;
    ~Task() { if (h) h.destroy(); }
    int result() { return h.promise().value; }
};

Task make_task() { co_return 7; }
```

实测：`Task 结果 = 7`

五个必备成员：

| 成员 | 作用 |
| --- | --- |
| `get_return_object()` | 创建返回给调用方的对象 |
| `initial_suspend()` | 协程启动时是否挂起（`suspend_never` = 立刻跑） |
| `final_suspend()` | 结束时是否挂起（通常 `always`，让调用方决定何时销毁） |
| `return_value(v)` / `return_void()` | 接收 `co_return` 的值 |
| `unhandled_exception()` | 处理协程体内抛出的异常 |

`std::suspend_never` / `std::suspend_always` 是两个现成的 awaitable：

| 类型 | `await_ready()` | 效果 |
| --- | --- | --- |
| `suspend_never` | `true` | 不挂起 |
| `suspend_always` | `false` | 总是挂起 |

## 四、协程 vs 线程 vs 迭代器

| 维度 | 协程 | 线程 | 迭代器 |
| --- | --- | --- | --- |
| 并发 | ❌ 单线程 | ✅ | ❌ |
| 能否保存执行位置 | ✅ | ❌ | 部分 |
| 开销 | 小（一次分配 + 状态机） | 大（MB 级栈） | 无 |
| 代码风格 | 顺序写法 | 顺序写法 | 需要写状态 |
| 适合 | 惰性序列、流式解析、异步 | 并行计算 | 容器遍历 |

协程最擅长的是**把"用回调写的复杂状态机"写成顺序代码**——比如逐行读文件、递归下降解析、异步 IO。

## 五、典型用途

| 场景 | 写法 |
| --- | --- |
| 惰性数列 | `std::generator<int> fib()` |
| 流式读文件 | `std::generator<std::string> lines(path)` |
| 树遍历 | `std::generator<Node&> walk(root)` |
| 生成器管线 | `take_even(fibonacci(20))` |
| 异步任务 | 需要自己写 `Task` + `co_await`（本节不深入） |

生成器管线示例：

```cpp
std::generator<int> take_even(std::generator<int> src) {
    for (int v : src) if (v % 2 == 0) co_yield v;
}

for (int v : take_even(fibonacci(20))) std::print("{} ", v);
```

## 六、性能与注意事项

| 事项 | 说明 |
| --- | --- |
| 堆分配 | 协程帧通常在堆上分配（编译器可能优化掉） |
| 内联 | 协程体很难被内联，比普通循环慢一些 |
| 生命周期 | `generator` 必须在**被遍历期间**存活 |
| 不能跨线程传递 | 协程帧里保存的是线程栈的延续状态 |

> [!warning] 别把 `generator` 存进容器后忘记遍历
> 协程体在被遍历时才执行；如果创建了却从不遍历，什么都不会发生（也不会有副作用）。

## 完整示例（已实测）

```powershell
.\build.ps1 samples\34_coroutine.cpp
```

```text
fibonacci(10): 0 1 1 2 3 5 8 13 21 34
偶数斐波那契: 0 2 8 34 144 610 2584
0 1 1 (只取了 3 个)
Task 结果 = 7
收集到容器: [0, 1, 1, 2, 3]
```

## 常见错误与排查

| 现象 | 报错 | 原因 | 改法 |
| --- | --- | --- | --- |
| `co_yield` 报错 | `C7610` / 语法错误 | 返回类型没有 `promise_type` | 用 `std::generator` 或自己实现 |
| 协程不执行 | 无（逻辑错） | 惰性，没被遍历 | 遍历它 |
| 程序崩溃在销毁时 | 崩溃 | `final_suspend` 返回 `suspend_never` 后 handle 被重复销毁 | 用 `suspend_always` + 手动 destroy |
| `generator` 悬垂 | 无（UB） | 源已销毁仍在遍历 | 保证生命周期 |
| 协程里 `co_await` 普通值 | 编译错 | 不是 awaitable | 用 `std::suspend_always` 等 |
| 性能不如预期 | 无 | 堆分配 + 无法内联 | 热路径用手写循环 |

## 练习

1. 用 `std::generator<int>` 生成 1..N 的平方。
2. 写 `take(n, gen)`，只取前 n 个元素。
3. 写一个 `lines(std::string_view text)` 生成器，按行切分文本。
4. 用生成器实现树的先序遍历（节点用 `struct Node { int v; std::vector<Node> kids; }`）。
5. 手写一个 `Task`，支持 `co_return` 一个 `std::string`。
6. 解释为什么 `fibonacci(1000)` 只取 3 个值时很快。

> [!success]- 参考答案
> 1.
> ```cpp
> std::generator<int> squares(int n) { for (int i = 1; i <= n; ++i) co_yield i * i; }
> ```
> 2.
> ```cpp
> std::generator<int> take(int n, std::generator<int> src) {
>     int k = 0;
>     for (int v : src) { if (k++ >= n) co_return; co_yield v; }
> }
> ```
> 3.
> ```cpp
> std::generator<std::string_view> lines(std::string_view text) {
>     std::size_t start = 0;
>     while (start < text.size()) {
>         auto pos = text.find('\n', start);
>         if (pos == std::string_view::npos) { co_yield text.substr(start); co_return; }
>         co_yield text.substr(start, pos - start);
>         start = pos + 1;
>     }
> }
> ```
> 4.
> ```cpp
> std::generator<const Node&> preorder(const Node& n) {
>     co_yield n;
>     for (const auto& k : n.kids) for (const auto& x : preorder(k)) co_yield x;
> }
> ```
> 5.
> ```cpp
> struct StringTask {
>     struct promise_type {
>         std::string value;
>         StringTask get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
>         std::suspend_never initial_suspend() noexcept { return {}; }
>         std::suspend_always final_suspend() noexcept { return {}; }
>         void return_value(std::string v) { value = std::move(v); }
>         void unhandled_exception() { std::terminate(); }
>     };
>     std::coroutine_handle<promise_type> h;
>     ~StringTask() { if (h) h.destroy(); }
> };
> ```
> 6. 因为协程是惰性的：每次 `co_yield` 只前进一格，`break` 之后剩下的循环体从未执行，`co_yield` 之后的代码也不会跑。

---

上一节：[[33 并发二：条件变量、future 与同步原语]] ｜ 下一节：[[35 内存、对齐与 mdspan]]
