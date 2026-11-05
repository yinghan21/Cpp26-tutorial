---
title: 实战：迷你 JSON 解析器
tags: [cpp, cpp26, 实战, 模块]
chapter: 6
order: 42
created: 2026-11-05
---

# 实战：迷你 JSON 解析器

> [!info] 本节目标
> - 用一个完整项目串起：模块、`variant`-式数据模型、递归下降、`expected`
> - 掌握"词法 → 语法 → 树 → 序列化"这条经典流水线
> - 学会用 `expected` 携带"错误 + 位置"
> - 体验模块化工程：`json.ixx` + `main.cpp`

> [!tip] 先看结论
> - 递归下降解析器的结构永远一样：`parse_value` 按首字符分派。
> - 错误信息带**偏移量**，比"解析失败"有用 100 倍。
> - 数据模型用"标签 + 各类型字段"的直白结构，比 `variant` 递归更好写（避免不完整类型问题）。

## 一、工程结构

```text
samples/proj_json/
├── json.ixx      # 模块 json：数据模型 + 词法 + 语法 + 序列化
└── main.cpp      # 使用方
```

```powershell
.\build-project.ps1 samples\proj_json
```

实测：

```text
[module] json
[compile] main.cpp
[link] proj_json
name    = cpp26
version = 26
stable  = true
tags[1] = modules
author  = YG
重新序列化: {"author":{"age":null,"name":"YG"},"name":"cpp26","stable":true,"tags":["modern","modules","ranges"],"version":26}
预期中的错误: 数组缺少 , 或 ] (偏移 13)
```

## 二、数据模型

```cpp
struct Value {
    enum class Kind { Null, Bool, Number, String, Array, Object };

    Kind kind = Kind::Null;
    bool boolean = false;
    double number = 0;
    std::string str;
    std::vector<Value> items;                  // 数组
    std::map<std::string, Value> members;      // 对象

    const Value* find(std::string_view key) const;
};
```

为什么不用 `std::variant<..., std::vector<Value>, std::map<std::string, Value>>`？

> [!warning] 递归 `variant` 的坑
> `Value` 里直接放 `std::vector<Value>` / `std::map<std::string, Value>` 是允许的（C++17 对不完整类型的支持），
> 但 `std::variant` 要求成员类型**完整**，递归时会失败。
> 想用 variant 就得引入 `std::unique_ptr` 间接层（如 `variant<..., std::unique_ptr<Object>>`）。
> 本节用"标签 + 字段"更直白。

## 三、词法与语法

解析器的骨架：

```cpp
class Parser {
    std::string_view s_;
    std::size_t pos_ = 0;

    std::expected<Value, std::string> parse_value() {
        skip_ws();
        char c = s_[pos_];
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') return parse_string_value();
        if (consume('t')) { expect("rue"); ... }
        if (consume('f')) { expect("alse"); ... }
        if (consume('n')) { expect("ull"); ... }
        return parse_number();
    }
};
```

核心技巧：

| 技巧 | 说明 |
| --- | --- |
| `parse_value` 按首字符分派 | JSON 的六种值首字符互不重叠 |
| 递归下降 | object/array 递归调用 `parse_value` |
| `skip_ws` 每步都调 | 空白可以出现在任意位置 |
| 错误带上 `pos_` | 定位问题 |

数字解析用 `std::from_chars`：

```cpp
auto [ptr, ec] = std::from_chars(s_.data() + start, s_.data() + pos_, d);
if (ec != std::errc{}) return std::unexpected(err("数字格式错误"));
```

## 四、错误处理：`expected` + 位置

```cpp
std::string err(std::string_view msg) const {
    return std::format("{} (偏移 {})", msg, pos_);
}
```

实测效果：

```text
预期中的错误: 数组缺少 , 或 ] (偏移 13)
```

`13` 正好指向 `{ "a": [1, 2 }` 里出错的位置——**这就是把 `pos_` 放进错误信息的价值**。

## 五、序列化

```cpp
std::string dump(const Value& v) {
    switch (v.kind) {
        case Value::Kind::Null: return "null";
        case Value::Kind::Bool: return v.boolean ? "true" : "false";
        case Value::Kind::Number: return std::format("{}", v.number);
        case Value::Kind::String: return std::format("\"{}\"", v.str);
        case Value::Kind::Array: { ... 递归 ... }
        case Value::Kind::Object: { ... 递归 ... }
    }
}
```

实测：

```text
{"author":{"age":null,"name":"YG"},"name":"cpp26","stable":true,"tags":["modern","modules","ranges"],"version":26}
```

注意：`std::map` 按 key 排序，所以输出顺序与输入不同——**JSON 对象的成员顺序本来就没有语义**，这是正确的。

## 六、用法

```cpp
const std::string text = R"({
    "name": "cpp26",
    "version": 26,
    "stable": true,
    "tags": ["modern", "modules", "ranges"],
    "author": { "name": "YG", "age": null }
})";

auto parsed = json::parse(text);
if (!parsed) { std::println("解析失败: {}", parsed.error()); return 1; }

const auto& v = *parsed;
std::println("name    = {}", v.find("name")->str);
std::println("tags[1] = {}", v.find("tags")->items[1].str);
std::println("author  = {}", v.find("author")->find("name")->str);
```

`R"(...)"` 是原始字符串字面量（不用转义反斜杠），写 JSON 时非常方便。

## 七、还能怎么扩展

| 扩展 | 难度 | 要点 |
| --- | --- | --- |
| 支持 `\uXXXX` 转义 | 中 | 需要 UTF-8 编码 |
| 支持科学计数法（完整） | 低 | `from_chars` 已支持 |
| 数字/字符串重复 key 处理 | 低 | `emplace` 还是 `[]` |
| 流式解析大文件 | 高 | 不能用 `string_view`，改成迭代器输入 |
| 保留成员顺序 | 中 | 用 `vector<pair<string,Value>>` 代替 `map` |
| 性能优化 | 中 | 跳过 `skip_ws` 的重复调用、用 `std::pmr` |

## 常见错误与排查

| 现象 | 报错/后果 | 原因 | 改法 |
| --- | --- | --- | --- |
| 数字解析为 0 | 无（逻辑错） | `from_chars` 的 `ec` 没检查 | 检查 `ec` |
| 空指针解引用 | `0xC0000005` | `find()` 返回 `nullptr` | 检查返回值 |
| 字符串未闭合导致越界 | UB | `while (pos_ < size)` 漏写 | 每步都检查边界 |
| 输出顺序与输入不同 | 无 | 用了 `std::map` | 无所谓，或换有序容器 |
| 模块里 `#include` 报错 | `C7611` | 模块内不能 include | 用全局模块片段 |
| `import json;` 找不到 | `C2230` | 没先构建 `.ixx` | 用 `build-project.ps1` |

## 练习

1. 给 `Value` 加 `serialize_pretty()`，带缩进输出。
2. 支持 `\uXXXX` 转义（提示：转成 UTF-8 字节序列）。
3. 加 `operator[]` 重载，让 `v["name"]` 直接取值。
4. 统计解析耗时：构造一个 100 KB 的 JSON，测 `parse` 的耗时。
5. 加一个 `parse_file(path)` 函数，读取文件后解析。
6. 把 `std::map` 换成 `std::vector<std::pair<...>>` 保留成员顺序，并支持 `find` 线性查找。

> [!success]- 参考答案
> 1.
> ```cpp
> std::string dump_pretty(const Value& v, int indent = 0) {
>     std::string pad(indent * 2, ' ');
>     if (v.kind != Value::Kind::Object) return dump(v);
>     std::string out = "{\n";
>     for (const auto& [k, val] : v.members)
>         out += std::format("{}\"{}\": {},\n", pad + "  ", k, dump_pretty(val, indent + 1));
>     return out + pad + "}";
> }
> ```
> 2. 解析 `\u` 后四位十六进制 → 码位 → 按 UTF-8 规则编码成 1～3 字节。
> 3.
> ```cpp
> const Value& operator[](std::string_view key) const {
>     static const Value empty{};
>     auto it = members.find(std::string{key});
>     return it == members.end() ? empty : it->second;
> }
> ```
> 4. 用 `steady_clock` 包住 `parse`，本机 100 KB 约 1～3 ms。
> 5.
> ```cpp
> std::expected<Value, std::string> parse_file(const std::filesystem::path& p) {
>     std::ifstream in(p);
>     if (!in) return std::unexpected(std::string("无法打开文件"));
>     std::string text((std::istreambuf_iterator<char>(in)), {});
>     return parse(text);
> }
> ```
> 6. `find` 改成 `std::ranges::find_if(members, [key](auto& p){ return p.first == key; })`，插入用 `push_back`。

---

上一节：[[41 实战：词频统计]] ｜ 下一节：[[43 实战：命令行工具]]
