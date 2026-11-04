---
title: 第一个程序：import std
tags: [cpp, cpp26, 基础]
chapter: 1
order: 1
created: 2026-11-05
---

# 第一个程序：import std

> [!info] 本节目标
> - 跑通第一个程序，知道 `import std;` 是什么
> - 会用 `std::println` 输出
> - 知道注释、`main` 函数的作用

## 一、最小程序

```cpp
import std;

int main() {
    std::println("你好，C++26！");
    std::println("1 + 2 = {}", 1 + 2);
}
```

```powershell
.\build.ps1 samples\01_hello.cpp
```

实测输出：

```text
你好，C++26！
1 + 2 = 3
```

## 二、逐行解释

| 代码                  | 含义                                 |
| ------------------- | ---------------------------------- |
| `import std;`       | 导入整个标准库（相当于一次性 `#include` 了几百个头文件） |
| `int main()`        | 程序入口，操作系统从这里开始执行                   |
| `std::println(...)` | 打印一行并换行；`std` 是标准库的命名空间            |
| `{}`                | 占位符，按顺序填后面的参数（不用记 `%d`/`%s`）       |
| `return 0;`         | 返回 0 表示成功（可以省略，`main` 会自动返回 0）     |

> [!tip] 为什么 `import std;` 比 `#include` 好
> 1. 只写一行，不用记 `<vector>` 还是 `<string>`
> 2. 编译更快（模块只编译一次），不会因为头文件顺序不同出玄学错误
> 3. 宏污染更少

## 三、输出：`println` 和 `print`

```cpp
import std;

int main() {
    std::print("不换行");
    std::print(" 继续");
    std::println("");            // 换行
    std::println("hp={} name={}", 100, "玩家");
    std::println("{:.2f}", 3.14159);      // 保留两位
    std::println("{:>6}|", 42);           // 右对齐宽度 6
}
```

```text
不换行 继续
hp=100 name=玩家
3.14
    42|
```

格式说明符（`{}` 里冒号后面的部分）：

| 写法 | 效果 |
| --- | --- |
| `{}` | 默认 |
| `{:.2f}` | 两位小数 |
| `{:>6}` / `{:^6}` | 右/居中对齐，宽度 6 |
| `{:x}` | 十六进制 |
| `{:08.3f}` | 补零、总宽 8、三位小数 |

> [!warning] 中文别写进格式串
> 本机实测：`std::println("结果是 {}", 1);` 会**没有任何输出**（`import std` + MSVC 的坑）。
> 把中文当**参数**传：`std::println("{}", "结果是 1");` 或 `std::println("result={}", 1);`

## 四、注释

```cpp
// 单行注释

/*
   多行注释
*/

/// 文档注释（Doxygen 风格，工具能读）
```

## 五、编译到底发生了什么

```text
源码 main.cpp
   │  预处理/导入模块
   ▼
编译成 .obj（机器码 + 符号表）
   │  链接（把 std 模块、运行库拼进来）
   ▼
main.exe → 操作系统加载 → 从 main() 开始跑
```

`build.ps1` 就是替你敲这两步：

```text
[setup] 生成 std.ifc（一次）
[build] cl ... /c main.cpp → link main.obj std.obj → main.exe
```

## 练习

1. 打印你的名字和三行任意文字。
2. 打印 `1/3` 的结果，分别保留 2 位和 6 位小数。
3. 故意删掉一行末尾的分号，看报错长什么样。

> [!success]- 参考答案
> 1.
> ```cpp
> import std;
> int main() {
>     std::println("YG");
>     std::println("line 1");
>     std::println("line 2");
> }
> ```
> 2.
> ```cpp
> std::println("{:.2f} {:.6f}", 1.0 / 3, 1.0 / 3);
> ```
> 3. 会看到 `error C2143: 语法错误: 缺少“;”`，位置通常指向**下一行**——这是 C++ 报错最常见的坑。

---

下一节：[[02 变量、类型与常量]]