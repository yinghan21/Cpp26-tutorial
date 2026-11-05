---
title: 枚举、variant、optional、any 与 tuple
tags: [cpp, cpp26, 数据]
chapter: 2
order: 13
created: 2026-11-05
---

# 枚举、`variant`、`optional`、`any` 与 `tuple`

> [!info] 本节目标
> - 用 `enum class` 写出类型安全的枚举，掌握底层类型与 `using enum`
> - 用 `std::variant` 表达"多选一"，配合 `visit` 做分派
> - 用 `std::optional` 表达"可能没有值"，掌握单子操作
> - 知道 `std::any` 的代价，别滥用
> - 用 `tuple` + `apply` 处理"一次返回一组值"

> [!tip] 先看结论
> - 枚举一律用 `enum class`，别用裸 `enum`（会隐式转 int，污染命名空间）。
> - `variant` 是**类型安全**的 union，大小等于最大成员 + 标签。
> - `optional` 取代"用 -1 / 空指针表示失败"。
> - `optional` 与 `variant` 都支持链式操作（`and_then` / `transform` / `or_else`）。

## 一、`enum class`

```cpp
enum class Color : std::uint8_t { red, green, blue };
std::println("{}", static_cast<int>(std::to_underlying(Color::green)));   // 1
```

四个要点：

| 要点 | 说明 |
| --- | --- |
| 强类型 | `Color::red` 不能隐式转 `int`，必须 `static_cast` |
| 作用域 | 名字在 `Color::` 里，不会污染外层 |
| 底层类型 | `: std::uint8_t` 控制大小（省内存、便于序列化） |
| `using enum`（C++20） | 函数内 `using enum Color;` 后可写 `red` |

`switch` 穷尽检查（不写 `default`，漏了分支 `/W4` 报 `C4062`）：

```cpp
const char* name_of(Color c) {
    switch (c) {
        case Color::red:   return "red";
        case Color::green: return "green";
        case Color::blue:  return "blue";
    }
    return "?";
}
```

位标志场景用 `enum class` + `operator|`（见 [[18 运算符重载与三路比较]]）。

## 二、`std::variant`：类型安全的 union

```cpp
using Value = std::variant<int, double, std::string>;
std::vector<Value> vals{42, 3.14, std::string{"hi"}};
```

实测 `sizeof(Value) == 40`，最大成员 `std::string` 是 32 字节，其余是序号 + 对齐。

### `visit` + overloaded 惯用法

```cpp
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

for (const auto& v : vals) {
    std::visit(overloaded{
        [](int x) { std::println("int {}", x); },
        [](double x) { std::println("double {}", x); },
        [](const std::string& x) { std::println("string {}", x); }
    }, v);
}
```

实测输出：

```text
int 42
double 3.14
string hi
```

这个 `overloaded` 模板只有两行，是所有 C++17 项目里的必备小工具。

### variant 常用操作

| 操作 | 代码 |
| --- | --- |
| 判断当前类型 | `std::holds_alternative<std::string>(v)` |
| 取值（类型错抛异常） | `std::get<std::string>(v)` |
| 取值（类型错返回空指针） | `std::get_if<int>(&v)` |
| 取序号 | `v.index()` |
| 访问 | `std::visit(f, v)` |

### 什么时候用 variant 而不是继承

| 场景 | 选择 |
| --- | --- |
| 分支固定、数量少、值语义 | `variant`（无虚函数开销） |
| 需要开放扩展（第三方实现） | 继承 + 虚函数 |
| 状态机 | `variant` + `visit` 最清晰 |
| 树/表达式 AST | `variant` + `unique_ptr` |

## 三、`std::optional`：可能没有值

```cpp
std::optional<int> o = 4;
auto doubled = o.transform([](int x) { return x * 2; });
auto as_text = doubled.transform([](int x) { return std::format("<{}>", x); });
std::println("{}", as_text.value_or("none"));      // <8>

std::optional<int> none;
std::println("{}", none.or_else([] { return std::optional<int>{-1}; }).value());   // -1
```

实测：`<8>`、`-1`。

| 方法 | 作用 |
| --- | --- |
| `has_value()` / `operator bool` | 有没有值 |
| `value()` | 有值返回值，没有抛 `bad_optional_access` |
| `value_or(d)` | 没有就用默认值 |
| `*o` / `o->x` | 不检查（保证有值时用） |
| `and_then(f)` | `f` 返回 `optional`，链式串联 |
| `transform(f)` | `f` 返回普通值，自动包成 `optional` |
| `or_else(f)` | 为空时提供替代 |

> [!warning] 别把 `optional` 当"可以忽略的错误"
> `optional` 只表达"有没有"，不表达"为什么没有"。需要错误原因时用 `std::expected<T, E>`（[[25 错误处理：异常、expected 与 error_code]]）。

## 四、`std::any`：类型擦除的代价

```cpp
std::any box = 42;
std::println("{}", std::any_cast<int>(box));    // 42
try { std::any_cast<std::string>(box); }
catch (const std::bad_any_cast&) { std::println("类型不对"); }
```

| 维度 | `any` | `variant` |
| --- | --- | --- |
| 类型集合 | 任意 | 编译期固定 |
| 大小代价 | 可能堆分配 | 最大成员大小 |
| 取值 | 运行期 `any_cast`，可能抛异常 | `visit` 编译期分派 |
| 类型安全 | 弱（运行时） | 强（编译时） |

**能用 `variant` 就别用 `any`。** `any` 主要用于插件接口、脚本绑定这类"真的不知道类型"的边界。

## 五、`tuple` / `pair` / `apply`

```cpp
std::tuple<int, std::string, double> t{1, "one", 1.5};
auto [id, label, score] = t;
std::println("{} {} {}", id, label, score);     // 1 one 1.5
std::apply([](auto&&... args) { ((std::print("{} ", args)), ...); }, t);
```

| 用途 | 写法 |
| --- | --- |
| 构造 | `std::make_tuple(1, "a")`、`std::pair{1, 2.0}` |
| 取值 | `std::get<0>(t)` |
| 解包 | `auto [a, b] = t;` |
| 批量调用 | `std::apply(f, t)` |
| 比较 | `t1 == t2`（逐元素） |
| 拼接 | `std::tuple_cat(t1, t2)` |

> [!tip] 优先用 struct
> 返回 2～4 个值且有意义时，`struct` + 结构化绑定比 `tuple` 清楚得多：字段有名字，报错也好看。

## 完整示例（已实测）

`samples\13_sum_types.cpp` 输出：

```text
green underlying=1
int 42
double 3.14
string hi
size_of variant=40 最大成员=32
holds_alternative<string>? true
<8>
空值 or_else -> -1
any is int? true value=42
any_cast 类型不对会抛 bad_any_cast
1 one 1.5
get<1>=one
1 one 1.5
```

```powershell
.\build.ps1 samples\13_sum_types.cpp
```

## 常见错误与排查

| 现象 | 报错 | 原因 | 改法 |
| --- | --- | --- | --- |
| `int x = Color::red;` | `C2440` | enum class 不隐式转换 | `static_cast<int>` |
| `std::get<int>(variant)` 类型不符 | `std::bad_variant_access` | 当前不是该类型 | 先 `holds_alternative` 或用 `get_if` |
| `visit` 少写一个分支 | 编译期错误（lambda 不匹配） | 未覆盖所有类型 | 补分支或加泛型 `[](auto&&)` |
| `*optional` 时为空 | UB | 没检查 | 用 `value()` 或先判断 |
| `any_cast` 类型错 | `bad_any_cast` | 类型不符 | 先 `type() == typeid(T)` |
| `variant` 里有引用类型 | `C2664` | variant 不能存引用 | 存 `std::reference_wrapper` |

## 练习

1. 定义 `enum class Status { ok, warn, error }`，写函数转成字符串，并用 `switch` 穷尽处理。
2. 用 `variant<int, std::string, std::vector<int>>` 表示一个配置值，写 `visit` 打印每种类型。
3. 写 `std::optional<int> to_int(std::string_view)`，解析失败返回 `nullopt`。
4. 用 `optional::and_then` 串联两个可能失败的操作（如"解析字符串→再除以 2"）。
5. 用 `tuple` 返回 `{最小值, 最大值, 平均值}`，再用结构化绑定取出来打印。
6. 比较 `variant` 与继承实现同一个"图形面积"计算的代码量，说明各自优缺点。

> [!success]- 参考答案
> 1.
> ```cpp
> enum class Status { ok, warn, error };
> const char* to_string(Status s) {
>     switch (s) {
>         case Status::ok: return "ok";
>         case Status::warn: return "warn";
>         case Status::error: return "error";
>     }
>     return "?";
> }
> ```
> 2.
> ```cpp
> using Config = std::variant<int, std::string, std::vector<int>>;
> std::visit(overloaded{
>     [](int x) { std::println("int {}", x); },
>     [](const std::string& s) { std::println("str {}", s); },
>     [](const std::vector<int>& v) { std::println("vec {}", v); }
> }, Config{std::vector<int>{1,2}});
> ```
> 3.
> ```cpp
> std::optional<int> to_int(std::string_view s) {
>     int v{};
>     auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
>     if (ec != std::errc{}) return std::nullopt;
>     return v;
> }
> ```
> 4.
> ```cpp
> auto r = to_int("84").and_then([](int x) -> std::optional<int> {
>     if (x % 2) return std::nullopt;
>     return x / 2;
> });
> std::println("{}", r.value_or(-1));   // 42
> ```
> 5.
> ```cpp
> std::tuple<int, int, double> stats(const std::vector<int>& v) {
>     auto [mn, mx] = std::ranges::minmax(v);
>     double avg = std::ranges::fold_left(v, 0.0, std::plus<>()) / v.size();
>     return {mn, mx, avg};
> }
> ```
> 6. `variant` 版本是一个 `visit` + 几个 lambda，新增图形要改 `visit` 的所有调用点（但不会忘记处理）；继承版本新增图形只需加一个派生类（但可能漏改 switch/if-else 链）。选择取决于"形状变化频率"和"操作变化频率"。

---

上一节：[[12 string 深入、string_view 与 Unicode]] ｜ 下一节：[[14 迭代器与 ranges 入门]]
