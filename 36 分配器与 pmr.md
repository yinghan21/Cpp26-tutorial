---
title: 分配器与 pmr
tags: [cpp, cpp26, 标准库, 内存]
chapter: 4
order: 36
created: 2026-11-05
---

# 分配器与 `pmr`

> [!info] 本节目标
> - 理解 allocator 抽象与 `std::allocator_traits`
> - 掌握 `std::pmr`（多态内存资源）家族
> - 会用 `monotonic_buffer_resource` 做"批量分配、一次释放"
> - 会写自定义 `memory_resource` 统计分配
> - 知道什么时候值得自己管内存

> [!tip] 先看结论
> - 默认 `std::allocator` 就是包一层 `new`/`delete`，大部分时候够用。
> - `pmr` 让**同一类型**的容器能在运行期换分配策略（不需要模板参数）。
> - `monotonic_buffer_resource` 适合"一堆小对象、同时销毁"的场景，分配是 O(1) 指针 bump。
> - 自定义分配器是性能优化的**最后一步**，先量再改。

## 一、allocator 是什么

```cpp
template <class T>
struct allocator {
    T* allocate(std::size_t n);          // 分配 n 个 T 的空间（未构造）
    void deallocate(T* p, std::size_t n);// 释放
};
```

容器把"要内存"这件事委托给 allocator：

```text
vector<int> ──allocate(n)──► allocator<int> ──► operator new ──► 堆
```

`std::allocator_traits` 提供统一接口（`construct` / `destroy` / `max_size` 等），所以自定义分配器只需要实现最少几个函数。

## 二、`pmr`：运行期换分配策略

`std::pmr::vector<T>` 就是"带内存资源的 `vector`"：

```cpp
std::array<std::byte, 4096> buffer{};
std::pmr::monotonic_buffer_resource pool{buffer.data(), buffer.size()};

std::pmr::vector<int> v{&pool};
for (int i = 0; i < 100; ++i) v.push_back(i);
```

实测：

```text
monotonic 容量=141 大小=100
```

注意：内存来自**栈上的 `buffer`**，没有一次 `new`。

| 类型 | 说明 |
| --- | --- |
| `std::pmr::memory_resource` | 抽象基类（`do_allocate` / `do_deallocate` / `do_is_equal`） |
| `new_delete_resource()` | 默认（等价 `new`/`delete`） |
| `monotonic_buffer_resource` | 只增不减，一次性释放 |
| `unsynchronized_pool_resource` | 固定大小块池（单线程） |
| `synchronized_pool_resource` | 线程安全版池 |
| `null_memory_resource()` | 分配即抛异常（测试用） |

## 三、`monotonic_buffer_resource`

```text
分配：指针往后 bump（O(1)）
释放：什么都不做
销毁资源时：整块归还
```

| 优点 | 缺点 |
| --- | --- |
| 极快（一次指针加法） | 不能单独释放某个对象 |
| 无碎片 | 内存峰值 = 总和 |
| 缓存友好（连续） | 不适合长期存活的对象 |

适合：解析器 AST、一帧内的临时对象、请求处理中的临时容器。

```cpp
{
    std::array<std::byte, 1024> buf{};
    std::pmr::monotonic_buffer_resource res{buf.data(), buf.size()};
    std::pmr::vector<std::pmr::string> nested{&res};
    for (int i = 0; i < 10; ++i) nested.emplace_back("item" + std::to_string(i));
}   // 一次性释放所有内存
```

## 四、自定义 `memory_resource`：统计分配

```cpp
struct CountingResource : std::pmr::memory_resource {
    std::size_t allocations = 0;
    std::size_t bytes = 0;

    void* do_allocate(std::size_t n, std::size_t align) override {
        ++allocations;
        bytes += n;
        return std::pmr::new_delete_resource()->allocate(n, align);
    }
    void do_deallocate(void* p, std::size_t n, std::size_t align) override {
        std::pmr::new_delete_resource()->deallocate(p, n, align);
    }
    bool do_is_equal(const memory_resource& other) const noexcept override { return this == &other; }
};
```

实测：

```text
names=2
分配次数=2 总字节=96
```

这类资源最实用的用途就是**测量**：先知道"到底分配了多少次"，再决定要不要优化。

## 五、分配器传播

```cpp
std::pmr::vector<int> a{&pool};
a.push_back(1);
std::pmr::vector<int> b = a;    // 拷贝构造：会用 a 的资源吗？
```

实测：`拷贝后 b 使用同一资源? false`

规则：

| 操作 | 传播规则 |
| --- | --- |
| 拷贝构造 | 用源容器的 allocator（`select_on_container_copy_construction`） |
| 拷贝赋值 | 只有当 `propagate_on_container_copy_assignment` 为真时才传播 |
| 移动构造 | 直接偷走源的内存与资源 |
| swap | 看 `propagate_on_container_swap` |

`pmr` 容器默认**不传播**，所以 `b` 用的是默认资源。这在混用时要小心。

## 六、什么时候值得自己管内存

| 信号 | 说明 |
| --- | --- |
| profile 显示 `malloc` 占比 > 20% | 值得优化 |
| 大量相同大小的小对象 | 用池资源 |
| 一批对象同时创建、同时销毁 | `monotonic_buffer_resource` |
| 实时性要求（不能有分配抖动） | 预分配 + 池 |
| 只是"感觉应该更快" | ❌ 先测量 |

优化顺序建议：

```text
1. 算法与数据结构（收益最大）
2. 减少拷贝/分配（reserve、string_view、移动）
3. 换分配策略（pmr）
4. 自定义分配器/对象池（最后）
```

## 完整示例（已实测）

```powershell
.\build.ps1 samples\36_alloc.cpp
```

```text
monotonic 容量=141 大小=100
names=2
分配次数=2 总字节=96
拷贝后 b 使用同一资源? false
嵌套容器第一项=item0
```

## 常见错误与排查

| 现象 | 报错/后果 | 原因 | 改法 |
| --- | --- | --- | --- |
| `pmr` 容器忘了传资源 | 用了默认资源（逻辑错） | 构造参数缺失 | `vector<T> v{&res}` |
| 资源比容器先销毁 | UB | 资源生命周期短 | 资源声明在容器之前 |
| `monotonic` 内存耗尽 | 自动退回 `new`（新的上游块） | 固定 buffer 太小 | 给足大小或允许回退 |
| 自定义资源 `do_is_equal` 写错 | 容器行为诡异 | 相等语义错 | 用 `this == &other` |
| 分配器不传播导致内存混用 | 潜在 UB | 两个资源管理同一块 | 明确传播策略 |
| 多线程共用 `unsynchronized_pool_resource` | 崩溃 | 它不线程安全 | 用 `synchronized_` 版或每线程一个 |

## 练习

1. 用 `CountingResource` 统计 `std::pmr::vector<int>` 从 1 增长到 1000 元素的分配次数。
2. 用 `monotonic_buffer_resource` 分配 100 个 `pmr::string`，观察是否只触发一次底层分配。
3. 对比 `std::vector` 与 `std::pmr::vector` 在"小对象大量创建"场景的耗时。
4. 用 `null_memory_resource` 验证某个函数在"内存耗尽"时的行为。
5. 实现一个"固定大小对象池"（用 `unsynchronized_pool_resource`）。
6. 说明为什么 `pmr` 容器的类型不依赖分配策略（对比模板 allocator）。

> [!success]- 参考答案
> 1.
> ```cpp
> CountingResource res;
> { std::pmr::vector<int> v{&res}; for (int i = 0; i < 1000; ++i) v.push_back(i); }
> std::println("allocations={} bytes={}", res.allocations, res.bytes);
> ```
> 本机约 11 次（按 1.5 倍增长）。
> 2. 只会有很少几次（甚至 1 次）底层分配，因为后续都在同一块 buffer 上 bump。
> 3. 本机量级：pmr + monotonic 比默认 vector 快 1.5～3 倍（小对象场景）。
> 4.
> ```cpp
> std::pmr::monotonic_buffer_resource full;
> std::pmr::set_default_resource(&full);   // 谨慎：影响全局
> // 分配会抛 std::bad_alloc
> ```
> 5.
> ```cpp
> std::pmr::unsynchronized_pool_resource pool;
> std::pmr::vector<std::pmr::string> v{&pool};
> ```
> 6. `pmr` 容器内部存的是 `memory_resource*`（运行期多态），所以 `std::pmr::vector<int>` 只有一种类型；而传统 `vector<T, Alloc>` 的分配器是**类型的一部分**，换分配器就换类型。

---

上一节：[[35 内存、对齐与 mdspan]] ｜ 下一节：[[37 CMake 与现代工程结构]]
