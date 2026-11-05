---
title: 数组、array 与 span
tags: [cpp, cpp26, 数据]
chapter: 2
order: 09
created: 2026-11-05
---

# 数组、`std::array` 与 `std::span`

> [!info] 本节目标
> - 知道原生数组什么时候有用、什么时候会坑你（退化 / decay）
> - 会用 `std::array` 的常用 API
> - 掌握 `std::span` 的"借用"语义和它的生命周期约束
> - 能判断该用 `array`、`vector` 还是 `span`

> [!tip] 先看结论
> - 原生数组 `int a[10]` 传参就退化成指针，**长度信息消失**——几十年来的经典 bug 源。
> - 长度固定用 `std::array<T,N>`，长度可变用 `std::vector<T>`，**只在函数参数位置用 `std::span`**。
> - `span` 不拥有数据，底层容器销毁后它立刻悬垂。

## 一、原生数组：能用，但别当主力

```cpp
int raw[4]{1, 2, 3, 4};
std::println("sizeof(raw)={} 元素数={}", sizeof(raw), sizeof(raw) / sizeof(raw[0]));
int* p = raw;                       // 退化成指针
std::println("sizeof(p)={}", sizeof(p));
```

实测：

```text
sizeof(raw)=16 元素数=4
sizeof(p)=8
```

数组名在**大多数表达式**里退化为指向首元素的指针，于是：

| 问题 | 后果 |
| --- | --- |
| 传参后 `sizeof` 变成指针大小 | 函数里算不出元素个数，必须额外传 `n` |
| 越界不检查 | `a[10] = 1` 静默破坏内存 |
| 不能赋值/返回/比较 | `b = a` 编译失败 |
| 不知道自己的长度 | 只能靠人记 |

现代写法：**只在和 C 接口交互、或写底层缓冲区时用原生数组**，其余用 `std::array` / `std::vector`。

## 二、`std::array`：定长、零开销、值语义

```cpp
std::array<int, 4> arr{4, 3, 2, 1};
std::ranges::sort(arr);
std::println("size={} front={} back={}", arr.size(), arr.front(), arr.back());
std::println("{}", arr);      // [1, 2, 3, 4]
```

| 特性 | 说明 |
| --- | --- |
| 大小在类型里 | `array<int,4>` 和 `array<int,5>` 是不同类型 |
| 零开销 | `sizeof(std::array<int,4>) == 16`，没有额外指针 |
| 可赋值/可比较 | `a = b`、`a == b` 都支持 |
| 有 `size()` | 不用再 `sizeof/sizeof` |
| 支持 ranges | 直接 `std::ranges::sort(arr)` |
| 越界检查 | `.at(i)` 抛 `std::out_of_range`，`[]` 不检查 |

常用 API：

```cpp
arr.size();          // 4
arr.empty();         // false
arr.front(); arr.back();
arr.at(0);           // 带检查
arr.fill(0);         // 全部填 0
arr.data();          // 拿到原生指针（给 C 接口用）
std::get<0>(arr);    // 编译期下标（越界是编译错误！）
```

`std::get<N>` 的好处：**下标越界在编译期就报错**。

## 三、`std::span`：借用一段连续内存

`span` 是"指针 + 长度"的现代封装，**不拥有数据**：

```cpp
int sum_span(std::span<const int> xs) {
    int s = 0;
    for (int x : xs) s += x;
    return s;
}

std::vector<int> v{1, 2, 3};
sum_span(v);          // vector 直接传
int raw[3]{1,2,3};
sum_span(raw);        // 原生数组也能传
std::array<int,3> a{1,2,3};
sum_span(a);          // std::array 也能传
```

三个好处：

1. **一个参数搞定"指针 + 长度"**，不会漏传长度。
2. **不拷贝**，比 `const std::vector<int>&` 更通用（不必是 vector）。
3. **保留长度信息**，可以 `size()`、范围 for、`.subspan()`。

> [!warning] `span` 不拥有数据，底层容器死了它就悬垂
> ```cpp
> std::span<int> bad;
> {
>     std::vector<int> tmp{1,2,3};
>     bad = tmp;          // 记录 tmp 的地址
> }                       // tmp 销毁
> // bad 悬垂，任何访问都是 UB
> ```
> **规则：`span` 只作函数参数或短期局部变量，绝不长期保存、绝不返回。**

`span` 的关键操作：

```cpp
std::span<int> sv{v};
sv.size(); sv.empty();
sv[0]; sv.front(); sv.back();
sv.first(2);          // 前 2 个
sv.last(1);           // 后 1 个
sv.subspan(1, 2);     // 从下标 1 取 2 个
sv.data();            // 底层指针
```

改 `span` 的元素就是改原容器（实测 `v[0]=99 span size=3`）。

## 四、越界：`[]` 与 `.at()` 的区别

```cpp
try {
    std::println("{}", v.at(10));
} catch (const std::out_of_range& e) {
    std::println("at 抛异常: {}", e.what());
}
```

实测输出 `at 抛异常: invalid vector subscript`。

| 方式 | 越界行为 | 什么时候用 |
| --- | --- | --- |
| `v[i]` | **未定义行为**（读垃圾/可能崩） | 热循环且能保证不越界 |
| `v.at(i)` | 抛 `std::out_of_range` | 边界来自外部输入 |
| `std::get<N>(array)` | 编译期错误 | 定长且下标是常量 |

## 五、二维与多维

```cpp
int grid[2][3]{{1, 2, 3}, {4, 5, 6}};
std::println("grid[1][2]={} 行长={}", grid[1][2], sizeof(grid[0]) / sizeof(int));
```

`grid` 的内存是连续的 6 个 `int`，`grid[1][2]` 等价于 `*(*(grid + 1) + 2)`。

现代替代：

| 需求 | 用什么 |
| --- | --- |
| 定长二维小矩阵 | `std::array<std::array<int,3>,2>` |
| 动态二维 | `std::vector<std::vector<int>>`（不连续）或一维 `vector` + 手动下标 |
| 视图/切片/任意维度 | `std::mdspan`（C++23，见 [[35 内存、对齐与 mdspan]]） |

## 六、怎么选：一张决策表

| 场景 | 选择 |
| --- | --- |
| 长度编译期已知、要值语义 | `std::array<T, N>` |
| 长度运行期决定、要增删 | `std::vector<T>` |
| 函数参数接收"任意连续序列" | `std::span<const T>` |
| 只读字符串 | `std::string_view` |
| 要给 C API 传缓冲区 | `.data()`（配合 `size()`） |
| 多维数值计算 | `std::mdspan` |
| 需要哈希/树结构 | 别用数组，用 `map`/`unordered_map` |

## 完整示例（已实测）

`samples\09_array_span.cpp`：

```cpp
import std;

int sum_raw(const int* data, int n) {
    int s = 0;
    for (int i = 0; i < n; ++i) s += data[i];
    return s;
}

int sum_span(std::span<const int> xs) {
    int s = 0;
    for (int x : xs) s += x;
    return s;
}

int main() {
    // ---- 原生数组与退化 ----
    int raw[4]{1, 2, 3, 4};
    std::println("sizeof(raw)={} 元素数={}", sizeof(raw), sizeof(raw) / sizeof(raw[0]));
    int* p = raw;                                  // 退化成指针，长度信息丢失
    std::println("sizeof(p)={}", sizeof(p));
    std::println("sum_raw={} sum_span={}", sum_raw(raw, 4), sum_span(raw));

    // ---- std::array ----
    std::array<int, 4> arr{4, 3, 2, 1};
    std::ranges::sort(arr);
    std::println("array size={} front={} back={}", arr.size(), arr.front(), arr.back());
    std::println("{}", arr);                       // 直接打印容器

    // ---- span 借用 vector，不拷贝 ----
    std::vector<int> v{10, 20, 30};
    std::span<int> sv{v};
    sv[0] = 99;                                    // 改 span 就是改 vector
    std::println("v[0]={} span size={}", v[0], sv.size());

    // ---- 越界：at() 抛异常，[] 不检查 ----
    try {
        std::println("{}", v.at(10));
    } catch (const std::out_of_range& e) {
        std::println("at 抛异常: {}", e.what());
    }

    // ---- 二维数组 ----
    int grid[2][3]{{1, 2, 3}, {4, 5, 6}};
    std::println("grid[1][2]={} 行长={}", grid[1][2], sizeof(grid[0]) / sizeof(int));

    // ---- span 也能接数组 ----
    std::println("span of array sum={}", sum_span(sv));
}
```

```powershell
.\build.ps1 samples\09_array_span.cpp
```

实测输出：

```text
sizeof(raw)=16 元素数=4
sizeof(p)=8
sum_raw=10 sum_span=10
array size=4 front=1 back=4
[1, 2, 3, 4]
v[0]=99 span size=3
at 抛异常: invalid vector subscript
grid[1][2]=6 行长=3
span of array sum=149
```

读几行：

- `sizeof(p)=8`：指针大小，长度信息没了。这就是老 API 要同时传 `n` 的原因。
- `[1, 2, 3, 4]`：`std::println` 直接支持打印容器（range formatter）。
- `span of array sum=149`：`v` 此时是 `{99,20,30}`，和是 149——`sv[0]=99` 改的是原 vector。

## 常见错误与排查

| 现象 | 报错 | 原因 | 改法 |
| --- | --- | --- | --- |
| 函数里 `sizeof(arr)` 变成 8 | 无（逻辑错） | 数组退化成指针 | 传 `std::span` 或额外传长度 |
| `int a[n];`（n 是变量） | `C2131` | 变长数组不是标准 C++ | 用 `std::vector` |
| `span` 指向已销毁容器 | 无（UB） | 不拥有数据 | 只作参数用 |
| `array` 越界 `[]` | 无（UB） | 不检查 | 热路径外一律 `.at()` |
| `v.at(i)` 抛异常没人接 | 程序终止 | 没 try/catch | 加 `try`，或先判 `i < v.size()` |
| `std::get<5>(arr)` 报错 | `C2338` | 编译期下标越界 | 检查 N |

## 练习

1. 用 `std::array` 存 5 个成绩，求平均、最大、最小（用 `std::ranges`）。
2. 写 `void scale(std::span<double> xs, double k)`，把每个元素乘 `k`，用 `vector` 和 `std::array` 各测一次。
3. 用 `subspan` 打印一个序列的前 3 个和后 2 个。
4. 解释下面这段为什么错，并给出正确写法：
   ```cpp
   std::span<int> make() { std::vector<int> v{1,2,3}; return v; }
   ```
5. 写一个二维矩阵（一维 `vector` 展平 + 行列计算），实现 `at(r,c)` 访问。
6. 用 `std::get<1>(arr)` 取第二个元素，再故意写 `std::get<10>` 编译一次，记录报错。

> [!success]- 参考答案
> 1.
> ```cpp
> import std;
> int main() {
>     std::array<int, 5> s{88, 92, 79, 95, 60};
>     auto [mn, mx] = std::ranges::minmax(s);
>     int sum = std::ranges::fold_left(s, 0, std::plus<>());
>     std::println("avg={:.1f} min={} max={}", sum / 5.0, mn, mx);
> }
> ```
> 2.
> ```cpp
> void scale(std::span<double> xs, double k) { for (auto& x : xs) x *= k; }
> ```
> 3.
> ```cpp
> std::vector<int> v{1,2,3,4,5,6};
> std::span<int> s{v};
> std::println("{} {}", s.first(3), s.last(2));
> ```
> 4. 返回的 `span` 指向函数内的局部 `vector`，函数返回后容器已销毁，`span` 悬垂。正确做法：返回 `std::vector<int>`（值语义），或让调用方持有容器、只把 `span` 传进函数。
> 5.
> ```cpp
> struct Matrix {
>     int rows, cols;
>     std::vector<int> data;
>     int& at(int r, int c) { return data[static_cast<std::size_t>(r) * cols + c]; }
> };
> ```
> 6. `std::get<10>(arr)` 报 `error C2338: static_assert failed: 'array index out of bounds'` 一类的编译期错误——这是 `std::get` 相对 `[]` 的最大优势。

---

上一节：[[08 作用域、生命周期与存储期]] ｜ 下一节：[[10 指针、引用与 const 正确性]]
