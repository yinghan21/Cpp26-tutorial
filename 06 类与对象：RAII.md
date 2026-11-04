---
title: 类与对象：RAII
tags: [cpp, cpp26, 基础]
chapter: 2
order: 6
created: 2026-11-05
---

# 类与对象：RAII

> [!info] 本节目标
> - 会写 `class`：成员、构造、析构、`const` 成员函数
> - 理解 RAII：资源跟着对象生命周期走
> - 知道对象什么时候被构造、什么时候被销毁

## 一、从结构体到类

```cpp
import std;

class Player {
public:
    Player(std::string name, int hp) : name_(std::move(name)), hp_(hp) {
        std::println("[构造] {}", name_);
    }
    ~Player() { std::println("[析构] {}", name_); }

    void damage(int d) { hp_ -= d; }
    bool alive() const { return hp_ > 0; }
    const std::string& name() const { return name_; }
    int hp() const { return hp_; }

private:
    std::string name_;
    int hp_;
};
```

| 关键字 | 含义 |
| --- | --- |
| `public` / `private` | 谁能访问（`class` 默认 `private`） |
| `Player(...) : name_(...)` | 构造函数 + 成员初始化列表 |
| `~Player()` | 析构函数，对象销毁时自动调用 |
| `const` 成员函数 | 承诺不改成员，`const` 对象只能调这些 |

## 二、构造/析构什么时候跑

```cpp
{
    Player p{"一号", 100};      // 构造
    p.damage(30);
}                               // 离开作用域 → 析构
```

实测输出：

```text
[构造] 一号
一号 hp=70 alive=true
[析构] 一号
```

> [!tip] 这就是 RAII
> 构造函数拿资源，析构函数放资源。对象在栈上 → 不可能忘记释放，
> 提前 `return`、抛异常也一样会析构。标准库的 `std::string`、`std::vector`、
> `std::ofstream`、`std::lock_guard` 全是这个思路。

## 三、`const` 成员函数

```cpp
class Counter {
public:
    void bump() { ++n_; }             // 非 const：能改
    int get() const { return n_; }    // const：只读
private:
    int n_ = 0;
};

const Counter c{};
c.get();      // ✅
c.bump();     // ❌ 编译错误：const 对象不能调非 const 成员
```

养成习惯：**不修改成员的函数都加 `const`**。这样 `const` 对象、`const&` 参数都能调。

## 四、构造函数的几种写法

```cpp
class A {
public:
    A() = default;                       // 让编译器生成默认构造
    explicit A(int x) : x_(x) {}         // explicit：禁止 int 隐式转 A
    A(const A&) = delete;                // 禁止拷贝
    A(A&&) noexcept = default;           // 允许移动（默认实现）
private:
    int x_ = 0;
};
```

`explicit` 的作用：

```cpp
void f(A a);
f(42);          // ❌ 被 explicit 拦住（否则会把 42 偷偷变成 A）
f(A{42});       // ✅
```

## 五、成员初始化顺序

成员**按声明顺序**初始化，不是按初始化列表顺序：

```cpp
class Bad {
    int a_;
    int b_;
public:
    Bad(int v) : b_(v), a_(b_ * 2) {}   // ❌ a_ 先初始化，此时 b_ 还没值
};
```

把初始化列表顺序写成和声明一致，别玩花活。

## 六、完整示例（已实测，含 `new` 的反面教材）

```cpp
import std;

class Player {
public:
    Player(std::string name, int hp) : name_(std::move(name)), hp_(hp) {
        std::println("[构造] {}", name_);
    }
    ~Player() { std::println("[析构] {}", name_); }

    void damage(int d) { hp_ -= d; }
    bool alive() const { return hp_ > 0; }
    const std::string& name() const { return name_; }
    int hp() const { return hp_; }

private:
    std::string name_;
    int hp_;
};

int main() {
    {
        Player p{"一号", 100};
        p.damage(30);
        std::println("{} hp={} alive={}", p.name(), p.hp(), p.alive());
    }

    auto* heap = new Player{"堆上的", 50};   // 不推荐，仅演示
    delete heap;                            // 必须手动释放 → 所以用智能指针
}
```

实测输出：

```text
[构造] 一号
一号 hp=70 alive=true
[析构] 一号
[构造] 堆上的
[析构] 堆上的
```

## 七、常见坑

| 坑 | 说明 |
| --- | --- |
| `new` 了忘记 `delete` | 泄漏；用 [[07 智能指针与所有权]] |
| 拷贝一个持有裸指针的对象 | double free；删拷贝或换智能指针 |
| 析构函数里抛异常 | 会导致 `std::terminate` |
| 构造函数里调虚函数 | 此时对象还没构造成派生类 |
| 成员初始化顺序写反 | 用未初始化的值，结果是垃圾 |

## 练习

1. 写 `class Counter`，有 `bump()` 和 `get() const`，在 `main` 里加到 3 并打印。
2. 给 `Player` 加一个 `heal(int)`，把血量加回去但不超过 100。
3. 写一个 `class File`：构造时 `std::ofstream` 打开文件，析构时自动关闭（用成员 `std::ofstream` 就自动了，不需要手写析构）。

> [!success]- 参考答案
> 1.
> ```cpp
> class Counter {
> public:
>     void bump() { ++n_; }
>     int get() const { return n_; }
> private:
>     int n_ = 0;
> };
> ```
> 2.
> ```cpp
> void heal(int amount) { hp_ += amount; if (hp_ > 100) hp_ = 100; }
> ```
> 3.
> ```cpp
> class File {
> public:
>     explicit File(const std::string& path) : out_(path) {}
>     void write(std::string_view s) { out_ << s; }
> private:
>     std::ofstream out_;   // 成员析构 → 文件自动关闭
> };
> ```

---

上一节：[[05 容器与字符串]] ｜ 下一节：[[07 智能指针与所有权]]