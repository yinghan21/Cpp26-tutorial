---
title: 内存、对齐与 mdspan
tags: [cpp, cpp26, 标准库, 内存]
chapter: 4
order: 35
created: 2026-11-05
---

# 内存、对齐与 `mdspan`

> [!info] 本节目标
> - 看懂对象布局与 padding，知道怎么省内存
> - 掌握 `alignas` / `alignof`
> - 会用 placement new 在指定内存上构造对象
> - 会用 `std::mdspan` 表达多维视图
> - 理解 cache line 与伪共享

> [!tip] 先看结论
> - 结构体成员顺序会影响 `sizeof`（padding 可能占掉一半内存）。
> - `alignas(64)` 用来把对象对齐到 cache line，避免伪共享。
> - `mdspan` 是"多维的 span"：不拥有数据、零拷贝、能算索引。
> - `std::byte` 是"原始内存"的正确类型，别用 `char`。

## 一、对象布局与 padding

```cpp
struct Point { char c; int i; double d; };
struct Packed { double d; int i; char c; };
```

实测：

```text
sizeof(Point)=16 sizeof(Packed)=16
Point 偏移: c=0 i=4 d=8
Packed 偏移: d=0 i=8 c=12
```

`Point` 里 `c` 后面有 3 字节 padding（为了让 `i` 4 字节对齐）。所以：

| 规则 | 说明 |
| --- | --- |
| 每个成员按自己的对齐要求放置 | `int` 要 4 字节对齐 |
| 结构体总大小是最大对齐的倍数 | 末尾补齐 |
| 把大成员放前面通常更省 | `double, int, char` = 16 字节（这里刚好相同） |

实际省内存的例子（bool 被打包）：

```cpp
struct Flags { bool a, b, c, d; };        // sizeof == 4（不是 4 字节的 bool，是 4 个 1 字节）
struct Bits { std::uint8_t mask; };       // 1 字节，用位表示 4 个标志
```

## 二、对齐：`alignas` / `alignof`

```cpp
struct alignas(64) CacheLine { int v; };
```

实测：

```text
alignof(int)=4 alignof(double)=8 alignof(CacheLine)=64
sizeof(CacheLine)=64
```

| 用途 | 写法 |
| --- | --- |
| 对齐到 cache line（防伪共享） | `alignas(64)` |
| SIMD 对齐（16/32 字节） | `alignas(16)` |
| 查询对齐 | `alignof(T)` |
| 动态分配对齐内存 | `operator new(size, std::align_val_t{64})` |

## 三、placement new 与显式析构

```cpp
alignas(std::string) std::byte buffer[sizeof(std::string)];
auto* p = new (buffer) std::string("in place");
std::println("{}", *p);
p->~basic_string();          // 必须显式析构
```

实测：`placement new 得到: in place`

| 步骤 | 说明 |
| --- | --- |
| 准备内存 | `alignas(T) std::byte buf[sizeof(T)]` |
| 构造 | `new (buf) T(args...)` |
| 使用 | 通过返回的指针 |
| 析构 | `p->~T()`（**必须**） |

> [!warning] 析构必须手动调用
> placement new 不分配内存，所以 deallocate 不存在；但对象的析构函数**不会自动调用**，忘了就泄漏资源（比如 `std::string` 的堆内存）。

`std::launder`：在某些"在同一内存上重建对象"的场景下，告诉编译器"请重新读取这个指针指向的对象"（多见于 union/内存池）。

## 四、`std::mdspan`：多维视图

```cpp
std::vector<int> data(6);
std::iota(data.begin(), data.end(), 1);
std::mdspan m(data.data(), 2, 3);

m.extent(0);   // 2
m[r, c];       // 二维下标
m.size();      // 6（元素总数）
```

实测：

```text
mdspan 维度: 2 x 3
1 2 3
4 5 6
mdspan 元素总数=6
```

| 特性 | 说明 |
| --- | --- |
| 不拥有数据 | 只是"指针 + extents" |
| 零开销 | 编译期确定维度数 |
| 可切片 | `submdspan`（C++26 部分实现） |
| 布局策略 | `layout_right`（行优先，默认）、`layout_left`、`layout_stride` |

用途：矩阵运算、图像处理、神经网络张量——凡是"多维数组但底层是一块连续内存"的场景。

## 五、cache line 与伪共享

```text
CPU 缓存以 64 字节为单位（cache line）
两个线程分别写同一 cache line 的不同变量 → 缓存行来回失效 → 性能暴跌
```

实测（两个线程各做 200 万次原子自增）：

```text
伪共享 vs 对齐填充 耗时: 174.6 ms
```

这段示例把"共享 cache line"和"填充到 64 字节"两种情况放在同一循环里对比，你可以改写它分别计时，通常能观察到 2～5 倍差距。

解决伪共享：

```cpp
struct alignas(64) PaddedCounter {
    std::atomic<int> v{0};
    char pad[64 - sizeof(std::atomic<int>)];
};
```

## 六、`std::byte`

```cpp
std::byte b{0x2A};
std::to_integer<int>(b);      // 42
```

| 类型 | 用途 |
| --- | --- |
| `char` | 字符（有符号性不确定） |
| `unsigned char` | 字节（但语义是数值） |
| `std::byte` | **原始内存**（不能做算术，只能位运算） |

用 `std::byte` 的好处：编译器不会把它当字符，也不会意外做数值运算——语义清晰。

## 完整示例（已实测）

```powershell
.\build.ps1 samples\35_memory.cpp
```

```text
sizeof(Point)=16 sizeof(Packed)=16
Point 偏移: c=0 i=4 d=8
Packed 偏移: d=0 i=8 c=12
alignof(int)=4 alignof(double)=8 alignof(CacheLine)=64
sizeof(CacheLine)=64
std::byte -> 42
placement new 得到: in place
mdspan 维度: 2 x 3
1 2 3
4 5 6
mdspan 元素总数=6
伪共享 vs 对齐填充 耗时: 174.6 ms
```

## 常见错误与排查

| 现象 | 报错/后果 | 原因 | 改法 |
| --- | --- | --- | --- |
| placement new 后泄漏 | 无（运行期） | 忘记显式析构 | `p->~T()` |
| 对齐不满足崩溃 | `0xC0000005` | 用了未对齐内存给 SIMD 类型 | `alignas` |
| `sizeof` 比预期大 | 无 | padding | 调整成员顺序 |
| mdspan 越界 | UB | `[]` 不检查 | 自己校验 extents |
| `offsetof` 报错 | `C3861` | 没包含 `<cstddef>`（模块下不导出） | 用指针相减算偏移 |
| 多线程计数慢 | 无 | 伪共享 | `alignas(64)` |

## 练习

1. 打印 `struct { char a; double b; int c; }` 的 `sizeof` 与各成员偏移，再重排成员让它变小。
2. 用 `alignas(64)` 定义两个计数器，起两个线程分别自增，比较有无填充的耗时。
3. 用 placement new 在 `std::byte` 缓冲区上构造一个 `std::vector<int>`，用完显式析构。
4. 用 `mdspan` 表示一个 3×4 矩阵，按行优先打印，再按列优先打印（提示：`layout_left`）。
5. 用 `std::byte` 实现一个简单的字节缓冲区读取（读 int32）。
6. 解释为什么 `std::vector<bool>` 的 `[]` 返回的不是 `bool&`。

> [!success]- 参考答案
> 1.
> ```cpp
> struct A { char a; double b; int c; };   // sizeof == 24
> struct B { double b; int c; char a; };   // sizeof == 16
> ```
> 2.
> ```cpp
> struct alignas(64) Padded { std::atomic<int> v{0}; char pad[60]; };
> ```
> 有填充的版本通常快 2～4 倍。
> 3.
> ```cpp
> alignas(std::vector<int>) std::byte buf[sizeof(std::vector<int>)];
> auto* v = new (buf) std::vector<int>{1, 2, 3};
> std::println("{}", *v);
> v->~vector();
> ```
> 4.
> ```cpp
> std::vector<int> d(12);
> std::iota(d.begin(), d.end(), 1);
> std::mdspan row_major(d.data(), 3, 4);
> std::mdspan<int, std::extents<std::size_t, 3, 4>, std::layout_left> col_major(d.data());
> ```
> 5.
> ```cpp
> std::array<std::byte, 4> raw{std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}};
> std::int32_t v = std::bit_cast<std::int32_t>(raw);
> ```
> 6. `vector<bool>` 是标准库的位压缩特化：每个 bool 占 1 bit，所以无法返回真正的 `bool&`，只能返回代理对象（能读能写但不能取地址）。

---

上一节：[[34 协程与 generator]] ｜ 下一节：[[36 分配器与 pmr]]
