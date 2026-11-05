---
title: 类与对象：构造、析构与 RAII
tags: [cpp, cpp26, 对象]
chapter: 3
order: 16
created: 2026-11-05
---

# 类与对象：构造、析构与 RAII

> [!info] 本节目标
> - 掌握成员初始化列表、`explicit`、委托构造、`= default` / `= delete`
> - 亲眼看到构造/析构的真实调用时机
> - 理解 RAII 与异常安全的三个等级
> - 会用 `const` 成员函数、`static` 成员、`mutable`
> - 知道"构造失败"应该怎么办

> [!tip] 先看结论
> - 成员初始化顺序**只由声明顺序决定**，与初始化列表书写顺序无关。
> - 参数只有一个的构造函数默认加 `explicit`，除非你确实要隐式转换。
> - 析构函数里**绝不能抛异常**（会导致 `std::terminate`）。
> - RAII = 构造函数获取资源、析构函数释放资源；这是现代 C++ 的核心。

## 一、构造函数与成员初始化列表

```cpp
struct Widget {
    int id;
    std::string name;
    explicit Widget(std::string n) : id(1), name(std::move(n)) {}
};
```

为什么必须用初始化列表：

| 情况 | 必须用初始化列表 |
| --- | --- |
| `const` 成员 | ✅ |
| 引用成员 | ✅ |
| 没有默认构造函数的成员 | ✅ |
| 基类构造 | ✅ |
| 其它情况 | 能用就用（避免"先默认构造再赋值"） |

> [!warning] 初始化顺序陷阱
> ```cpp
> struct Bad {
>     int a;
>     int b;
>     Bad(int v) : b(v), a(b + 1) {}   // a 先初始化！此时 b 还是未初始化值
> };
> ```
> 顺序永远是**声明顺序**（`a` 然后 `b`），`/W4` 会警告 `C5038`。

## 二、`explicit`：阻止隐式转换

```cpp
class Buffer {
public:
    explicit Buffer(std::size_t n) : data_(n, 0) {}
    std::size_t size() const { return data_.size(); }
private:
    std::vector<int> data_;
};

Buffer buf{4};        // 显式构造，OK
// Buffer bad = 4;    // 错误：explicit 阻止 int → Buffer
```

不加 `explicit` 会怎样：

```cpp
void f(Buffer b);
f(4);      // 悄悄构造一个 Buffer(4)，读代码的人看不出发生了转换
```

**规则：单参数构造函数默认加 `explicit`**，除非它是真的在做类型转换（如 `std::string` 接收 `const char*`）。

## 三、委托构造与 `= default` / `= delete`

```cpp
struct Widget {
    explicit Widget(std::string n) : name(std::move(n)) {}
    Widget() : Widget("anon") {}        // 委托构造：复用另一个构造函数
};
```

```cpp
struct Defaults {
    Defaults() = default;                        // 显式要编译器生成
    Defaults(const Defaults&) = delete;          // 禁止拷贝
    Defaults& operator=(const Defaults&) = delete;
};
```

`= delete` 的用途：禁止拷贝（如独占资源类）、禁止某些重载（如 `void f(int) = delete;` 挡住恼人的隐式转换）。

## 四、构造/析构的真实调用时机

`samples\16_class.cpp` 里每个特殊成员函数都打印自己的名字，实测输出：

```text
Widget(a) 构造
Widget(a) 拷贝
Widget(anon) 构造
Widget(a) 拷贝赋值
三个对象的 id: 1 2 3 created=3
Widget(anon) 析构
Widget(a) 析构
Widget(a) 析构
```

逐行对应代码：

| 输出 | 触发的代码 |
| --- | --- |
| `Widget(a) 构造` | `Widget a{"a"};` |
| `Widget(a) 拷贝` | `Widget b = a;`（拷贝构造） |
| `Widget(anon) 构造` | `Widget c;` → 委托构造到 `Widget("anon")` |
| `Widget(a) 拷贝赋值` | `b = a;`（拷贝赋值，不是构造） |
| 三次析构 | 离开作用域，**逆序** |

构造/析构的顺序规则（完整的）：

```text
构造：基类 → 成员（按声明顺序）→ 本类构造函数体
析构：本类析构函数体 → 成员（逆序）→ 基类
```

## 五、RAII：现代 C++ 的地基

RAII = Resource Acquisition Is Initialization：

```cpp
{
    std::ofstream f("out.txt");      // 构造：打开文件
    f << "hello";
}                                    // 析构：自动关闭文件（即使抛异常）
```

标准库里的 RAII 例子：

| 类型 | 管理什么 |
| --- | --- |
| `std::vector` | 堆内存 |
| `std::string` | 字符缓冲区 |
| `std::unique_ptr` | 堆对象 |
| `std::lock_guard` | 互斥锁 |
| `std::fstream` | 文件句柄 |

自己写 RAII 的模板：

```cpp
class FileHandle {
public:
    explicit FileHandle(const char* path) : f_(std::fopen(path, "r")) {
        if (!f_) throw std::runtime_error("open failed");
    }
    ~FileHandle() { if (f_) std::fclose(f_); }
    FileHandle(const FileHandle&) = delete;             // 不允许拷贝
    FileHandle& operator=(const FileHandle&) = delete;
    std::FILE* get() const { return f_; }
private:
    std::FILE* f_;
};
```

### 异常安全的三个等级

| 等级 | 保证 | 实现难度 |
| --- | --- | --- |
| 基本保证 | 不泄漏、不破坏不变量，对象仍可用 | 容易 |
| 强保证 | 失败时回滚到操作前状态（要么全成功，要么不变） | 中等（copy-and-swap） |
| 不抛保证 | `noexcept`，绝不失败 | 难（只用于简单操作） |

## 六、`const` 成员函数与 `mutable`

```cpp
struct Widget {
    int id;
    int get_id() const { return id; }      // 承诺不改成员
};
```

`mutable` 让某个成员在 `const` 函数里也能改（常见用途：缓存、计数器）：

```cpp
struct Cache {
    mutable int hits = 0;
    int get() const { ++hits; return 42; }   // const 函数里改 mutable 成员
};
```

## 七、`static` 成员

```cpp
struct Widget {
    static int created;                 // 声明
    explicit Widget() { ++created; }
};
int Widget::created = 0;                // 定义（C++17 起可写 inline static 省略）
```

C++17 起可以类内直接 `static inline int created = 0;`，不用再在 .cpp 里定义。

## 八、构造失败怎么办

构造函数**没有返回值**，失败只能：

| 方式 | 说明 |
| --- | --- |
| 抛异常 | 最常见；RAII 下资源不会泄漏 |
| 工厂函数 + `optional`/`expected` | 不抛异常时用（见 [[25 错误处理：异常、expected 与 error_code]]） |
| 两段式初始化（`init()`） | 老式做法，容易忘记检查，不推荐 |

```cpp
static std::optional<Widget> create(int id) {
    if (id <= 0) return std::nullopt;
    return Widget{id};
}
```

## 完整示例（已实测）

```powershell
.\build.ps1 samples\16_class.cpp
```

```text
Widget(a) 构造
Widget(a) 拷贝
Widget(anon) 构造
Widget(a) 拷贝赋值
三个对象的 id: 1 2 3 created=3
Widget(anon) 析构
Widget(a) 析构
Widget(a) 析构
buf.size=4
Widget(const) 构造
cw.get_id=4
Widget(const) 析构
```

要点：

- `created=3`：拷贝构造也自增了 `created`（代码里这么写的），说明**拷贝也是"新对象诞生"**。
- 三次析构顺序 `anon → a → a`，正是 LIFO。
- `cw.get_id=4`：`const Widget` 只能调 `const` 成员函数。

## 常见错误与排查

| 现象 | 报错 | 原因 | 改法 |
| --- | --- | --- | --- |
| 初始化顺序与预期不符 | `warning C5038`（`/W4`） | 按声明顺序初始化 | 调换成员声明顺序 |
| `const` 成员没在初始化列表赋值 | `C2789` | const 成员必须初始化 | 用初始化列表 |
| `Buffer b = 4;` | `C2440` | `explicit` 阻止隐式转换 | 显式构造 `Buffer{4}` |
| 析构函数抛异常导致终止 | `std::terminate` | 栈展开时二次异常 | 析构函数标 `noexcept`，内部吞掉错误 |
| `= delete` 后仍被调用 | `C2280` | 显式使用了被删函数 | 改调用方式 |
| 拷贝后两个对象共享资源导致 double free | 崩溃 | 浅拷贝 | 定义拷贝/移动，或禁用拷贝 |

## 练习

1. 写一个 `Timer` 类，构造时记录 `steady_clock::now()`，析构时打印耗时。
2. 写一个 `UniqueId` 类，禁止拷贝、允许移动，并验证 `std::vector` 里能放它。
3. 写一个带 `const` 成员的类，验证必须用初始化列表。
4. 写一个 RAII 的文件句柄类（`std::fopen`/`std::fclose`），并测试文件不存在时抛异常且不泄漏。
5. 用 `mutable` 实现一个"调用次数统计"的 `const` 成员函数。
6. 对比"抛异常"与"工厂 + `optional`"两种构造失败处理方式，各写一个例子。

> [!success]- 参考答案
> 1.
> ```cpp
> struct Timer {
>     std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
>     ~Timer() {
>         auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
>         std::println("elapsed {:.3f} ms", ms);
>     }
> };
> ```
> 2.
> ```cpp
> struct UniqueId {
>     int id;
>     explicit UniqueId(int i) : id(i) {}
>     UniqueId(const UniqueId&) = delete;
>     UniqueId& operator=(const UniqueId&) = delete;
>     UniqueId(UniqueId&&) = default;
>     UniqueId& operator=(UniqueId&&) = default;
> };
> std::vector<UniqueId> v;
> v.push_back(UniqueId{1});      // 移动，OK
> ```
> 3.
> ```cpp
> struct S { const int k; explicit S(int v) : k(v) {} };
> // S bad{1}; S bad2 = bad;   // 拷贝被隐式删除（const 成员）
> ```
> 4.
> ```cpp
> class FileHandle {
> public:
>     explicit FileHandle(const std::string& path) : f_(std::fopen(path.c_str(), "r")) {
>         if (!f_) throw std::runtime_error("open failed");
>     }
>     ~FileHandle() { if (f_) std::fclose(f_); }
>     FileHandle(const FileHandle&) = delete;
>     FileHandle& operator=(const FileHandle&) = delete;
>     std::FILE* get() const { return f_; }
> private:
>     std::FILE* f_;
> };
> ```
> 5.
> ```cpp
> struct Counter { mutable int n = 0; int get() const { return ++n; } };
> ```
> 6. 抛异常版：`Widget(int id) { if (id <= 0) throw std::invalid_argument("bad id"); }`；工厂版：`static std::optional<Widget> create(int id)`，失败返回 `nullopt`。前者适合"失败是异常情况"，后者适合"失败是常规分支"。

---

上一节：[[15 算法与 views 进阶]] ｜ 下一节：[[17 拷贝、移动与零法则]]
