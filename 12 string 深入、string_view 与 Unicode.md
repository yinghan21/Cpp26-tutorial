---
title: string 深入、string_view 与 Unicode
tags: [cpp, cpp26, 数据, 字符串]
chapter: 2
order: 12
created: 2026-11-05
---

# string 深入、`string_view` 与 Unicode

> [!info] 本节目标
> - 理解 `std::string` 的 SSO 短字符串优化，知道 `sizeof(string)` 为什么是 32
> - 掌握 `string_view` 的零拷贝优势与悬垂陷阱
> - 会用 `from_chars` / `to_chars` 做高性能数字转换
> - 分清字节、码位、字形：UTF-8 里 1 个汉字是 3 个字节
> - 知道什么时候该用 `u8string` / `char8_t`

> [!tip] 先看结论
> - `std::string` 有 SSO：≤15 字节的字符串**不分配堆内存**。
> - `string_view` 只存"指针 + 长度"，**绝不拥有数据**，返回它时要小心。
> - 数字 ↔ 字符串用 `std::from_chars` / `std::to_chars`（无异常、无 locale、最快）。
> - `std::string::size()` 是**字节数**，不是字符数。

## 一、`std::string` 的内存布局与 SSO

```cpp
std::string short_s = "abc";
std::string long_s(100, 'x');
std::println("sizeof={} 短 capacity={} 长 capacity={}", sizeof(short_s), short_s.capacity(), long_s.capacity());
```

实测：

```text
sizeof(string)=32 短 capacity=15 长 capacity=111
```

这 32 字节里有一块**内联缓冲区**（MSVC 15 字节 + 结尾 '\0'）。所以短字符串：

| 长度 | 是否分配堆 |
| --- | --- |
| ≤ 15 字节 | 否（存在对象内部） |
| > 15 字节 | 是（堆上分配，capacity 通常按 1.5 倍涨） |

这就是"短字符串不慢"的原因，也是 `sizeof(std::string) == 32` 而不是 8 的原因。**代价是：每个 string 自带 32 字节开销**，大量小字符串要小心。

## 二、常用 API 全表

```cpp
std::string s = "hello, world";
```

| 操作 | 代码 | 复杂度 |
| --- | --- | --- |
| 长度 | `s.size()` / `s.length()` | O(1) |
| 判空 | `s.empty()` | O(1) |
| 查找子串 | `s.find("world")` → 7 | O(n·m) |
| 反向查找 | `s.rfind('o')` | O(n) |
| 子串（拷贝） | `s.substr(0, 5)` → `"hello"` | O(k) |
| 前缀/后缀/包含（C++20/23） | `starts_with` / `ends_with` / `contains` | O(k) |
| 替换 | `s.replace(0, 5, "HI")` | O(n) |
| 追加 | `s += "x"` / `s.append(...)` | 均摊 O(1) |
| 删除 | `s.erase(pos, n)` | O(n) |
| 插入 | `s.insert(pos, "x")` | O(n) |
| 转 C 字符串 | `s.c_str()` / `s.data()` | O(1) |
| 数字转换 | `std::stoi` / `std::from_chars` | 见下 |

## 三、`string_view`：只借不还

```cpp
std::string_view sv = s;             // 不拷贝
sv.substr(0, 2);                     // O(1)，也是 view
sum_span(sv);                        // 传给函数不拷贝
```

`string_view` 只有两个成员：`const char*` + `size`（所以 `sizeof == 16`）。它**不拥有**数据：

> [!warning] 三个必踩的悬垂场景
> ```cpp
> // 1) 指向临时对象
> std::string_view v1 = std::string{"temp"};   // 临时对象已销毁 → 悬垂
>
> // 2) 函数返回局部 string 的 view
> std::string_view bad() { std::string s = "x"; return s; }   // 悬垂
>
> // 3) 容器扩容后旧 view 失效
> std::string_view v3 = v[0];
> v.push_back("more");      // 扩容，v[0] 的内存搬走了
> ```
> 实测里"读到 4 个字节但内容是垃圾"是典型症状：**UB 不一定崩**。

正确用法：**只作函数参数，或只在同一作用域内短期使用**。

```cpp
void log_it(std::string_view msg);   // ✅ 好：调用方保证生命周期
std::string_view store();            // ❌ 坏：返回 view
```

## 四、数字 ↔ 字符串：`from_chars` / `to_chars`

```cpp
int value = 0;
std::string_view num = "12345";
auto [ptr, ec] = std::from_chars(num.data(), num.data() + num.size(), value);
std::println("from_chars={} ec={}", value, static_cast<int>(ec));

char buf[32];
auto res = std::to_chars(buf, buf + sizeof(buf), 3.14159, std::chars_format::fixed, 2);
std::println("to_chars={}", std::string_view(buf, res.ptr - buf));
```

实测：

```text
from_chars=12345 ec=0
to_chars=3.14
```

对比三种转换方式：

| 方式 | 抛异常 | 分配内存 | 受 locale 影响 | 速度 |
| --- | --- | --- | --- | --- |
| `std::stoi` / `std::to_string` | 是 | 是（`to_string`） | 是 | 慢 |
| `std::stringstream` | 否（看状态） | 是 | 是 | 最慢 |
| `std::from_chars` / `to_chars` | 否（返回 `errc`） | 否 | 否 | 最快 |

**解析外部数据（配置文件、网络包、CSV）一律用 `from_chars`。**

## 五、拼接的性能陷阱

```cpp
std::string acc;
for (int i = 0; i < 3; ++i) acc += std::to_string(i) + ",";
```

`std::to_string(i) + ","` 每轮都建一个临时 `string`，然后 `+=` 一次。3 轮 3 个临时对象，100 万轮就是 100 万个。

更好的写法：

```cpp
std::string acc;
acc.reserve(1024);
for (int i = 0; i < 3; ++i) {
    acc += std::to_string(i);   // 只 +=，不新建临时
    acc += ',';
}
```

或者用 `std::format` 一次成型（可读性最好）：

```cpp
auto s = std::format("{},{},{}", 0, 1, 2);
```

## 六、Unicode：字节 ≠ 字符

```cpp
std::u8string u8 = u8"中文";
std::println("u8 字节数={}", u8.size());   // 6
```

实测：`u8 字节数=6 字符数(近似)=2`。

| 概念 | 含义 | "中文" 的值 |
| --- | --- | --- |
| 字节（byte） | UTF-8 编码后的长度 | 6 |
| 码位（code point） | Unicode 编号 | U+4E2D、U+6587 |
| 字形（grapheme） | 人眼看到的一个字 | 2 个汉字 |

所以 `std::string::size()` 永远给**字节数**。想按字符切分必须做 UTF-8 解码，标准库目前没有直接支持（`std::text_encoding` 在 C++26 才引入）。

实用建议：

1. 内部一律用 **UTF-8**（`std::string` + `/utf-8`）。
2. 只在"显示宽度对齐"时拆 UTF-8 序列（按首字节判断长度）。
3. `char8_t` / `u8string` 用于明确表达"这是 UTF-8"，与 `std::string` 之间需要 `reinterpret_cast`。

```cpp
std::u8string u8 = u8"中文";
std::string back(reinterpret_cast<const char*>(u8.data()), u8.size());
std::println("{}", back);      // 中文
```

## 完整示例（已实测）

`samples\12_string.cpp` 输出：

```text
sizeof(string)=32 短 capacity=15 长 capacity=111
size=12 find=7 substr=hello
contains=true starts_with=true ends_with=true
replace -> HI, world
view=HI, world size=9 substr_view=HI
bad view size（UB 演示，可能仍显示 4）=4
from_chars=12345 ec=0
to_chars=3.14
acc=0,1,2, (每轮都产生临时 string)
u8 字节数=6 字符数(近似)=2
转回 string: 中文
```

```powershell
.\build.ps1 samples\12_string.cpp
```

`bad view size=4` 那一行特意演示了"悬垂 view 还能读出数字"——这正是最危险的地方。

## 常见错误与排查

| 现象 | 报错/后果 | 原因 | 改法 |
| --- | --- | --- | --- |
| `string_view` 内容是垃圾 | 无（UB） | 底层对象已销毁 | 别返回 view，改返回 `string` |
| `s.size()` 小于字数 | 无 | 是字节数 | 用 UTF-8 解码或接受字节语义 |
| `s + 1` 编译错 | `C2110` 等 | 指针算术或类型不对 | 用 `s += std::to_string(1)` |
| `stoi` 抛 `invalid_argument` | 异常 | 输入不是数字 | 用 `from_chars` |
| 中文字符串比较不相等 | 无 | 不同编码 | 统一 UTF-8 |
| `substr` 循环很慢 | 无 | 每次拷贝 | 用 `string_view::substr` |
| `c_str()` 后 string 变了 | 指针失效 | 底层重新分配 | 用完就取，别存 |

## 练习

1. 打印 `sizeof(std::string)`、`sizeof(std::string_view)`、`sizeof(std::string{})`，解释差异。
2. 用 `string_view` 写一个 `count_words(std::string_view)`，统计空格分隔的单词数。
3. 用 `from_chars` 解析 `"3.14abc"`，打印解析出的数和"停在了第几个字符"。
4. 用 `to_chars` 把 `255` 分别按十进制、十六进制写入缓冲区。
5. 统计一个 UTF-8 字符串里的**字节数**和**码位数**（提示：按首字节判断后续字节数）。
6. 写一个函数把 `std::u8string` 安全转成 `std::string`，并说明为什么需要 `reinterpret_cast`。

> [!success]- 参考答案
> 1. `sizeof(std::string)==32`（含 SSO 缓冲区），`sizeof(std::string_view)==16`（指针 + 长度），`sizeof(std::string{})==32`（空串也不分配堆）。
> 2.
> ```cpp
> std::size_t count_words(std::string_view s) {
>     std::size_t n = 0; bool in = false;
>     for (char c : s) {
>         if (c == ' ') in = false;
>         else if (!in) { in = true; ++n; }
>     }
>     return n;
> }
> ```
> 3.
> ```cpp
> double v{}; std::string_view s = "3.14abc";
> auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
> std::println("v={} 停在偏移 {} ec={}", v, ptr - s.data(), static_cast<int>(ec));
> ```
> 输出 `v=3.14 停在偏移 4 ec=0`。
> 4.
> ```cpp
> char buf[16];
> auto r1 = std::to_chars(buf, buf + 16, 255);                    // "255"
> auto r2 = std::to_chars(buf, buf + 16, 255, 16);                // "ff"
> ```
> 5.
> ```cpp
> std::size_t codepoints(std::string_view s) {
>     std::size_t n = 0;
>     for (std::size_t i = 0; i < s.size(); ) {
>         unsigned char c = static_cast<unsigned char>(s[i]);
>         i += (c < 0x80) ? 1 : (c >> 5) == 0b110 ? 2 : (c >> 4) == 0b1110 ? 3 : 4;
>         ++n;
>     }
>     return n;
> }
> ```
> 6.
> ```cpp
> std::string to_utf8(const std::u8string& u8) {
>     return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
> }
> ```
> `char8_t` 与 `char` 是不同类型，标准没有提供隐式转换，必须显式重解释（保证底层布局一致，是安全的）。

---

上一节：[[11 容器与字符串]] ｜ 下一节：[[13 枚举、variant、optional、any 与 tuple]]
