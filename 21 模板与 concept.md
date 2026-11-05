---
title: 模板与 concept
tags: [cpp, cpp26, 对象, 泛型]
chapter: 3
order: 21
created: 2026-11-05
---

# 模板与 concept

> [!info] 本节目标
> - 掌握函数模板、类模板、非类型模板参数
> - 用 `concept` 约束模板，让错误发生在调用点
> - 知道特化、偏特化与重载优先级
> - 理解"模板为什么必须写在接口里"
> - 会读最常见的模板报错

> [!tip] 先看结论
> - 模板是**编译期代码生成器**，零运行期开销。
> - 90% 的场景只需要 `concept` + `requires`。
> - 模板定义必须在**使用它的翻译单元可见**（头文件或模块接口）。

## 一、函数模板

```cpp
template <class T>
T twice(T v) { return v + v; }

twice(2);                    // T = int
twice(1.5);                  // T = double
twice(std::string{"ab"});    // T = std::string
```

实测：`twice(2)=4 twice(1.5)=3 twice(string)=abab`

推导要点：

| 情况 | 结果 |
| --- | --- |
| `twice(2)` | `T = int` |
| 多参数各推出不同 T | `C2782` 推导冲突 |
| `twice<int>(2.5)` | 显式指定，实参被转换 |

## 二、类模板与非类型模板参数

```cpp
template <class T>
struct Box {
    T value;
    explicit Box(T v) : value(std::move(v)) {}
    T get() const { return value; }
};

template <std::size_t N>
struct FixedBuffer {
    std::array<char, N> data{};
    static constexpr std::size_t capacity = N;
};
```

实测：`capacity=16 sizeof=16`。类模板成员**只在使用时实例化**。

## 三、`concept` 与 `requires`

```cpp
template <class T>
concept Addable = requires(T a, T b) {
    { a + b } -> std::same_as<T>;
    { a += b } -> std::same_as<T&>;
};

template <Addable T>
T sum3(const std::vector<T>& v) {
    T acc{};
    for (const auto& x : v) acc += x;
    return acc;
}
```

`requires` 的四种形式：

| 形式 | 含义 |
| --- | --- |
| `{ expr; }` | 表达式合法即可 |
| `{ expr } -> Concept` | 合法且返回类型满足约束 |
| `typename T::value_type;` | 类型成员存在 |
| `requires sizeof(T) > 4;` | 嵌套布尔要求 |

实测：`sum3 int=6`、`sum3 string=ab`。`vector<int>` 没有 `+=`，实例化会在**调用点**报"不满足约束"。

### 四种写法

```cpp
template <class T> requires Addable<T> T f1(T);
template <Addable T> T f2(T);                    // 简写（推荐）
template <class T> T f3(T) requires Addable<T>;
auto f4(Addable auto x) { return x; }
```

### 标准 concepts 速查

| concept | 含义 |
| --- | --- |
| `std::integral` / `std::floating_point` | 整数 / 浮点 |
| `std::same_as<T,U>` / `std::convertible_to<F,T>` | 相同 / 可转换 |
| `std::derived_from<D,B>` | 继承 |
| `std::copyable` / `std::movable` | 可拷贝 / 可移动 |
| `std::totally_ordered` | 支持全套比较 |
| `std::ranges::range` / `view` | 范围能力 |
| `std::invocable<F, Args...>` | 可调用 |

## 四、约束下的重载

```cpp
void describe(std::integral auto v) { std::println("integral {}", v); }
void describe(std::floating_point auto v) { std::println("floating {:.2f}", v); }
describe(1);      // integral 1
describe(2.5);    // floating 2.50
```

优先级：**非模板 > 约束更强的模板 > 泛型模板**。

## 五、特化与偏特化

```cpp
template <class T> struct TypeName { static constexpr const char* value = "unknown"; };
template <> struct TypeName<int> { static constexpr const char* value = "int"; };
template <class T> struct TypeName<std::vector<T>> { static constexpr const char* value = "vector<T>"; };
```

实测：`int unknown vector<T>`。

> [!warning] 函数模板不支持偏特化，全特化也容易"看起来没生效"
> 用重载或 `if constexpr` 代替函数模板特化。

## 六、模板的编译模型

```text
每个 T 都要生成一份代码 → 编译器必须看到完整定义
→ 定义要放 .h / .ixx（被包含或导入）
→ 放 .cpp 里就会出现 LNK2019
```

| 组织方式 | 说明 |
| --- | --- |
| 全写在头文件 | 传统 |
| 声明 + 末尾 `#include "impl.tpp"` | 分离但不彻底 |
| 模块接口单元里 `export` | 现代（第 22 节） |
| 显式实例化 `template struct Box<int>;` | 减少编译时间 |

## 七、读模板报错

```text
error C2676: 二元"+": "std::vector<int>" 不定义该运算符...
note: 查看对正在编译的函数 模板 实例化 "T twice<int>(T)" 的引用
```

三步法：

1. 看最后一条 `note`（谁实例化了什么）；
2. 看第一个 `error`（真正的类型不匹配点）；
3. 中间一长串 "see reference to..." 全部忽略。

有 `concept` 后变成 `error C7602: "twice": 不满足关联的约束` + 一行说明。

## 完整示例（已实测）

```powershell
.\build.ps1 samples\21_templates.cpp
```

```text
twice(2)=4 twice(1.5)=3 twice(string)=abab
Box=42
sum3 int=6
sum3 string=ab
integral 1
floating 2.50
capacity=16 sizeof=16
int unknown vector<T>
```

## 常见错误与排查

| 现象 | 报错 | 原因 | 改法 |
| --- | --- | --- | --- |
| 模板定义在 .cpp | `LNK2019` | 实例化时看不到定义 | 移到头/模块接口 |
| 推导冲突 | `C2782` | 多参数推出不同 T | 显式 `f<int>(...)` |
| 用法不满足约束 | `C7602` / `C2676` | 类型不支持该操作 | 加 `concept` 或换实现 |
| 函数模板特化不生效 | 无（逻辑错） | 特化不参与重载 | 改用重载 |
| 实例化过深 | `C1202` | 递归模板 | 拆解或用 `constexpr` |

## 练习

1. 写 `template <class T> T max_of(const std::vector<T>&)`，用 `std::totally_ordered` 约束。
2. 写 `template <std::size_t N> struct Matrix`，提供 `trace()`。
3. 写 `concept HasSize`，要求有 `size()` 且可转 `std::size_t`。
4. 用两个重载 `print_all(std::ranges::range auto)` 与 `print_all(std::integral auto)` 测试选择。
5. 用 `if constexpr` 写 `show(T)`：整数打印位数、浮点打印两位小数。

> [!success]- 参考答案
> 1.
> ```cpp
> template <std::totally_ordered T>
> T max_of(const std::vector<T>& v) { return *std::ranges::max_element(v); }
> ```
> 2.
> ```cpp
> template <std::size_t N>
> struct Matrix {
>     std::array<std::array<double, N>, N> a{};
>     double trace() const { double s = 0; for (std::size_t i = 0; i < N; ++i) s += a[i][i]; return s; }
> };
> ```
> 3.
> ```cpp
> template <class T>
> concept HasSize = requires(const T& t) { { t.size() } -> std::convertible_to<std::size_t>; };
> ```
> 4. `print_all(42)` → integral 版；`print_all(std::vector{1,2,3})` → range 版；`print_all(std::string{"ab"})` 两者都满足，range 版更特化。
> 5.
> ```cpp
> template <class T>
> void show(T v) {
>     if constexpr (std::integral<T>)
>         std::println("{} bit_width={}", v, std::bit_width(static_cast<std::make_unsigned_t<T>>(v)));
>     else if constexpr (std::floating_point<T>)
>         std::println("{:.2f}", v);
> }
> ```

---

上一节：[[20 智能指针与所有权]] ｜ 下一节：[[22 现代 .h 与模块：头文件的正确写法]]
