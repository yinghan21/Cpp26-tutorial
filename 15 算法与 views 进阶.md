---
title: 算法与 views 进阶
tags: [cpp, cpp26, 数据, ranges]
chapter: 2
order: 15
created: 2026-11-05
---

# 算法与 views 进阶

> [!info] 本节目标
> - 熟悉常用算法分类，掌握排序四兄弟的区别
> - 掌握 C++23 的 `views`：`enumerate` / `chunk` / `zip` / `adjacent` / `repeat` / `split`
> - 会用 `std::ranges::to` 把视图定格成容器
> - 会用 `fold_left` / `fold_right`
> - 判断视图管线什么时候比手写循环慢

> [!tip] 先看结论
> - 排序记 4 个：`sort` 全排、`stable_sort` 保序、`partial_sort` 前 k、`nth_element` 第 n。
> - 删除元素用 `std::erase_if`，不要手写 `erase` 循环。
> - `enumerate` 解决下标、`zip` 解决并行遍历、`chunk` 解决分批。
> - 视图零分配，但**反复遍历**时反而更慢，用 `to` 定格。

## 一、常用算法分类表

| 类别 | 算法 | 复杂度 |
| --- | --- | --- |
| 查找 | `find`、`find_if`、`contains`、`count_if`、`any_of`、`all_of`、`none_of` | O(n) |
| 二分（需有序） | `lower_bound`、`upper_bound`、`equal_range`、`binary_search` | O(log n) |
| 排序 | `sort`、`stable_sort`、`partial_sort`、`nth_element` | 见下 |
| 极值 | `min`、`max`、`minmax`、`min_element`、`max_element` | O(n) |
| 变换 | `transform`、`replace`、`fill`、`iota` | O(n) |
| 分组 | `partition`、`is_sorted`、`equal`、`mismatch` | O(n) |
| 数值 | `fold_left`、`fold_right`、`accumulate`、`reduce` | O(n) |
| 集合（需有序） | `set_union`、`set_intersection`、`set_difference` | O(n+m) |

## 二、排序四兄弟

```cpp
std::vector<int> a{5, 1, 4, 2, 3};
std::ranges::sort(a);
std::ranges::nth_element(a, a.begin() + 2);    // 第 3 小就位
std::ranges::partial_sort(a, a.begin() + 3);   // 前 3 个有序
```

| 需求 | 用 | 复杂度 |
| --- | --- | --- |
| 全部有序 | `sort` | O(n log n) |
| 全部有序且相等元素保持原序 | `stable_sort` | O(n log² n) |
| 只要前 k 个有序 | `partial_sort` | O(n log k) |
| 只要第 k 小就位 | `nth_element` | 平均 O(n) |

> [!tip] 求 Top-K 别全排
> 10 万条取前 10，`partial_sort` 比重排快一个数量级（本机 100 万取前 10：约 4 ms vs 80 ms）。

## 三、删除-擦除

```cpp
std::vector<int> b{1, 2, 3, 4, 5, 6};
std::erase_if(b, [](int x) { return x % 3 == 0; });   // [1, 2, 4, 5]
```

老写法（会见到，别写）：`b.erase(std::remove_if(b.begin(), b.end(), pred), b.end());`

## 四、C++23 views 全表（实测）

| view | 作用 | 实测输出 |
| --- | --- | --- |
| `views::enumerate` | 带下标遍历 | `[(0, 1), (1, 2), (2, 3)]` |
| `views::chunk(n)` | 每 n 个一组 | `[[1, 2, 3], [4, 5, 6], [7, 8]]` |
| `views::zip(a, b)` | 并行遍历 | `[(1, 10), (2, 11), (3, 12)]` |
| `views::adjacent<2>` | 相邻 n 元组 | `[(1, 2), (2, 3), (3, 4)]` |
| `views::repeat(x, n)` | 重复 n 次 | `[7, 7, 7, 7]` |
| `views::join_with` | 展开并插分隔符 | 见示例 |
| `views::cartesian_product` | 笛卡尔积 | 见示例 |
| `views::split(d)` | 按分隔符切 | 见示例 |
| `views::iota(a, b)` | 等差序列 | 见示例 |
| `filter` / `transform` / `take` / `drop` / `reverse` | 基础变换 | — |

```cpp
std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};
for (auto [i, x] : v | std::views::enumerate) std::println("{}:{}", i, x);
for (auto part : v | std::views::chunk(3)) std::println("{}", part);
for (auto [a, b] : std::views::zip(v, std::views::iota(10))) std::println("{} {}", a, b);
```

### `ranges::to`：把视图定格成容器

```cpp
auto squares = v
             | std::views::filter([](int x) { return x % 2 == 1; })
             | std::views::transform([](int x) { return x * x; })
             | std::ranges::to<std::vector>();
// [1, 9, 25, 49]
```

| 场景 | 要不要 `to` |
| --- | --- |
| 只遍历一次 | 不要（省内存） |
| 要多次遍历 | 要（避免重算） |
| 要返回给调用方 | 要（否则 view 悬垂） |
| 要 `size()`/随机访问 | 要 |

## 五、`fold` 家族

```cpp
std::ranges::fold_left(v, 0, std::plus<>());    // 36
std::ranges::fold_right(v, 0, std::plus<>());   // 36
```

| 函数 | 方向 | 备注 |
| --- | --- | --- |
| `std::accumulate` | 左 | C++98，最常用 |
| `std::reduce` | 不确定 | 可并行，要求可结合 |
| `fold_left` | 左 | C++23，支持 ranges |
| `fold_right` | 右 | 对 `-`、`/` 结果不同 |

## 六、视图管线什么时候更慢

| 情况 | 原因 | 对策 |
| --- | --- | --- |
| 同一视图遍历多次 | 每次重算 | `ranges::to` 一次 |
| 深层嵌套（>5 层） | 迭代器类型爆炸 | 拆中间变量 |
| `filter` 后接 `size()` | 要遍历才知道大小 | 先计数或 `to` |
| 需要随机访问 | 视图可能 forward-only | `to<vector>` |

## 完整示例（已实测）

`samples\15_views.cpp` 输出：

```text
count_if even = 4
contains 5 = true
max = 8
sort          [1, 2, 3, 4, 5]
nth_element   [1, 2, 3, 4, 5]
partial_sort  [1, 2, 3, 4, 5]
erase_if      [1, 2, 4, 5]
enumerate     [(0, 1), (1, 2), (2, 3)]
chunk(3)      [[1, 2, 3], [4, 5, 6], [7, 8]]
zip           [(1, 10), (2, 11), (3, 12)]
adjacent<2>   [(1, 2), (2, 3), (3, 4)]
repeat        [7, 7, 7, 7]
to<vector>    [1, 9, 25, 49]
fold_left sum = 36
fold_right    = 36
big total = 7499850000
```

```powershell
.\build.ps1 samples\15_views.cpp
```

## 常见错误与排查

| 现象 | 报错 | 原因 | 改法 |
| --- | --- | --- | --- |
| `views::enumerate` 不存在 | `C2039` | 标准不够新 | 用 `/std:c++latest` |
| `to<std::vector>()` 模板错 | `C2672` | 少了 `ranges::` 或目标类型 | `std::ranges::to<std::vector<int>>()` |
| 视图遍历结果为空 | 无（UB） | 源已销毁 | 用 `to` 定格 |
| `sort` 后 `unique` 不彻底 | 无（逻辑错） | `unique` 只去相邻重复 | 先排序再 `unique` |
| `partial_sort` 结果不完整 | 无 | 只保证前 k 有序 | 需要全序用 `sort` |

## 练习

1. 用 `enumerate` 打印名字和下标。
2. 用 `chunk(4)` 把 1..10 分批求和。
3. 用 `zip` 遍历两个不等长 `vector`，打印配对。
4. 用 `adjacent<3>` 找出三个连续递增数。
5. 从 1..100 取能被 3 整除的数的平方，收集成 `vector` 并求和。
6. 比较 `partial_sort` 与 `sort` 在 100 万随机数取前 10 的耗时。

> [!success]- 参考答案
> 1.
> ```cpp
> for (auto [i, name] : std::vector<std::string>{"a","b"} | std::views::enumerate)
>     std::println("{}: {}", i, name);
> ```
> 2.
> ```cpp
> for (auto batch : std::views::iota(1, 11) | std::views::chunk(4))
>     std::println("{}", std::ranges::fold_left(batch, 0, std::plus<>()));
> ```
> 3.
> ```cpp
> std::vector<int> a{1,2,3}, b{10,20};
> for (auto [x, y] : std::views::zip(a, b)) std::println("{} {}", x, y);   // 按最短截断
> ```
> 4.
> ```cpp
> std::vector<int> v{1,2,3,5,4,6,7,8};
> for (auto [a,b,c] : v | std::views::adjacent<3>)
>     if (a < b && b < c) std::println("{},{},{}", a, b, c);
> ```
> 5.
> ```cpp
> auto nums = std::views::iota(1, 101)
>           | std::views::filter([](int x) { return x % 3 == 0; })
>           | std::views::transform([](int x) { return x * x; })
>           | std::ranges::to<std::vector>();
> std::println("sum={}", std::ranges::fold_left(nums, 0, std::plus<>()));
> ```
> 6. 本机量级：`partial_sort` 约 4 ms，`sort` 约 80 ms。

---

上一节：[[14 迭代器与 ranges 入门]] ｜ 下一节：[[16 类与对象：构造、析构与 RAII]]
