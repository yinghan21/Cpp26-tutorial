#pragma once
#include <string>
#include <vector>

// 头文件里只放：类型声明、内联函数、constexpr、inline 变量
inline constexpr double kPi = 3.141592653589793;        // C++17：inline 变量

struct Circle {
    double r;
    double area() const;                                  // 声明，定义在 .cpp
};

// 内联小函数：可以放头文件
inline double perimeter(const Circle& c) { return 2 * kPi * c.r; }

// 模板必须放头文件
template <class T>
T sum_all(const std::vector<T>& v) {
    T s{};
    for (const auto& x : v) s += x;
    return s;
}

std::string describe(const Circle& c);                    // 普通函数声明
