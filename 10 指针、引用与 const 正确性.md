---
title: 指针、引用与 const 正确性
tags: [cpp, cpp26, 数据]
chapter: 2
order: 10
created: 2026-11-05
---

# 指针、引用与 const 正确性

> [!info] 本节目标
> - 掌握指针的本质、指针算术，以及它和数组的真实关系
> - 分清引用与指针的适用场景
> - 彻底搞懂 `const` 的四种位置与顶层/底层 const
> - 知道 `const_cast` 唯一正当的用途
> - 认得出悬垂指针与 use-after-free

> [!tip] 先看结论
> - 指针难的不是语法，是**所有权不清楚**。指针本身不知道对象死没死。
> - 函数参数：想改调用方就用 `T&`，想表达"可能为空"才用 `T*`。
> - `const` 位置的记忆法：**`const` 修饰它左边的东西；左边没东西就修饰右边**。
> - `const_cast` 只在"对象本身不是 const，但要传给老接口"时才是合法的。

## 一、指针是什么

指针就是一个整数（存着地址），`sizeof(int*) == 8`。

```cpp
int a = 1;
int* p = &a;        // p 存 a 的地址
*p = 2;             // 解引用：改 a
std::println("{}", a);   // 2
```

| 表达式 | 含义 |
| --- | --- |
| `&a` | 取地址 |
| `*p` | 解引用（读/写指向的对象） |
| `p + 1` | 前进**一个元素**（不是 1 字节） |
| `p1 - p2` | 两个同数组指针相减 = 元素个数 |
| `nullptr` | 空指针常量 |

## 二、指针算术与数组的真实关系

```cpp
int arr[5]{10, 20, 30, 40, 50};
int* p = arr;
int* q = p + 3;
std::println("*p={} *(p+3)={} q-p={} 差字节={}", *p, *(p + 3), q - p,
             reinterpret_cast<char*>(q) - reinterpret_cast<char*>(p));
```

实测：

```text
*p=10 *(p+3)=40 q-p=3 差字节=12
```

关键：**`p + 1` 前进 `sizeof(int) == 4` 字节**，指针算术按元素类型缩放。所以 `a[i]` 完全等价于 `*(a + i)`，甚至可以写 `2[begin]`（合法但别这么写）。

`q - p == 3` 而不是 12：指针相减得到元素个数。

## 三、引用 vs 指针

```cpp
int v = 5;
int& rv = v;      // 引用：v 的别名
rv = 6;           // 改的是 v
std::println("{}", &v == &rv);   // true：没有独立地址
```

| 维度 | 引用 `T&` | 指针 `T*` |
| --- | --- | --- |
| 是否必须初始化 | 是 | 否 |
| 能否改绑 | 否 | 能 |
| 能否为空 | 否 | 能（`nullptr`） |
| `sizeof` | 被引用对象的大小 | 指针大小（8） |
| 算术 | 不支持 | 支持 |
| 常见用途 | 函数参数、返回值、别名 | 可选参数、链表、底层内存 |

实践规则：

1. **函数要修改调用方**：用 `T&`，不要用 `T*`。
2. **参数"可能没有"**：用 `T*`（或 `std::optional<T>`，更安全）。
3. **返回大对象**：返回值（有 RVO），不要返回引用或指针指向局部变量。
4. **遍历序列**：用迭代器/range，不要手写指针。

## 四、`const` 的四种位置

```cpp
int m = 1;
const int* p1 = &m;        // 指向 const int：不能通过 p1 改 m
int* const p2 = &m;        // const 指针：不能指向别处，但能改 m
const int* const p3 = &m;  // 两者都不能
*p2 = 2;                   // 合法
// *p1 = 3;                // 编译错误
// p2 = &other;            // 编译错误
```

口诀：**`const` 修饰它左边的东西；左边没东西就修饰右边**。

- `const int* p`：`const` 左边没东西 → 修饰 `int`，即"指向 const int"。
- `int* const p`：修饰指针本身。
- `const int* const p`：两个都修饰。

## 五、顶层 const 与底层 const

| 类型 | 谁被 const 修饰 | 名称 |
| --- | --- | --- |
| `int* const` | 指针本身 | 顶层 const（top-level） |
| `const int*` | 被指对象 | 底层 const（low-level） |
| `const int* const` | 两者 | 都有 |

一句话结论：**顶层 const 在模板推导、函数重载里会被丢掉**。

```cpp
void f(int*);            // 与 void f(int* const) 是同一个函数
// void f(int* const);   // 重复定义
```

## 六、`const` 成员函数与 const 正确性

```cpp
struct S {
    int x = 1;
    int get() const { return x; }     // 承诺不改成员
};
const S s{};
std::println("{}", s.get());          // 只能调 const 成员函数
```

`const` 一旦加上，整条调用链只能读——这叫 **const 正确性**。它的价值：把"这个函数会不会改我的数据"变成**编译期可验证**的事实，而不是注释里的承诺。

> [!tip] 成员函数默认都该是 `const`
> 只有确实要修改对象状态时才去掉 `const`。这一条能挡住大量隐性 bug。

## 七、`const_cast`：唯一正当的用途

```cpp
void legacy_api(char* buf);       // 老 C 接口，其实不写 buf

void wrapper(const std::string& s) {
    legacy_api(const_cast<char*>(s.c_str()));   // 对象本身不是 const ? 
}
```

合法性判据：

| 情况 | 是否合法 |
| --- | --- |
| 对象本身**不是** const，只是通过 const 引用/指针看到 | 合法 |
| 对象本身**是** const（如 `const std::string`） | **未定义行为** |
| 去掉 const 后还写入了它 | 极可能 UB |

## 八、悬垂指针与 use-after-free

```cpp
int* stale = nullptr;
{
    int local = 42;
    stale = &local;
}   // local 生命周期结束
std::println("{}", *stale);    // UB：可能打印 42，也可能崩
```

指针不知道对象死没死。真正的解法是**所有权模型**：`std::unique_ptr` / `std::shared_ptr` / 容器（见 [[20 智能指针与所有权]]）。

## 完整示例（已实测）

`samples\10_pointer_ref.cpp`（节选核心）：

```cpp
import std;

void add_by_value(int x) { x += 1; }
void add_by_ref(int& x) { x += 1; }
void add_by_ptr(int* x) { if (x) *x += 1; }

int main() {
    int a = 1;
    add_by_value(a); std::println("after value: {}", a);   // 1
    add_by_ref(a);   std::println("after ref:   {}", a);   // 2
    add_by_ptr(&a);  std::println("after ptr:   {}", a);   // 3

    int arr[5]{10, 20, 30, 40, 50};
    int* p = arr;
    int* q = p + 3;
    std::println("*p={} *(p+3)={} q-p={} 差字节={}", *p, *(p + 3), q - p,
                 reinterpret_cast<char*>(q) - reinterpret_cast<char*>(p));

    int* np = nullptr;
    std::println("np == nullptr ? {}", np == nullptr);
    std::println("地址: {:#x}", reinterpret_cast<std::uintptr_t>(arr));

    int v = 5;
    int& rv = v;
    rv = 6;
    std::println("v={} rv={} 同一对象={}", v, rv, &v == &rv);
}
```

```powershell
.\build.ps1 samples\10_pointer_ref.cpp
```

实测输出：

```text
after value: 1
after ref:   2
after ptr:   3
*p=10 *(p+3)=40 q-p=3 差字节=12
10 20 30 40 50
np == nullptr ? true
地址（转成整数打印）: 0x89248ff9c8
v=6 rv=6 同一对象=true
m=2 p1->2
const S get=1
```

注意 `std::println("{}", 指针)` **编译不过**（`C2338: Cannot format an argument`），所以打印地址要转成整数：`reinterpret_cast<std::uintptr_t>(p)`。

## 常见错误与排查

| 现象 | 报错 | 原因 | 改法 |
| --- | --- | --- | --- |
| 传 `int*` 却想改调用方 | 无（逻辑错） | 改的是副本指针 | 用 `int&` 或 `int*&` |
| `const int*` 赋值给 `int*` | `C2440` | 丢掉底层 const | 加 `const` 或用 `const_cast`（慎） |
| `println("{}", p)` | `C2338` | 标准不支持格式化指针 | 转 `std::uintptr_t` |
| 悬垂指针读到旧值 | 无（UB） | 对象已销毁 | 用容器/智能指针 |
| `const_cast` 后写入 | 无（UB） | 原对象是 const | 改设计，别硬转 |
| 指针越界 | `0xC0000005` | 无边界检查 | 用 `span`/`at()` |

## 练习

1. 写三个函数（值、引用、指针参数）分别把参数乘 2，验证哪些能影响调用方。
2. 用指针算术遍历一个 `int[6]` 数组并求和。
3. 写一个 `const` 成员函数和一个非 `const` 成员函数，验证 `const` 对象只能调前者。
4. 解释 `const int* p` 与 `int* const p` 的区别，各写一行代码证明。
5. 下面代码为什么危险？
   ```cpp
   int* f() { int x = 1; return &x; }
   ```
6. 用 `reinterpret_cast<std::uintptr_t>` 打印同一个对象的地址，再用 `&` 打印一次，比较两者。

> [!success]- 参考答案
> 1.
> ```cpp
> import std;
> void by_value(int x) { x *= 2; std::println("value: {}", x); }
> void by_ref(int& x) { x *= 2; }
> void by_ptr(int* x) { *x *= 2; }
> int main() { int a = 3; by_value(a); std::println("a={}", a); by_ref(a); std::println("a={}", a); by_ptr(&a); std::println("a={}", a); }
> ```
> 值传递不影响调用方；引用和指针都影响。
> 2.
> ```cpp
> int arr[6]{1,2,3,4,5,6};
> int sum = 0;
> for (int* p = arr; p != arr + 6; ++p) sum += *p;
> std::println("{}", sum);   // 21
> ```
> 3.
> ```cpp
> struct S { int x = 1; int get() const { return x; } void set(int v) { x = v; } };
> const S cs{};
> std::println("{}", cs.get());
> // cs.set(2);   // error C2662：不能把 const S 转成 S&
> ```
> 4. `const int* p`：p 能改指向，不能改所值；`int* const p`：p 不能改指向，能改所值。
> ```cpp
> int a = 1, b = 2;
> const int* p1 = &a; p1 = &b;      // 行
> // *p1 = 9;                        // 不行
> int* const p2 = &a; *p2 = 9;      // 行
> // p2 = &b;                        // 不行
> ```
> 5. 返回局部变量的地址：函数返回后 `x` 已销毁，指针悬垂，任何解引用都是 UB。修法：返回值，或返回 `std::unique_ptr<int>`。
> 6. 两者输出相同（都是同一地址的整数值），区别只是类型：`reinterpret_cast` 得到 `std::uintptr_t` 能打印，`&` 得到指针不能直接格式化。

---

上一节：[[09 数组、array 与 span]] ｜ 下一节：[[11 容器与字符串]]
