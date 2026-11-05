---
title: 迭代器与 ranges 入门
tags: [cpp, cpp26, 数据, ranges]
chapter: 2
order: 14
created: 2026-11-05
---

# 迭代器与 ranges 入门

> [!info] 本节目标
> - 理解迭代器的五种类别与它们保证的能力
> - 知道 `range-for` 的完整展开过程
> - 会用 ranges 的核心 concept 与**投影**（projection）
> - 理解 view 的惰性求值，以及"view 不拥有数据"的后果
> - 认得几个经典的误用

> [!tip] 先看结论
> - 迭代器 = 泛化的指针；`begin()`/`end()` 划出**左闭右开**区间 `[begin, end)`。
> - ranges 让"算法 + 容器"不再需要写 `.begin(), .end()`。
> - **投影**是 ranges 最被低估的特性：`std::ranges::sort(v, {}, &T::field)` 一行搞定按字段排序。
> - view 只保存"怎么算"，遍历时才计算；它不拥有数据，源变了它也变、源死了它就悬垂。

## 一、迭代器：五种类别

| 类别 | 能力 | 典型容器 |
| --- | --- | --- |
| input | 单遍读 | `istream_iterator` |
| forward | 多遍读、可前移 | `forward_list`、`unordered_*` |
| bidirectional | 可后退 | `list`、`map`、`set` |
| random_access | 可跳转（`+n`）、可相减 | `vector`、`deque`、`array` |
| contiguous（C++20） | 内存连续，可转指针 | `vector`、`array`、`string`、`span` |

```cpp
std::vector<int> v{5, 2, 8, 1};
auto first = v.begin();
std::println("{} {}", *first, v.end() - first);   // 5 4
++first;
std::println("{}", *first);                        // 2
```

区间是**左闭右开**：`v.end()` 指向"最后一个元素的下一个位置"，`*v.end()` 是 UB。这是 `[begin, end)` 能表示空区间的关键。

## 二、`range-for` 的完整展开

```cpp
for (auto x : v) std::print("{} ", x);
```

编译器展开成：

```cpp
auto&& __r = v;
auto __b = __r.begin();
auto __e = __r.end();
for (; __b != __e; ++__b) {
    auto x = *__b;
    std::print("{} ", x);
}
```

由此可以推出三条规则：

1. 循环里**不能改变容器大小**（`__e` 会失效）。
2. **`auto` 会拷贝元素**，大对象要 `const auto&`。
3. 需要修改元素用 `auto&`。

## 三、ranges 的核心概念

```cpp
static_assert(std::ranges::range<decltype(v)>);
static_assert(std::ranges::sized_range<decltype(v)>);
static_assert(std::ranges::random_access_range<decltype(v)>);
```

实测：`range 概念检查通过`。

| concept | 含义 |
| --- | --- |
| `range` | 有 `begin`/`end` |
| `sized_range` | 能 O(1) 取 `size()` |
| `view` | 轻量、可拷贝、不拥有数据 |
| `input_range` / `forward_range` / `bidirectional_range` / `random_access_range` / `contiguous_range` | 迭代器能力递进 |
| `borrowed_range` | 可安全返回其迭代器 |

这些 concept 的实际作用：**算法签名用它们约束**，报错从"模板地狱"变成一句人话。

## 四、投影：`std::ranges` 的杀手特性

```cpp
struct Item { std::string name; int price; };
std::vector<Item> items{{"pen", 3}, {"book", 12}, {"cup", 5}};

std::ranges::sort(items, {}, &Item::price);                  // 按价格升序
std::ranges::sort(items, std::greater<>{}, &Item::price);    // 按价格降序
```

实测：

```text
pen(3) cup(5) book(12)
book(12) cup(5) pen(3)
```

第二个参数是**比较器**（空 `{}` 表示默认 `<`），第三个参数是**投影**（把元素映射成要比较的值）。以前要写的 lambda：

```cpp
std::ranges::sort(items, [](const Item& a, const Item& b) { return a.price < b.price; });
```

投影同样适用于 `find`、`max`、`count_if`、`all_of` 等几乎所有 ranges 算法：

```cpp
std::ranges::max(items, {}, &Item::price);            // 最贵的
std::ranges::find(items, "book", &Item::name);         // 按名字找
std::ranges::any_of(items, [](const Item& i) { return i.price > 10; });
```

## 五、view：惰性求值

```cpp
auto doubled = v | std::views::transform([](int x) { return x * 2; });
std::println("{}", doubled);      // [10, 4, 16, 2]
```

`transform` 不产生新容器，`doubled` 只保存"源 + 函数"。每次遍历才计算 —— 所以：

| 事实 | 后果 |
| --- | --- |
| 不分配内存 | 省内存、管线可以无限长 |
| 每次遍历都重算 | 遍历多次可能比"先 materialize 再遍历"慢 |
| 不拥有数据 | 源变了它变、源死了它悬垂 |

```cpp
std::vector<int> src{1, 2, 3};
auto view = src | std::views::transform([](int x) { return x + 1; });
src.push_back(4);
std::println("{}", view);    // [2, 3, 4, 5]：源变了，view 跟着变
```

如果确实要"定格"结果，用 `std::ranges::to<std::vector>()`（见 [[15 算法与 views 进阶]]）。

## 六、组合管线

```cpp
auto result = v
    | std::views::filter([](int x) { return x % 2 == 0; })
    | std::views::transform([](int x) { return x * x; })
    | std::views::take(2);
std::println("{}", result);      // [4, 64]
```

读法就是从左到右的数据流：先过滤偶数，再平方，再取前两个。比嵌套 `for` 循环清楚得多，而且编译器能优化成一层循环。

## 七、三个经典误用

```cpp
// 1) 返回指向局部的 view
auto make_view() {
    std::vector<int> local{1,2,3};
    return local | std::views::transform([](int x) { return x; });   // 悬垂
}

// 2) 在 range-for 里改容器
for (auto x : v) v.push_back(x);      // 迭代器失效

// 3) 对临时容器取 view
auto r = std::vector<int>{1,2,3} | std::views::filter([](int){ return true; });  // 源已销毁
```

## 完整示例（已实测）

`samples\14_iter_ranges.cpp` 输出：

```text
first=5 last-first=4
++前 5
++后 2
5 2 8 1
range 概念检查通过
pen(3) cup(5) book(12)
book(12) cup(5) pen(3)
未遍历前不计算；遍历结果 [10, 4, 16, 2]
filter+take 管线: [2, 8]
源变化后 view 也跟着变: [2, 3, 4, 5]
```

```powershell
.\build.ps1 samples\14_iter_ranges.cpp
```

## 常见错误与排查

| 现象 | 报错 | 原因 | 改法 |
| --- | --- | --- | --- |
| `*v.end()` | 无（UB） | 尾后迭代器不能解引用 | 用 `v.back()` |
| 算法不动容器 | 无（逻辑错） | 遍历时改了别的东西 | 用 `std::ranges::` 版本 |
| view 内容变空/乱 | 无（UB） | 源已销毁 | 用 `ranges::to` 定格 |
| `sort(v)` 编译错 | `C3861` | 没写 `std::ranges::sort` | 加 `ranges::` 前缀 |
| 投影参数搞混 | 编译错 | `sort(v, comp, proj)` 顺序 | 第二个是比较器，第三个是投影 |
| `distance` 很慢 | 无 | 用了 forward 迭代器 | 用 `sized_range` 的 `size()` |

## 练习

1. 用迭代器手写一个 `find_int`，找到返回迭代器，找不到返回 `end()`。
2. 用 `std::ranges::sort` + 投影把一组 `{name, score}` 按分数降序排。
3. 用 `std::ranges::find` + 投影按名字查找元素。
4. 写一条管线：取 `1..100`，筛出能被 7 整除的，平方，取前 5 个，收集到 `vector`。
5. 解释为什么 `auto view = v | std::views::transform(f);` 之后再 `v.clear()` 会出问题。
6. 用 `static_assert` 验证 `std::vector<int>` 是 `contiguous_range`，而 `std::list<int>` 不是。

> [!success]- 参考答案
> 1.
> ```cpp
> auto find_int(std::vector<int>& v, int target) {
>     for (auto it = v.begin(); it != v.end(); ++it) if (*it == target) return it;
>     return v.end();
> }
> ```
> 2.
> ```cpp
> struct Row { std::string name; int score; };
> std::ranges::sort(rows, std::greater<>{}, &Row::score);
> ```
> 3.
> ```cpp
> auto it = std::ranges::find(rows, "YG", &Row::name);
> if (it != rows.end()) std::println("{}", it->score);
> ```
> 4.
> ```cpp
> auto out = std::views::iota(1, 101)
>          | std::views::filter([](int x) { return x % 7 == 0; })
>          | std::views::transform([](int x) { return x * x; })
>          | std::views::take(5)
>          | std::ranges::to<std::vector>();
> ```
> 5. `view` 保存的是指向 `v` 的迭代器范围；`clear()` 后元素被销毁，再遍历 view 就是访问已释放内存（UB）。要么在 clear 之前 materialize，要么保证 view 生命周期不超过源。
> 6.
> ```cpp
> static_assert(std::ranges::contiguous_range<std::vector<int>>);
> static_assert(!std::ranges::contiguous_range<std::list<int>>);
> ```

---

上一节：[[13 枚举、variant、optional、any 与 tuple]] ｜ 下一节：[[15 算法与 views 进阶]]
