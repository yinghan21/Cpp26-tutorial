import std;

int main() {
    // ---- duration 与单位换算 ----
    using namespace std::chrono_literals;
    auto d1 = 1500ms;
    auto d2 = 2s;
    std::println("1500ms = {} ms = {:.2f} s", d1.count(), std::chrono::duration<double>(d1).count());
    std::println("总秒数 = {}", std::chrono::duration_cast<std::chrono::seconds>(d1 + d2).count());

    // ---- time_point 与两种 clock ----
    auto sys_now = std::chrono::system_clock::now();
    auto steady_now = std::chrono::steady_clock::now();
    std::println("system_clock: {:%Y-%m-%d %H:%M:%S}", sys_now);
    std::println("steady 自纪元: {} ns", steady_now.time_since_epoch().count() > 0);

    // ---- 时区 ----
    auto zone = std::chrono::current_zone();
    std::println("当前时区: {}", zone->name());
    std::chrono::zoned_time local{zone, sys_now};
    std::println("本地时间: {:%Y-%m-%d %H:%M:%S}", local);

    // ---- 日历类型 ----
    std::chrono::year_month_day ymd{std::chrono::year{2026}, std::chrono::month{11}, std::chrono::day{5}};
    std::println("日期: {} 是否合法: {}", ymd, ymd.ok());

    // ---- 计时器 ----
    auto t0 = std::chrono::steady_clock::now();
    volatile std::int64_t sum = 0;
    for (int i = 0; i < 5'000'000; ++i) sum += i;
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::println("500 万次累加耗时 {:.2f} ms（sum={}）", ms, static_cast<std::int64_t>(sum));

    // ---- sleep ----
    auto s0 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(20ms);
    auto slept = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - s0).count();
    std::println("sleep_for(20ms) 实际 {:.1f} ms", slept);
}
