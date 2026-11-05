---
title: 测试、调试与 sanitizer
tags: [cpp, cpp26, 工程]
chapter: 5
order: 38
created: 2026-11-05
---

# 测试、调试与 sanitizer

> [!info] 本节目标
> - 用 `static_assert` 与断言做"免费测试"
> - 手写一个 30 行的极简测试框架（不依赖第三方）
> - 会用表驱动测试与异常测试
> - 会用 `std::stacktrace` 与 `std::source_location` 定位问题
> - 知道常见崩溃类型的定位流程

> [!tip] 先看结论
> - `static_assert` 是编译期测试：不要钱、不会漏。
> - 测试框架成本极低，手写 30 行就够用；关键是**每个 bug 都补一条用例**。
> - `assert` 在 Release 下消失，**不能**用于输入校验。
> - `std::stacktrace` 能直接打印调用栈——比 `printf` 调试高一个档次。

## 一、三层检查

| 层次 | 工具 | 时机 | Release 下 |
| --- | --- | --- | --- |
| 编译期 | `static_assert` | 编译 | 保留（已检查） |
| 开发期 | `assert` | 运行 | 移除（`NDEBUG`） |
| 交付期 | 显式 `if` + 异常/`expected` | 运行 | 保留 |

```cpp
static_assert(sizeof(int) == 4, "int 必须是 4 字节");
static_assert(add(1, 2) == 3, "编译期就能验证纯函数");
```

## 二、30 行极简测试框架

```cpp
struct TestStats { int passed = 0; int failed = 0; };
TestStats g_stats;

void check(bool cond, std::string_view expr, std::string_view file, int line) {
    if (cond) ++g_stats.passed;
    else {
        ++g_stats.failed;
        std::println("FAIL {} ({}:{})", expr, file, line);
    }
}

#define CHECK(expr) check((expr), #expr, __FILE__, __LINE__)
```

用法：

```cpp
CHECK(add(1, 2) == 3);
CHECK(add(0, 0) == 0);
CHECK(add(1, 2) == 4);      // 故意失败
```

实测：

```text
FAIL add(1, 2) == 4 (C:\Users\Admin\Desktop\Cpp26-教程\samples\38_test.cpp:53)
通过 8 项，失败 1 项
```

三个设计要点：

| 要点 | 原因 |
| --- | --- |
| `#expr` 字符串化 | 失败时能看到"哪个表达式" |
| `__FILE__` / `__LINE__` | 直接定位 |
| 计数而不是立刻退出 | 一次跑完所有用例 |

## 三、表驱动测试

```cpp
struct Case { int a, b, expect; };

void test_add_table() {
    const std::array<Case, 4> cases{{{1, 2, 3}, {0, 0, 0}, {-1, 1, 0}, {100, -50, 50}}};
    for (const auto& c : cases) CHECK(add(c.a, c.b) == c.expect);
}
```

表驱动的好处：**加用例只需加一行数据**，不用复制粘贴测试代码。

## 四、异常测试

```cpp
bool threw = false;
try { div_safe(1, 0); }
catch (const std::invalid_argument&) { threw = true; }
CHECK(threw);
```

更完整的写法还应该校验异常信息：

```cpp
try {
    div_safe(1, 0);
    CHECK(false);                       // 没抛异常就是失败
} catch (const std::invalid_argument& e) {
    CHECK(std::string_view{e.what()} == "div by zero");
}
```

## 五、`std::stacktrace`：直接看调用栈

```cpp
void deep(int n, std::string_view tag) {
    if (n == 0) {
        auto trace = std::stacktrace::current();
        std::println("调用栈深度 {} 帧", trace.size());
        for (const auto& frame : trace | std::views::take(3)) {
            std::println("  {}", frame.description());
        }
        return;
    }
    deep(n - 1, tag);
}
```

实测：

```text
调用栈深度 8 帧 (tag=demo)
  38_test+0xF031
  38_test+0xF2AC
  38_test+0xF2AC
```

`std::source_location` 补充"代码在哪"：

```cpp
auto loc = std::source_location::current();
std::println("{}:{}", loc.file_name(), loc.line());
```

实测：`当前代码位置: ...\38_test.cpp:69`

> [!tip] 崩溃处理里打印 stacktrace
> 在生产环境装一个终止处理函数，崩溃时把 `std::stacktrace::current()` 写日志，比"用户说它崩了"有用一万倍。

## 六、常见崩溃类型与定位

| 崩溃码/现象 | 典型原因 | 定位手段 |
| --- | --- | --- |
| `0xC0000005`（访问冲突） | 空指针、野指针、越界 | Debug 构建 + 断点；`stacktrace` |
| `0xC00000FD`（栈溢出） | 无限递归、超大局部数组 | 看调用栈深度 |
| `0xC0000409`（栈缓冲溢出） | 缓冲区越界（`/GS` 检测到） | 检查数组写入 |
| 结果偶发错误 | 未初始化变量、数据竞争 | `/RTC1`、ThreadSanitizer |
| 内存持续增长 | 泄漏、循环引用 | 任务管理器 + 计数分配 |
| 只在 Release 崩 | UB 被优化放大 | 用 `-O0`/Debug 复现 |

## 七、sanitizer 与运行期检查

| 工具 | MSVC | GCC/Clang | 检查什么 |
| --- | --- | --- | --- |
| AddressSanitizer | `/fsanitize=address`（较新版本支持） | `-fsanitize=address` | 越界、use-after-free、双重释放 |
| UndefinedBehaviorSanitizer | 无 | `-fsanitize=undefined` | 有符号溢出、空指针解引用 |
| ThreadSanitizer | 无 | `-fsanitize=thread` | 数据竞争 |
| `/RTC1` | ✅ | — | 未初始化变量、栈帧检查 |
| `_ITERATOR_DEBUG_LEVEL` | ✅（Debug 默认 2） | — | 迭代器失效 |

本机验证建议：

```powershell
# 打开运行期检查 + 调试信息
.\build.ps1 samples\38_test.cpp -ExtraArgs "/Od /RTC1 /Zi"
```

> [!warning] 别在生产构建里开 sanitizer
> 它们会显著降低性能（2～20 倍）并使二进制变大。只在 CI/本地测试里用。

## 完整示例（已实测）

```powershell
.\build.ps1 samples\38_test.cpp
```

```text
FAIL add(1, 2) == 4 (C:\Users\Admin\Desktop\Cpp26-教程\samples\38_test.cpp:53)
通过 8 项，失败 1 项
调用栈深度 8 帧 (tag=demo)
  38_test+0xF031
  38_test+0xF2AC
  38_test+0xF2AC
当前代码位置: C:\Users\Admin\Desktop\Cpp26-教程\samples\38_test.cpp:69
```

## 常见错误与排查

| 现象 | 报错/后果 | 原因 | 改法 |
| --- | --- | --- | --- |
| `assert` 在 Release 不生效 | 无（逻辑错） | `NDEBUG` 移除 | 用显式检查 |
| 测试只跑第一个失败 | 无 | `assert` 直接终止 | 用计数式 `CHECK` |
| 栈溢出 | `0xC00000FD` | 递归无限/大数组 | 改迭代/用 vector |
| 崩溃无信息 | `0xC0000005` | 无调试信息 | `/Zi` + Debug 构建 |
| 迭代器失效未发现 | 无 | Release 不检查 | Debug 构建（`_ITERATOR_DEBUG_LEVEL=2`） |
| `static_assert` 用到非 constexpr 函数 | `C2131` | 函数没标 `constexpr` | 加 `constexpr` |

## 练习

1. 把本节极简框架扩展成支持 `CHECK_EQ(a, b)`（失败时打印两边的值）。
2. 为 `div_safe` 写三个用例：正常、除零、负数。
3. 用 `stacktrace` 写一个函数，在检测到非法参数时打印调用栈。
4. 用 `source_location` 写一个 `log(msg, loc = current())` 函数，自动带上文件行号。
5. 故意写一个越界访问，用 Debug 构建观察是否崩溃/断言。
6. 写一个"内存泄漏"的循环引用例子，用计数器观察析构未执行。

> [!success]- 参考答案
> 1.
> ```cpp
> #define CHECK_EQ(a, b) do { \
>     auto va = (a); auto vb = (b); \
>     if (va == vb) ++g_stats.passed; \
>     else { ++g_stats.failed; std::println("FAIL {} == {} ({} vs {}) at {}:{}", #a, #b, va, vb, __FILE__, __LINE__); } \
> } while (0)
> ```
> 2.
> ```cpp
> CHECK(div_safe(10, 2) == 5);
> CHECK_THROWS(div_safe(1, 0));
> CHECK(div_safe(-6, 3) == -2);
> ```
> 3.
> ```cpp
> void require(bool cond, std::string_view msg) {
>     if (!cond) {
>         std::println("断言失败: {}", msg);
>         for (const auto& f : std::stacktrace::current() | std::views::take(5)) std::println("  {}", f.description());
>         std::abort();
>     }
> }
> ```
> 4.
> ```cpp
> void log(std::string_view msg, const std::source_location& loc = std::source_location::current()) {
>     std::println("[{}:{}] {}", loc.file_name(), loc.line(), msg);
> }
> ```
> 5. Debug 构建下 `_ITERATOR_DEBUG_LEVEL=2` 会让 `v[v.size()]` 触发断言；Release 下则是 UB（可能静默通过）。
> 6. 两个 `shared_ptr` 互指，然后用 `Counting` 计数析构未发生 → 泄漏。

---

上一节：[[37 CMake 与现代工程结构]] ｜ 下一节：[[39 性能：复杂度、缓存与优化]]
