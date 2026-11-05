export module json;

import std;

export namespace json {

struct Value {
    enum class Kind { Null, Bool, Number, String, Array, Object };

    Kind kind = Kind::Null;
    bool boolean = false;
    double number = 0;
    std::string str;
    std::vector<Value> items;
    std::map<std::string, Value> members;

    const Value* find(std::string_view key) const {
        if (kind != Kind::Object) return nullptr;
        auto it = members.find(std::string{key});
        return it == members.end() ? nullptr : &it->second;
    }
};

// ---- 词法 + 递归下降语法分析 ----
class Parser {
public:
    explicit Parser(std::string_view text) : s_(text) {}

    std::expected<Value, std::string> parse() {
        skip_ws();
        auto v = parse_value();
        if (!v) return std::unexpected(v.error());
        skip_ws();
        if (pos_ != s_.size()) return std::unexpected(err("多余的内容"));
        return *v;
    }

private:
    std::string_view s_;
    std::size_t pos_ = 0;

    std::string err(std::string_view msg) const {
        return std::format("{} (偏移 {})", msg, pos_);
    }
    void skip_ws() {
        while (pos_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[pos_]))) ++pos_;
    }
    bool consume(char c) {
        if (pos_ < s_.size() && s_[pos_] == c) { ++pos_; return true; }
        return false;
    }

    std::expected<Value, std::string> parse_value() {
        skip_ws();
        if (pos_ >= s_.size()) return std::unexpected(err("意外的结尾"));
        char c = s_[pos_];
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') {
            auto s = parse_string();
            if (!s) return std::unexpected(s.error());
            Value v; v.kind = Value::Kind::String; v.str = *s;
            return v;
        }
        if (consume('t')) { expect("rue"); Value v; v.kind = Value::Kind::Bool; v.boolean = true; return v; }
        if (consume('f')) { expect("alse"); Value v; v.kind = Value::Kind::Bool; v.boolean = false; return v; }
        if (consume('n')) { expect("ull"); return Value{}; }
        return parse_number();
    }

    void expect(std::string_view rest) {
        for (char c : rest) { if (pos_ < s_.size() && s_[pos_] == c) ++pos_; }
    }

    std::expected<std::string, std::string> parse_string() {
        if (!consume('"')) return std::unexpected(err("期望字符串"));
        std::string out;
        while (pos_ < s_.size()) {
            char c = s_[pos_++];
            if (c == '"') return out;
            if (c == '\\' && pos_ < s_.size()) {
                char e = s_[pos_++];
                switch (e) {
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    default: out += e; break;
                }
                continue;
            }
            out += c;
        }
        return std::unexpected(err("字符串未闭合"));
    }

    std::expected<Value, std::string> parse_number() {
        std::size_t start = pos_;
        while (pos_ < s_.size() && (std::isdigit(static_cast<unsigned char>(s_[pos_])) ||
                                    s_[pos_] == '-' || s_[pos_] == '+' || s_[pos_] == '.' ||
                                    s_[pos_] == 'e' || s_[pos_] == 'E')) ++pos_;
        if (pos_ == start) return std::unexpected(err("无法识别的值"));
        double d{};
        auto [ptr, ec] = std::from_chars(s_.data() + start, s_.data() + pos_, d);
        if (ec != std::errc{}) return std::unexpected(err("数字格式错误"));
        Value v; v.kind = Value::Kind::Number; v.number = d;
        return v;
    }

    std::expected<Value, std::string> parse_array() {
        consume('[');
        Value v; v.kind = Value::Kind::Array;
        skip_ws();
        if (consume(']')) return v;
        while (true) {
            auto item = parse_value();
            if (!item) return std::unexpected(item.error());
            v.items.push_back(std::move(*item));
            skip_ws();
            if (consume(',')) continue;
            if (consume(']')) return v;
            return std::unexpected(err("数组缺少 , 或 ]"));
        }
    }

    std::expected<Value, std::string> parse_object() {
        consume('{');
        Value v; v.kind = Value::Kind::Object;
        skip_ws();
        if (consume('}')) return v;
        while (true) {
            skip_ws();
            auto key = parse_string();
            if (!key) return std::unexpected(key.error());
            skip_ws();
            if (!consume(':')) return std::unexpected(err("对象缺少 :"));
            auto val = parse_value();
            if (!val) return std::unexpected(val.error());
            v.members.emplace(std::move(*key), std::move(*val));
            skip_ws();
            if (consume(',')) continue;
            if (consume('}')) return v;
            return std::unexpected(err("对象缺少 , 或 }"));
        }
    }
};

export std::expected<Value, std::string> parse(std::string_view text) {
    return Parser{text}.parse();
}

export std::string dump(const Value& v) {
    switch (v.kind) {
        case Value::Kind::Null: return "null";
        case Value::Kind::Bool: return v.boolean ? "true" : "false";
        case Value::Kind::Number: return std::format("{}", v.number);
        case Value::Kind::String: return std::format("\"{}\"", v.str);
        case Value::Kind::Array: {
            std::string out = "[";
            for (std::size_t i = 0; i < v.items.size(); ++i) {
                if (i) out += ",";
                out += dump(v.items[i]);
            }
            return out + "]";
        }
        case Value::Kind::Object: {
            std::string out = "{";
            bool first = true;
            for (const auto& [k, val] : v.members) {
                if (!first) out += ",";
                first = false;
                out += std::format("\"{}\":{}", k, dump(val));
            }
            return out + "}";
        }
    }
    return "null";
}

}
