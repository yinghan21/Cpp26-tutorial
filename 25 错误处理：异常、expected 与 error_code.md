---
title: 错误处理：异常、expected 与 error_code
tags: [cpp, cpp26, 标准库]
chapter: 4
order: 25
created: 2026-11-05
---

# 错误处理：异常、`expected` 与 `error_code`

> [!info] 本节目标
> - 掌握异常机制、栈展开、`noexcept` 的真实影响
> - 会用 `std::expected` 表达"可能失败且有原因"
> - 会用 `error_code` 处理系统错误
> - 拿到一张"该用哪种"的决策树
> - 知道构造失败该怎么处理

> [!tip] 先看结论
> - 异常适合**罕见、异常**的错误；`expected` 适合**常规**的失败分支；`error_code` 留给系统/OS 错误。
> - `std::expected<T, E>` 是"要么值、要么错误"，强迫调用方处理。
> - 析构函数**不许**抛异常；移动构造要 `noexcept`。
> - `assert` 在 Release 下会消失，别用它做输入校验。

## 一、异常：抛与接

```cpp
int parse_div(int a, int b) {
    if (b == 0) throw std::invalid_argument("除数不能为 0");
    return a / b;
}

try {
    std::println("10/2={}", parse_div(10, 2));
    std::println("10/0={}", parse_div(10, 0));      // 这里抛出
} catch (const std::invalid_argument& e) {
    std::println("捕获: {}", e.what());
}
```

实测：

```text
10/2=5
捕获: 除数不能为 0
```

机制：`throw` 后**栈展开**（stack unwinding），沿途所有局部对象按逆序析构，然后跳到匹配的 `catch`。

| 要点 | 说明 |
| --- | --- |
| 按类型匹配 | `catch (const Base&)` 能接住派生类异常 |
| 捕获顺序 | 从最具体到最泛化 |
| `catch (...)` | 兜底，拿不到信息，慎用 |
| 异常安全 | 展开时局部对象一定被析构（RAII 保证不泄漏） |

### 异常安全的三个等级

| 等级 | 保证 | 用法 |
| --- | --- | --- |
| 基本 | 不泄漏、对象仍可用 | 最低要求 |
| 强 | 失败时回滚（要么全成功要么不变） | copy-and-swap |
| 不抛 | `noexcept`，绝不失败 | 移动构造、析构 |

## 二、`noexcept` 的真实影响

```cpp
struct MoveOnly {
    std::vector<int> data;
    MoveOnly(MoveOnly&&) noexcept = default;    // 必须有 noexcept
};
```

两个后果：

1. `vector` 扩容时**优先用移动**（因为移动被承诺不抛，能够保证强异常安全）。
2. 标记 `noexcept` 的函数里若真的抛异常 → `std::terminate`（直接崩，不给机会）。

| 场景 | 是否该标 `noexcept` |
| --- | --- |
| 移动构造/移动赋值 | ✅ 必须 |
| 析构函数 | ✅（默认就是） |
| 简单 getter/setter | ✅ |
| 可能失败的 IO | ❌ |
| 分配内存 | ❌ |

## 三、`std::expected`：不抛异常的错误传递

```cpp
std::expected<int, std::string> safe_div(int a, int b) {
    if (b == 0) return std::unexpected("除零");
    return a / b;
}

auto r1 = safe_div(10, 2);   // has_value=true, value=5
auto r2 = safe_div(10, 0);   // has_value=false, error="除零"
```

实测：

```text
r1 has_value=true value=5
r2 has_value=false error=除零
```

完整 API：

| 成员 | 作用 |
| --- | --- |
| `has_value()` / 转 `bool` | 是否有值 |
| `value()` | 取值，没有则抛 `bad_expected_access` |
| `value_or(d)` | 有值取值，否则默认值 |
| `error()` | 取错误 |
| `operator*` / `->` | 直接访问（前提是有值） |
| `and_then(f)` | `f` 返回 `expected`，串联 |
| `transform(f)` | 变换值 |
| `transform_error(f)` | 变换错误 |
| `or_else(f)` | 错误时兜底 |
| `value_or_else`（C++26） | 惰性默认值 |

链式示例（实测 `chained=40`）：

```cpp
auto chained = safe_div(100, 5)
    .and_then([](int v) { return safe_div(v, 5); })
    .transform([](int v) { return v * 10; })
    .or_else([](const std::string& e) -> std::expected<int, std::string> {
        std::println("or_else 处理: {}", e);
        return std::unexpected(e);
    });
```

## 四、`error_code`：系统错误

```cpp
std::error_code try_read(const std::filesystem::path& p) {
    std::error_code ec;
    (void)std::filesystem::file_size(p, ec);
    return ec;
}
```

实测：

```text
error_code value=2 message=The system cannot find the file specified.
```

| 类型 | 含义 |
| --- | --- |
| `std::error_code` | 具体错误（平台相关值 + 分类） |
| `std::error_condition` | 抽象条件（可移植比较） |
| `std::errc` | 标准错误条件枚举（`errc::file_not_found`） |

很多标准库 API 有两个重载：**抛异常版**和**收 `error_code&` 版**（`filesystem`、`from_chars`、`to_chars`）。需要"不抛异常"的性能敏感路径时用后者。

## 五、断言与快速失败

| 工具 | 时机 | Release 下 |
| --- | --- | --- |
| `static_assert` | 编译期 | 保留（编译期已检查） |
| `assert` | 运行期 | 被 `NDEBUG` 移除 |
| `std::unreachable()` | 运行期 | UB（告诉编译器此路不通） |
| 抛异常 | 运行期 | 保留 |

本教程示例用 `std::unreachable()` 表达"这里不可能发生"（因为它不依赖 `assert` 的宏开关）：

```cpp
if (x <= 0) std::unreachable();   // 契约式检查：违反即快速失败
```

## 六、决策树

```text
这个失败是"程序 bug"还是"外部输入/环境问题"？
├─ 程序 bug（不该发生） → 断言 / std::unreachable / 直接终止
└─ 外部问题
    ├─ 是常规分支（解析、校验、查表未命中） → std::optional / std::expected
    ├─ 需要携带详细原因（错误码+上下文） → std::expected<T, MyError>
    ├─ 是系统/IO 错误 → error_code（或 expected<T, error_code>）
    └─ 罕见且调用方无法局部处理 → 异常
```

经验值：

| 场景 | 选 |
| --- | --- |
| 解析用户输入 | `expected` |
| 查表/查缓存 | `optional` |
| 打开文件（要原因） | `expected<T, error_code>` |
| 构造函数失败 | 抛异常，或工厂函数返回 `expected` |
| 内存分配失败 | 异常（`bad_alloc`） |
| 游戏主循环里每帧的失败 | `expected`（异常太慢/不可预测） |

## 七、构造失败怎么办

构造函数没有返回值，三种做法：

```cpp
// 1) 抛异常（最常见）
Widget(int id) { if (id <= 0) throw std::invalid_argument("bad id"); }

// 2) 工厂 + expected（推荐用于"失败很常见"的场景）
static std::expected<Widget, std::string> create(int id) {
    if (id <= 0) return std::unexpected("bad id");
    return Widget{id};
}

// 3) 两段式 init()（老式，容易忘记检查，不推荐）
```

## 完整示例（已实测）

```powershell
.\build.ps1 samples\25_errors.cpp
```

```text
10/2=5
捕获: 除数不能为 0
r1 has_value=true value=5
r2 has_value=false error=除零
chained=40
error_code value=2 message=The system cannot find the file specified.
noexcept 移动后 v[0].size=3
assert 通过
```

## 常见错误与排查

| 现象 | 报错/后果 | 原因 | 改法 |
| --- | --- | --- | --- |
| 异常没人接 | `std::terminate` | 未捕获 | 加 `try/catch` 或 `noexcept` 边界 |
| 析构里抛异常 | `std::terminate` | 栈展开期间二次异常 | 析构函数内吞掉异常 |
| 移动构造没 `noexcept` | 性能退化 | `vector` 退回拷贝 | 加 `noexcept` |
| `expected::value()` 无值时调用 | 抛 `bad_expected_access` | 没检查 | 先 `has_value()` / 用 `value_or` |
| `error_code` 被忽略 | 逻辑错 | 返回值没检查 | `if (ec) { ... }` |
| `assert` 在 Release 失效 | 运行期越界 | `NDEBUG` 移除断言 | 改显式检查 |
| `catch (...)` 后无法处理 | 无信息 | 吞掉了类型 | 捕获具体类型 |

## 练习

1. 写 `std::expected<int, std::string> parse_int(std::string_view)`，用 `from_chars` 实现。
2. 把第 1 题改成返回 `std::optional<int>`，说明两者适用场景差异。
3. 用 `error_code` 重载写一个"尝试打开文件并返回大小"的函数（不抛异常）。
4. 写一个类，其移动构造标记 `noexcept`，用 `std::vector` 观察是否走移动（用计数器）。
5. 用 `and_then` 串三个可能失败的操作（解析 → 校验 → 计算）。
6. 解释为什么析构函数不能抛异常。

> [!success]- 参考答案
> 1.
> ```cpp
> std::expected<int, std::string> parse_int(std::string_view s) {
>     int v{};
>     auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
>     if (ec != std::errc{}) return std::unexpected("不是合法整数");
>     return v;
> }
> ```
> 2. `optional` 只表达"有没有"，`expected` 还能说明"为什么没有"。用户输入错误应该用 `expected`；"缓存未命中"用 `optional`。
> 3.
> ```cpp
> std::expected<std::uintmax_t, std::error_code> file_size_safe(const std::filesystem::path& p) {
>     std::error_code ec;
>     auto sz = std::filesystem::file_size(p, ec);
>     if (ec) return std::unexpected(ec);
>     return sz;
> }
> ```
> 4. 有 `noexcept` 时 `moves` 增长、`copies` 不增长；去掉后 `copies` 明显增加。
> 5.
> ```cpp
> auto r = parse_int("42")
>     .and_then([](int v) -> std::expected<int, std::string> { if (v < 0) return std::unexpected("负数"); return v; })
>     .and_then([](int v) { return std::expected<int, std::string>{v * 2}; });
> ```
> 6. 析构常在栈展开期间被调用；如果此时再抛异常，就有了两个同时活动的异常，C++ 运行时无法处理，直接 `std::terminate`。所以析构函数默认 `noexcept`，内部错误只能记录或吞掉。

---

上一节：[[24 变参模板与折叠表达式]] ｜ 下一节：[[26 格式化与输出]]
