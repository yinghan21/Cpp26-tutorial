---
title: 时间与日期：chrono
tags: [cpp, cpp26, 标准库, 时间]
chapter: 4
order: 29
created: 2026-11-05
---

# 时间与日期：`chrono`

> [!info] 本节目标
> - 掌握 `duration` 的单位与换算
> - 分清 `system_clock` 与 `steady_clock`，知道测性能该用哪个
> - 会用 `{:%Y-%m-%d %H:%M:%S}` 格式化时间点
> - 会用时区库（`current_zone` / `zoned_time`）与日历类型
> - 会写一个正确的计时器

> [!tip] 先看结论
> - 测耗时**只能**用 `steady_clock`（单调，不受系统时间调整影响）。
> - `system_clock` 用于"墙上时间"（打印日期、日志时间戳）。
> - `duration` 是"数值 + 单位"的强类型，不要用裸 `double` 表示时间。
> - `std::chrono` 的格式化用 `{:%...}`，与 `strftime` 语法接近但不完全相同。

## 一、`duration`：数值 + 单位

```cpp
using namespace std::chrono_literals;
auto d1 = 1500ms;
auto d2 = 2s;
```

实测：

```text
1500ms = 1500 ms = 1.50 s
总秒数 = 3
```

| 字面量 | 含义 |
| --- | --- |
| `1ns` / `1us` / `1ms` | 纳秒 / 微秒 / 毫秒 |
| `1s` / `1min` / `1h` | 秒 / 分 / 时 |
| `1d`（C++20） | 天 |

换算方式：

| 目标 | 写法 |
| --- | --- |
| 转成整数秒 | `duration_cast<seconds>(d)` |
| 转成浮点秒 | `duration<double>(d).count()` |
| 转成毫秒（浮点） | `duration<double, std::milli>(d).count()` |

> [!warning] `duration_cast` 会截断
> `duration_cast<seconds>(1500ms)` 是 `1s`（不是 1.5）。要精度就用浮点 duration。

## 二、两种 clock

| clock | 特点 | 用途 |
| --- | --- | --- |
| `system_clock` | 墙上时间，可能被 NTP/用户调整 | 打印时间戳、日志 |
| `steady_clock` | 单调递增，不受系统时间影响 | **测耗时** |
| `high_resolution_clock` | 通常是上面之一的别名 | 别直接用 |

实测：

```text
system_clock: 2026-11-05 07:45:48.7590674
steady 自纪元: true ns
当前时区: Asia/Shanghai
本地时间: 2026-11-05 15:45:48.7590674
日期: 2026-11-05 是否合法: true
```

注意：`system_clock` 打印的是 **UTC**，`zoned_time` 转换后才是本地时间（`15:45` vs `07:45`，差 8 小时）。

## 三、格式化

```cpp
std::println("{:%Y-%m-%d %H:%M:%S}", std::chrono::system_clock::now());
```

| 说明 | 输出 |
| --- | --- |
| `%Y` | 4 位年 |
| `%m` / `%d` | 月 / 日（补零） |
| `%H` / `%M` / `%S` | 时 / 分 / 秒 |
| `%F` | 等价 `%Y-%m-%d` |
| `%T` | 等价 `%H:%M:%S` |

## 四、时区与日历

```cpp
auto zone = std::chrono::current_zone();          // Asia/Shanghai
std::chrono::zoned_time local{zone, std::chrono::system_clock::now()};
std::println("{:%Y-%m-%d %H:%M:%S}", local);

std::chrono::year_month_day ymd{std::chrono::year{2026}, std::chrono::month{11}, std::chrono::day{5}};
std::println("{} {}", ymd, ymd.ok());
```

| 类型 | 含义 |
| --- | --- |
| `year_month_day` | 年月日 |
| `year_month_day_last` | 某月最后一天 |
| `weekday` / `weekday_indexed` | 星期 |
| `zoned_time<Duration>` | 带时区的时间点 |

跨时区换算：

```cpp
std::chrono::zoned_time ny{"America/New_York", std::chrono::system_clock::now()};
std::println("纽约: {:%H:%M}", ny);
```

## 五、计时器：正确写法

```cpp
auto t0 = std::chrono::steady_clock::now();
// ... 被测代码
auto t1 = std::chrono::steady_clock::now();
auto ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
std::println("{:.2f} ms", ms);
```

实测：

```text
500 万次累加耗时 11.38 ms（sum=12499997500000）
sleep_for(20ms) 实际 29.5 ms
```

两个值得注意的点：

1. **优化会删掉无用计算**：示例里用 `volatile` 保证循环不被优化掉（代价是变慢），真实测量时要确保代码"有副作用"。
2. **`sleep_for` 不保证精确**：Windows 默认定时器精度约 15.6 ms，所以 20 ms 的睡眠实测 29.5 ms。

## 六、RAII 计时器

```cpp
struct ScopedTimer {
    std::string label;
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    explicit ScopedTimer(std::string l) : label(std::move(l)) {}
    ~ScopedTimer() {
        auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        std::println("[{}] {:.3f} ms", label, ms);
    }
};
```

用法：

```cpp
{ ScopedTimer t{"排序"}; std::ranges::sort(v); }   // 离开作用域自动打印
```

## 七、常见坑

| 坑 | 说明 |
| --- | --- |
| 用 `system_clock` 测耗时 | 系统时间被调整时结果可能为负 |
| `duration_cast` 丢精度 | 1500ms → 1s |
| `sleep_for(1ms)` 实际 15ms | Windows 定时器精度限制 |
| 直接比较不同单位的 duration | 需要用 `common_type` 或显式转换 |
| 打印 UTC 当本地时间 | 用 `zoned_time` 转换 |

## 完整示例（已实测）

```powershell
.\build.ps1 samples\29_chrono.cpp
```

```text
1500ms = 1500 ms = 1.50 s
总秒数 = 3
system_clock: 2026-11-05 07:45:48.7590674
当前时区: Asia/Shanghai
本地时间: 2026-11-05 15:45:48.7590674
日期: 2026-11-05 是否合法: true
500 万次累加耗时 11.38 ms（sum=12499997500000）
sleep_for(20ms) 实际 29.5 ms
```

## 常见错误与排查

| 现象 | 报错 | 原因 | 改法 |
| --- | --- | --- | --- |
| 时间差为负数 | 无 | 用了 `system_clock` | 换 `steady_clock` |
| 格式串报错 | 编译期格式错误 | `{:%...}` 用在不支持的类型 | 转成 `sys_time`/`zoned_time` |
| `current_zone()` 抛异常 | `runtime_error` | 系统缺时区数据库 | 用固定偏移 `utc_offset` |
| 时间精度不够 | 无 | 单位太小 | 用 `milliseconds`/`microseconds` |
| `duration` 相加类型不匹配 | `C2676` | 不同单位的 duration | 先 `duration_cast` 统一 |

## 练习

1. 打印当前时间（本地）与 UTC，格式 `YYYY-MM-DD HH:MM:SS`。
2. 计算从今天到 2027 年元旦还有多少天（用 `year_month_day` 转 `sys_days`）。
3. 写 `ScopedTimer`，测 `std::ranges::sort` 排序 100 万个整数的耗时。
4. 用 `duration` 表示 1 小时 30 分 15 秒，并换算成秒。
5. 把 UTC 时间 `2026-11-05 07:00:00` 转成 `Asia/Shanghai` 的本地时间。
6. 解释为什么 `sleep_for(1ms)` 在 Windows 上实际可能睡 15 ms。

> [!success]- 参考答案
> 1.
> ```cpp
> auto now = std::chrono::system_clock::now();
> std::println("UTC: {:%F %T}", now);
> std::println("本地: {:%F %T}", std::chrono::zoned_time{std::chrono::current_zone(), now});
> ```
> 2.
> ```cpp
> std::chrono::year_month_day today{std::chrono::year{2026}, std::chrono::month{11}, std::chrono::day{5}};
> std::chrono::year_month_day target{std::chrono::year{2027}, std::chrono::month{1}, std::chrono::day{1}};
> auto days = std::chrono::sys_days{target} - std::chrono::sys_days{today};
> std::println("还有 {} 天", days.count());
> ```
> 3.
> ```cpp
> std::vector<int> v(1'000'000);
> std::ranges::generate(v, std::mt19937{42});
> { ScopedTimer t{"sort"}; std::ranges::sort(v); }
> ```
> 4.
> ```cpp
> auto d = std::chrono::hours{1} + std::chrono::minutes{30} + std::chrono::seconds{15};
> std::println("{} s", std::chrono::duration_cast<std::chrono::seconds>(d).count());   // 5415
> ```
> 5.
> ```cpp
> auto utc = std::chrono::sys_days{std::chrono::year{2026}/11/5} + std::chrono::hours{7};
> std::chrono::zoned_time z{"Asia/Shanghai", utc};
> std::println("{:%F %T}", z);
> ```
> 6. Windows 默认系统定时器分辨率是 15.625 ms，`sleep_for` 只能按这个粒度调度，所以 1 ms 的请求实际会睡一个时钟滴答。可以用 `timeBeginPeriod` 提高精度，但代价是耗电增加。

---

上一节：[[28 流与序列化]] ｜ 下一节：[[30 随机数]]
