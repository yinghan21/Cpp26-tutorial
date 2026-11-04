---
title: 模板与 concept
tags: [cpp, cpp26, 基础]
chapter: 3
order: 8
created: 2026-11-05
---

# 模板与 concept

> [!info] 本节目标
> - 函数模板、类模板：一份代码多种类型
> - `concept`（C++20）：给模板加约束，报错更好懂
> - 什么时候**不该**用模板

## 一、函数模板

想写“两数取大”，每种类型写一遍太蠢：

```cpp
template <class T>
T my_max(T a, T b) { return a < b ? b : a; }

my_max(3, 7);        // T = int
my_max(1.5, 0.5);    // T = double
```

编译器为你用到的每种类型生成一份代码（实例化），运行时零开销。

## 二、类模板

```cpp
template <class T>
class Box {
public:
    void put(T v) { v_ = std::move(v); }
    const T& get() const { return v_; }
private:
    T v_{};
};

Box<int> a; a.put(42);
Box<std::string> b; b.put("hello");
```

`std::vector<int>`、`std::map<K,V>`、`std::optional<T>` 都是类模板。

## 三、concept：约束

没有约束时，错误会炸在模板内部，报错几十行；有 concept 后直接告诉你“不满足”：

```cpp
template <class T>
concept Number = std::is_arithmetic_v<T>;

template <Number T>
T twice(T v) { return v + v; }

twice(21);        // OK
twice("abc"s);    // ❌ 报错：'std::string' does not satisfy 'Number'
```

常用现成 concept（`import std;` 就有）：`std::integral`、`std::floating_point`、`std::same_as`、`std::convertible_to`、`std::ranges::range`。

## 四、完整示例（已实测）

```cpp
import std;

template <class T>
T my_max(T a, T b) { return a < b ? b : a; }

template <class T>
concept Number = std::is_arithmetic_v<T>;

template <Number T>
T twice(T v) { return v + v; }

template <class T>
class Box {
public:
    void put(T v) { v_ = std::move(v); }
    const T& get() const { return v_; }
private:
    T v_{};
};

int main() {
    std::println("max int={} max double={}", my_max(3, 7), my_max(1.5, 0.5));
    std::println("twice int={} twice double={}", twice(21), twice(1.25));

    Box<int> a;
    a.put(42);
    Box<std::string> b;
    b.put("hello");
    std::println("box a={} box b={}", a.get(), b.get());
}
```

实测输出：

```text
max int=7 max double=1.5
twice int=42 twice double=2.5
box a=42 box b=hello
```

## 五、什么时候别用模板

| 场景 | 建议 |
| --- | --- |
| 只有两三种类型 | 直接写重载，代码更好读 |
| 需要把实现藏起来（.cpp 里） | 模板必须放头文件，用虚函数或类型擦除 |
| 报错看不懂的新手期 | 先用具体类型写，能跑再抽象 |
| 编译时间敏感 | 模板实例化很吃编译时间 |

## 六、常见坑

| 坑 | 说明 |
| --- | --- |
| 模板定义放在 `.cpp` | 别的文件实例化时看不到定义 → 链接错误；放头文件 |
| `template <class T> T max(T,T)` 传 `int` 和 `double` | 推导冲突；显式写 `max<double>(1, 2.5)` |
| concept 写得太窄 | 后来想传别的类型就被挡住；先写宽再收紧 |

## 练习

1. 写 `template <class T> T sum(const std::vector<T>&)`，对 `int`、`double` 都能用。
2. 给上面的函数加 concept，只允许算术类型。
3. 写类模板 `Pair<A,B>`，有 `first()` / `second()`。

> [!success]- 参考答案
> 1.
> ```cpp
> template <class T>
> T sum(const std::vector<T>& v) {
>     T s{};
>     for (const auto& x : v) s += x;
>     return s;
> }
> ```
> 2.
> ```cpp
> template <class T>
> concept Addable = requires(T a, T b) { a + b; };
>
> template <Addable T>
> T sum(const std::vector<T>& v) { T s{}; for (auto& x : v) s += x; return s; }
> ```
> 3.
> ```cpp
> template <class A, class B>
> class Pair {
> public:
>     Pair(A a, B b) : a_(std::move(a)), b_(std::move(b)) {}
>     const A& first() const { return a_; }
>     const B& second() const { return b_; }
> private:
>     A a_; B b_;
> };
> ```

---

上一节：[[07 智能指针与所有权]] ｜ 下一节：[[09 算法与 ranges]]