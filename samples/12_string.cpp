import std;

int main() {
    // ---- string 的 SSO：短字符串不分配堆 ----
    std::string short_s = "abc";
    std::string long_s(100, 'x');
    std::println("sizeof(string)={} 短 capacity={} 长 capacity={}", sizeof(short_s), short_s.capacity(), long_s.capacity());

    // ---- 常用 API ----
    std::string s = "hello, world";
    std::println("size={} find={} substr={}", s.size(), s.find("world"), s.substr(0, 5));
    std::println("contains={} starts_with={} ends_with={}", s.contains("lo"), s.starts_with("hel"), s.ends_with("rld"));
    s.replace(0, 5, "HI");
    std::println("replace -> {}", s);

    // ---- string_view：零拷贝 ----
    std::string_view sv = s;
    std::println("view={} size={} substr_view={}", sv, sv.size(), sv.substr(0, 2));

    // ---- 悬垂 view 演示（不要这样写） ----
    std::string_view bad;
    {
        std::string tmp = "temp";
        bad = tmp;
    }
    std::println("bad view size（UB 演示，可能仍显示 4）={}", bad.size());

    // ---- from_chars / to_chars：最快、不抛异常 ----
    int value = 0;
    std::string_view num = "12345";
    auto [ptr, ec] = std::from_chars(num.data(), num.data() + num.size(), value);
    std::println("from_chars={} ec={}", value, static_cast<int>(ec));

    char buf[32];
    auto res = std::to_chars(buf, buf + sizeof(buf), 3.14159, std::chars_format::fixed, 2);
    std::println("to_chars={}", std::string_view(buf, res.ptr - buf));

    // ---- 拼接性能陷阱 ----
    std::string acc;
    for (int i = 0; i < 3; ++i) acc += std::to_string(i) + ",";
    std::println("acc={} (每轮都产生临时 string)", acc);

    // ---- UTF-8 与 char8_t ----
    std::u8string u8 = u8"中文";
    std::println("u8 字节数={} 字符数(近似)={}", u8.size(), u8.size() / 3);
    std::string back(reinterpret_cast<const char*>(u8.data()), u8.size());
    std::println("转回 string: {}", back);
}
