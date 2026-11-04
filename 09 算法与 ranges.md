---
title: 算法与 ranges
tags: [cpp, cpp26, 基础, stl]
chapter: 3
order: 9
created: 2026-11-05
---

# 算法与 ranges

> [!info] 本节目标
> - `std::ranges::sort` / `minmax` / `fold_left`
> - `views::filter` / `views::transform`（惰性管道）
> - 知道算法都在 `<algorithm>` 里（`import std;` 已包含）

## 一、最常用的四个

```cpp
import std;

int main() {
    std::vector<int> v{5, 3, 9, 1, 4, 8};

    std::ranges::sort(v);                       // 升序
    int total = std::ranges::fold_left(v, 0, std::plus{});   // 求和
    auto [lo, hi] = std::ranges::minmax(v);     // 最小/最大
    bool has_even = std::ranges::any_of(v, [](int x) { return x % 2 == 0; });
}
```

注意 `ranges::` 前缀：不用写 `v.begin(), v.end()`。

## 二、views：惰性管道

```cpp
auto even = v | std::views::filter([](int x) { return x % 2 == 0; });
auto squared = v | std::views::transform([](int x) { return x * x; });
auto first3 = v | std::views::take(3);

for (int x : even) std::print("{} ", x);
```

`views` **不产生新容器**，遍历时才计算（惰性）。想落地：

```cpp
auto vec = v | std::views::filter(...) | std::ranges::to<std::vector>();
```

## 三、完整示例（已实测）

```cpp
import std;

int main() {
    std::vector<int> v{5, 3, 9, 1, 4, 8};

    std::ranges::sort(v);
    std::print("sorted:");
    for (int x : v) std::print(" {}", x);
    std::println("");

    auto even = v | std::views::filter([](int x) { return x % 2 == 0; });
    std::print("even:");
    for (int x : even) std::print(" {}", x);
    std::println("");

    auto squared = v | std::views::transform([](int x) { return x * x; });
    std::print("squared:");
    for (int x : squared) std::print(" {}", x);
    std::println("");

    int total = std::ranges::fold_left(v, 0, std::plus{});
    std::println("total={}", total);

    auto [lo, hi] = std::ranges::minmax(v);
    std::println("min={} max={}", lo, hi);
}
```

实测输出：

```text
sorted: 1 3 4 5 8 9
even: 4 8
squared: 1 9 16 25 64 81
total=30
min=1 max=9
```

## 四、lambda：临时函数

```cpp
auto add = [](int a, int b) { return a + b; };
add(1, 2);

int base = 10;
auto with_capture = [base](int x) { return x + base; };   // 捕获外部变量
auto by_ref       = [&base](int x) { return x + (++base); };  // 引用捕获（能改）
```

| 捕获 | 含义 |
| --- | --- |
| `[x]` | 按值捕获 x（拷贝一份） |
| `[&x]` | 按引用捕获（能改外面） |
| `[=]` / `[&]` | 全部按值 / 全部按引用（少用，容易悬空） |

> [!warning] 别让 lambda 活得比被捕获的变量久
> 异步/存起来稍后调用时，按引用捕获会悬空。需要长期保存就用值捕获。

## 五、常见坑

| 坑 | 说明 |
| --- | --- |
| 忘了 `ranges::` | `std::sort(v)` 需要 `v.begin(), v.end()` |
| 对 `const` 容器排序 | 排序会改元素；拷贝一份再排 |
| 在 `filter` 里改元素 | views 是只读视图；要改用 `transform` + 落地 |
| 把 view 存起来给别处用 | view 引用原容器，原容器没了就悬空 |

## 练习

1. 对一个 `vector<int>` 排序后输出前 3 个（`views::take`）。
2. 用 `views::filter` + `views::transform` 求所有偶数的平方和。
3. 用 `ranges::count_if` 数出大于 5 的元素个数。

> [!success]- 参考答案
> 1.
> ```cpp
> std::ranges::sort(v);
> for (int x : v | std::views::take(3)) std::print("{} ", x);
> ```
> 2.
> ```cpp
> auto s = v | std::views::filter([](int x){ return x % 2 == 0; })
>            | std::views::transform([](int x){ return x * x; });
> int sum = 0;
> for (int x : s) sum += x;
> ```
> 3.
> ```cpp
> auto n = std::ranges::count_if(v, [](int x){ return x > 5; });
> ```

---

上一节：[[08 模板与 concept]] ｜ 下一节：[[10 错误处理]]