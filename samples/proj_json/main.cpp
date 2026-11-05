import std;
import json;

int main() {
    const std::string text = R"({
        "name": "cpp26",
        "version": 26,
        "stable": true,
        "tags": ["modern", "modules", "ranges"],
        "author": { "name": "YG", "age": null }
    })";

    auto parsed = json::parse(text);
    if (!parsed) {
        std::println("解析失败: {}", parsed.error());
        return 1;
    }

    const auto& v = *parsed;
    std::println("name    = {}", v.find("name")->str);
    std::println("version = {}", v.find("version")->number);
    std::println("stable  = {}", v.find("stable")->boolean);
    std::println("tags[1] = {}", v.find("tags")->items[1].str);
    std::println("author  = {}", v.find("author")->find("name")->str);

    std::println("重新序列化: {}", json::dump(v));

    // 错误处理
    auto bad = json::parse(R"({ "a": [1, 2 })");
    if (!bad) std::println("预期中的错误: {}", bad.error());
}
