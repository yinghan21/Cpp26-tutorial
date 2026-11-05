---
title: 编译期编程：constexpr 树
tags: [cpp, cpp26, 对象, 编译期]
chapter: 3
order: 23
created: 2026-11-05
---

# 编译期编程：constexpr 树

> [!info] 本节目标
> - 分清 `constexpr` / `consteval` / `constinit` 的职责
> - 会在编译期用容器和算法（C++20 起 `vector`/`string` 可用）
> - 会用 `if constexpr` 做"按类型分支"
> - 会用 lambda 初始化编译期表（最实用的技巧）
> - 知道编译期计算的代价：编译时间与代码膨胀

> [!tip] 先看结论
> - `constexpr` = **可以**在编译期算；`consteval` = **必须**在编译期算；`constinit` = 保证**静态初始化**。
> - 编译期计算的产物是**常量**，运行期零成本；但编译时间会变长。
> - `static_assert` 是免费的单元测试：验证不通过就直接编译失败。

## 一、三个关键字

```cpp
constexpr int factorial(int n) { return n <= 1 ? 1 : n * factorial(n - 1); }
consteval int square(int x) { return x * x; }
constinit int global_ready = 42;
```

| 关键字 | 用在函数上 | 用在变量上 | 求值时机 |
| --- | --- | --- | --- |
| `constexpr` | 可以被编译期调用 | 值必须在编译期确定 | 编译期优先，可退化运行期 |
| `consteval` | **必须**编译期调用 | — | 只能编译期 |
| `constinit` | — | 保证静态初始化 | 编译期初始化，运行期仍可改 |

```cpp
int n = 7;
std::println("{}", factorial(n));   // constexpr 函数也能接收运行期参数
// square(n);                       // 错误：consteval 参数必须是常量
```

## 二、编译期容器（C++20 起）

```cpp
constexpr std::size_t count_even(std::size_t n) {
    std::vector<int> v;
    for (std::size_t i = 0; i < n; ++i) v.push_back(static_cast<int>(i));
    return std::ranges::count_if(v, [](int x) { return x % 2 == 0; });
}
static_assert(count_even(10) == 5);
```

限制（编译期）：

| 允许 | 不允许 |
| --- | --- |
| `new`/`delete`（必须成对、不能逃逸） | 读文件、网络、`rand()` |
| `std::vector`/`std::string`/`std::array` | `std::cout`（可用 `std::format`） |
| `throw`（但一旦执行就编译失败） | `reinterpret_cast` |
| lambda、虚函数（部分） | `goto` 跳出 constexpr 作用域 |

## 三、`if constexpr`：按类型分支

```cpp
template <class T>
std::string describe(T v) {
    if constexpr (std::integral<T>)
        return std::format("int {} ({} bit)", v, std::bit_width(static_cast<std::make_unsigned_t<T>>(v)));
    else if constexpr (std::floating_point<T>)
        return std::format("float {:.3f}", v);
    else
        return "other";
}
```

实测：`int 42 (6 bit) | float 2.500 | other`

关键点：`if constexpr` 的**未命中分支不会被实例化**，所以可以写"只对某些类型合法"的代码（例如 `v.size()`），这是普通 `if` 做不到的。

## 四、编译期查表：lambda 初始化（最实用）

```cpp
constexpr std::array<int, 8> fib_table = [] {
    std::array<int, 8> t{};
    t[0] = 0; t[1] = 1;
    for (std::size_t i = 2; i < t.size(); ++i) t[i] = t[i - 1] + t[i - 2];
    return t;
}();
static_assert(fib_table[7] == 13);
```

这个模式可以替代 90% 的"模板元编程"：**想要编译期算出来的表，就写个 constexpr lambda 跑一遍**。

同样的思路可以做：

- CRC 表、SIN 表、素数表、哈希常量表
- 编译期字符串处理（`constexpr` 版本的 `to_upper`、`split`）
- 编译期 JSON/配置解析（进阶）

## 五、`static_assert`：免费的测试

```cpp
static_assert(sizeof(int) == 4, "int 必须是 4 字节");
static_assert(factorial(5) == 120);
```

| 类型 | 检查时机 | 失败后果 |
| --- | --- | --- |
| `static_assert` | 编译期 | 编译失败（信息可控） |
| `assert` | 运行期（`/DNDEBUG` 下消失） | 运行中断言失败 |
| `std::unreachable()` | 运行期 | 未定义行为（告诉编译器"这里不可能"） |

## 六、代价：编译时间与代码膨胀

实测对比（本机 MSVC `/O2`）：

| 写法 | 运行期耗时 | 说明 |
| --- | --- | --- |
| `factorial(10)`（编译期已算出常量） | 0.000 ms | 1000 次循环被优化成常量赋值 |
| 带运行期参数的 `factorial(n)` | 取决于 n | 有实际计算 |

```text
运行期 1000 次 factorial(10) = 3628800000 耗时 0.000 ms
```

编译期计算的代价：

| 代价 | 表现 |
| --- | --- |
| 编译变慢 | 大量 `constexpr` 计算会显著增加编译时间 |
| 内存占用 | 编译器要保存中间常量 |
| 调试困难 | 编译期算错时只能看 `static_assert` 报错 |

> [!warning] 别把重活全塞进编译期
> 编译期适合"小而频繁"的表（几百到几万项）。真要处理 MB 级数据，用 `#embed`（C++26，见 [[46 C++26 新特性全览]]）或运行时加载。

## 完整示例（已实测）

```powershell
.\build.ps1 samples\23_constexpr.cpp
```

```text
factorial(10)=3628800
square(12)=144
global_ready=42
fib_table=[0, 1, 1, 2, 3, 5, 8, 13]
运行期 1000 次 factorial(10) = 3628800000 耗时 0.000 ms
int 42 (6 bit) | float 2.500 | other
```

## 常见错误与排查

| 现象 | 报错 | 原因 | 改法 |
| --- | --- | --- | --- |
| `constexpr` 函数里用 `std::cout` | `C2131` | 编译期不能做 IO | 用 `std::format` 返回字符串 |
| `consteval` 收到运行期参数 | `C7595` | 必须是常量表达式 | 改回 `constexpr` |
| `constexpr` 递归太深 | `C1202` / 编译极慢 | 模板/常量求值深度限制 | 改迭代写法 |
| `static_assert` 失败 | `C2338` | 断言条件为假 | 看表达式实际值 |
| 全局变量初始化顺序问题 | 无（运行时） | 动态初始化 | 用 `constinit` |
| 编译时间暴涨 | 无 | 大量编译期计算 | 拆表、改成运行期 |

## 练习

1. 写 `constexpr int fib(int n)`，用 `static_assert` 验证 `fib(20) == 6765`。
2. 用 `consteval` 写一个"编译期检查字符串长度不超过 N"的函数。
3. 用 lambda 初始化生成 `constexpr std::array<int, 10>` 的平方表。
4. 写 `template <class T> constexpr auto size_of(T)`，用 `if constexpr` 对容器返回 `size()`，对数组返回 `N`。
5. 用 `constinit` 定义全局对象，并说明它解决了什么问题。
6. 用 `static_assert` + `std::ranges::is_sorted` 验证一个编译期生成的数组有序。

> [!success]- 参考答案
> 1.
> ```cpp
> constexpr int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
> static_assert(fib(20) == 6765);
> ```
> 2.
> ```cpp
> consteval std::size_t checked_len(const char* s, std::size_t max) {
>     std::size_t n = 0;
>     while (s[n]) ++n;
>     if (n > max) throw "too long";     // 编译期一旦执行就编译失败
>     return n;
> }
> ```
> 3.
> ```cpp
> constexpr auto squares = [] {
>     std::array<int, 10> a{};
>     for (int i = 0; i < 10; ++i) a[i] = i * i;
>     return a;
> }();
> ```
> 4.
> ```cpp
> template <class T>
> constexpr auto size_of(const T& c) {
>     if constexpr (requires { c.size(); }) return c.size();
>     else return std::size(c);
> }
> ```
> 5. `constinit int g = init();` 保证 `g` 在加载时就用常量表达式初始化完成，避免"跨 TU 静态初始化顺序"导致的未初始化读取。
> 6.
> ```cpp
> constexpr auto asc = [] { std::array<int, 5> a{1,2,3,4,5}; return a; }();
> static_assert(std::ranges::is_sorted(asc));
> ```

---

上一节：[[22 现代 .h 与模块：头文件的正确写法]] ｜ 下一节：[[24 变参模板与折叠表达式]]
