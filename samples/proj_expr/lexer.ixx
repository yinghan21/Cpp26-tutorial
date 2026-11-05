export module expr.lexer;

import std;

export namespace expr {

enum class TokenKind { Number, Identifier, Plus, Minus, Star, Slash, Percent, LParen, RParen, Assign, End };

struct Token {
    TokenKind kind{};
    std::string text;
    double number{};
    std::size_t pos{};
};

export class Lexer {
public:
    explicit Lexer(std::string_view src) : src_(src) {}

    std::expected<std::vector<Token>, std::string> tokenize() {
        std::vector<Token> out;
        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (std::isspace(static_cast<unsigned char>(c))) { ++pos_; continue; }

            if (std::isdigit(static_cast<unsigned char>(c)) || (c == '.' && pos_ + 1 < src_.size())) {
                std::size_t start = pos_;
                while (pos_ < src_.size() && (std::isdigit(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '.')) ++pos_;
                double d{};
                auto [p, ec] = std::from_chars(src_.data() + start, src_.data() + pos_, d);
                if (ec != std::errc{}) return std::unexpected(std::format("非法数字 (偏移 {})", start));
                out.push_back({TokenKind::Number, std::string(src_.substr(start, pos_ - start)), d, start});
                continue;
            }

            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                std::size_t start = pos_;
                while (pos_ < src_.size() && (std::isalnum(static_cast<unsigned char>(src_[pos_])) || src_[pos_] == '_')) ++pos_;
                out.push_back({TokenKind::Identifier, std::string(src_.substr(start, pos_ - start)), 0, start});
                continue;
            }

            TokenKind kind{};
            switch (c) {
                case '+': kind = TokenKind::Plus; break;
                case '-': kind = TokenKind::Minus; break;
                case '*': kind = TokenKind::Star; break;
                case '/': kind = TokenKind::Slash; break;
                case '%': kind = TokenKind::Percent; break;
                case '(': kind = TokenKind::LParen; break;
                case ')': kind = TokenKind::RParen; break;
                case '=': kind = TokenKind::Assign; break;
                case ';': ++pos_; continue;
                default: return std::unexpected(std::format("非法字符 '{}' (偏移 {})", c, pos_));
            }
            out.push_back({kind, std::string(1, c), 0, pos_});
            ++pos_;
        }
        out.push_back({TokenKind::End, "", 0, pos_});
        return out;
    }

private:
    std::string_view src_;
    std::size_t pos_ = 0;
};

}
