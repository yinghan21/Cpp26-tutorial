---
title: 文件、路径与 filesystem
tags: [cpp, cpp26, 标准库]
chapter: 4
order: 27
created: 2026-11-05
---

# 文件、路径与 `filesystem`

> [!info] 本节目标
> - 掌握 `std::filesystem::path` 的拼接与分解
> - 会读写文本/二进制文件，并且**检查失败**
> - 会遍历目录（单层与递归）
> - 分清"抛异常版"与"`error_code` 版"两套 API
> - 知道临时文件、目录创建/删除的正确姿势

> [!tip] 先看结论
> - 路径用 `operator/` 拼接，跨平台自动处理分隔符。
> - 文件读写用 `std::ofstream` / `std::ifstream`（RAII 自动关闭），别用 C 的 `fopen`。
> - `filesystem` 每个操作几乎都有两个版本：抛异常 / 收 `error_code`。
> - 不存在的东西不要直接 `file_size()`（会抛异常），先 `exists()` 或用 `error_code` 版本。

## 一、`path`：路径就是字符串 + 规则

```cpp
namespace fs = std::filesystem;

fs::path p = "dir/sub/file.txt";
p.filename();       // file.txt
p.stem();           // file
p.extension();      // .txt
p.parent_path();    // dir/sub
```

实测：

```text
filename=file.txt stem=file ext=.txt parent=dir/sub
拼接: a\b\c.txt
```

拼接用 `/`（Windows 上会输出 `\`）：

```cpp
fs::path full = fs::path("a") / "b" / "c.txt";
```

| 操作 | 说明 |
| --- | --- |
| `p / "x"` | 追加（自动加分隔符） |
| `p /= "x"` | 原地追加 |
| `p += ".bak"` | 字符串级追加（不加分隔符） |
| `p.string()` / `p.wstring()` | 转字符串 |
| `p.is_absolute()` | 是否绝对路径 |

## 二、读写文本文件

```cpp
{
    std::ofstream out(work / "demo.txt");
    out << "line1\nline2\nline3\n";
}   // 析构自动关闭并 flush

{
    std::ifstream in(work / "demo.txt");
    std::string line;
    while (std::getline(in, line)) {
        std::println("{}", line);
    }
}
```

实测：

```text
读到[1]: line1
读到[2]: line2
读到[3]: line3
exists=true size=21
```

要点：

| 事 | 说明 |
| --- | --- |
| 打开失败不抛异常 | `ofstream` 默认只设状态位，要检查 `if (!out)` |
| 写入后崩溃 | 不要忘了 `flush` 或让析构处理 |
| 二进制模式 | `std::ios::binary` |
| 追加模式 | `std::ios::app` |

**检查失败的模板：**

```cpp
std::ofstream out(path);
if (!out) { /* 处理错误：目录不存在/无权限 */ }
```

## 三、两套 API：抛异常 vs `error_code`

| 版本 | 失败时 | 适用 |
| --- | --- | --- |
| `fs::file_size(p)` | 抛 `filesystem_error` | 失败就是异常情况 |
| `fs::file_size(p, ec)` | 设 `ec`，返回值无意义 | 失败是常规分支 |

实测：

```text
读不存在文件: size=18446744073709551615 ec=2 msg=The system cannot find the file specified.
filesystem_error: file_size: The system cannot find the file specified.: "...\missing.txt"
```

注意 `18446744073709551615`（`-1` 转无符号）——**失败时返回值不可用**，必须看 `ec`。

## 四、目录遍历

```cpp
for (const auto& e : fs::directory_iterator(work)) {
    std::println("  {} ({} bytes)", e.path().filename().string(),
                 fs::is_regular_file(e) ? fs::file_size(e) : 0);
}
```

| 迭代器 | 范围 |
| --- | --- |
| `directory_iterator` | 只当前层 |
| `recursive_directory_iterator` | 递归所有子目录 |

递归时跳过某些目录：

```cpp
for (auto it = fs::recursive_directory_iterator(root);
     it != fs::recursive_directory_iterator(); ++it) {
    if (it->is_directory() && it->path().filename() == ".git") {
        it.disable_recursion_pending();     // 不进入 .git
    }
}
```

## 五、创建、移动、删除

```cpp
fs::create_directories(work);        // 递归创建（父目录不存在也行）
fs::create_directory(one_level);     // 只建一层
fs::copy(from, to);                  // 复制文件
fs::copy(from, to, fs::copy_options::recursive);   // 复制目录
fs::rename(a, b);                    // 移动/改名
fs::remove(p);                       // 删文件或空目录
fs::remove_all(p);                   // 递归删除
```

实测：`清理完成, 目录还在? false`

> [!warning] `remove_all` 不可撤销
> 代码里用之前**一定要打印或断言目标路径**。本教程所有示例只在 `temp_directory_path()` 下操作。

## 六、临时文件与临时目录

```cpp
fs::path dir = fs::temp_directory_path();      // 系统临时目录
fs::path work = dir / "cpp26_files_demo";
fs::create_directories(work);
// ... 用完
fs::remove_all(work);
```

标准库没有"创建唯一临时文件"的 API（`std::tmpfile` 是 C 的，返回 `FILE*`）。常用做法是自己拼一个带随机数的文件名，或用平台 API。

## 七、常见任务的正确写法

| 任务 | 写法 |
| --- | --- |
| 读整个文件到字符串 | `std::ifstream in(p, std::ios::binary); std::string s((std::istreambuf_iterator<char>(in)), {});` |
| 写整个字符串 | `std::ofstream out(p, std::ios::binary); out.write(s.data(), s.size());` |
| 文件是否存在 | `fs::exists(p)` |
| 目录是否存在 | `fs::is_directory(p)` |
| 文件大小 | `fs::file_size(p, ec)` |
| 最后修改时间 | `fs::last_write_time(p)` |
| 当前工作目录 | `fs::current_path()` |
| 可执行文件所在目录 | 用 `argv[0]` 推（无标准 API） |

## 完整示例（已实测）

```powershell
.\build.ps1 samples\27_files.cpp
```

```text
filename=file.txt stem=file ext=.txt parent=dir/sub
拼接: a\b\c.txt
读到[1]: line1
读到[2]: line2
读到[3]: line3
exists=true size=21
  demo.txt (21 bytes)
读不存在文件: size=18446744073709551615 ec=2 msg=The system cannot find the file specified.
filesystem_error: file_size: The system cannot find the file specified.: "C:\Users\Admin\AppData\Local\Temp\cpp26_files_demo\missing.txt"
清理完成, 目录还在? false
```

## 常见错误与排查

| 现象 | 报错/后果 | 原因 | 改法 |
| --- | --- | --- | --- |
| `file_size` 抛异常 | `filesystem_error` | 文件不存在 | 先 `exists` 或用 `ec` 版 |
| 打开文件失败但程序继续 | 无（逻辑错） | 没检查 `ifstream`/`ofstream` | `if (!in) { ... }` |
| 中文路径乱码 | 无 | 编码/`/utf-8` 缺失 | 存 UTF-8，`path` 用 `u8path` 语义 |
| 目录遍历抛异常 | `filesystem_error` | 权限不足 | 用带 `ec` 的迭代器构造 |
| 写文件后内容为空 | 无 | 未 flush / 提前崩溃 | 让流析构，或显式 `flush()` |
| `remove_all` 删错目录 | 数据丢失 | 路径拼错 | 删前 `println` 目标路径 |

## 练习

1. 写函数 `count_lines(path)` 返回文件行数（用 `std::getline`）。
2. 递归统计某个目录下所有 `.cpp` 文件的总行数。
3. 把一个大文件按 1 MB 分块复制（`ifstream`/`ofstream` + `read`/`write`）。
4. 用 `error_code` 版本实现"目录不存在就创建"，不抛异常。
5. 写函数列出目录中最大的 5 个文件。
6. 用 `last_write_time` 找出最近修改过的文件。

> [!success]- 参考答案
> 1.
> ```cpp
> std::size_t count_lines(const std::filesystem::path& p) {
>     std::ifstream in(p);
>     std::size_t n = 0; std::string line;
>     while (std::getline(in, line)) ++n;
>     return n;
> }
> ```
> 2.
> ```cpp
> std::size_t total = 0;
> for (auto& e : std::filesystem::recursive_directory_iterator(root)) {
>     if (e.is_regular_file() && e.path().extension() == ".cpp") total += count_lines(e.path());
> }
> ```
> 3.
> ```cpp
> std::ifstream in(src, std::ios::binary);
> std::ofstream out(dst, std::ios::binary);
> std::vector<char> buf(1 << 20);
> while (in.read(buf.data(), buf.size()) || in.gcount() > 0) {
>     out.write(buf.data(), in.gcount());
> }
> ```
> 4.
> ```cpp
> std::error_code ec;
> std::filesystem::create_directories(p, ec);
> if (ec) std::println("创建失败: {}", ec.message());
> ```
> 5.
> ```cpp
> std::vector<std::pair<std::uintmax_t, std::filesystem::path>> items;
> for (auto& e : std::filesystem::directory_iterator(dir))
>     if (e.is_regular_file()) items.emplace_back(e.file_size(), e.path());
> std::ranges::sort(items, std::greater<>{});
> for (auto& [sz, path] : items | std::views::take(5)) std::println("{} {}", sz, path.string());
> ```
> 6.
> ```cpp
> auto newest = std::ranges::max_element(items, {}, [](auto& e) { return std::filesystem::last_write_time(e.path()); });
> ```

---

上一节：[[26 格式化与输出]] ｜ 下一节：[[28 流与序列化]]
