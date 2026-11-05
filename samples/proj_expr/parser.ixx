export module expr.parser;

import std;
import expr.lexer;

export namespace expr {

struct Node;
using NodePtr = std::shared_ptr<Node>;

struct Node {
    enum class Kind { Number, Variable, Binary, Assign, Negate } kind{Kind::Number};
    double value = 0;
    std::string name;
    char op = 0;
    NodePtr lhs, rhs;
};

struct ParseResult {
    NodePtr root;
    std::string assigned_name;      // 如果是赋值语句
};

export class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : t_(std::move(tokens)) {}

    std::expected<ParseResult, std::string> parse_statement() {
        auto expr = parse_expression();
        if (!expr) return std::unexpected(expr.error());

        ParseResult r;
        if (peek().kind == TokenKind::Assign) {
            // 形如 x = expr，但我们先在 parse_expression 里解析了 lhs
            return std::unexpected(std::string("赋值请用 x = 表达式 形式，且左侧必须是变量"));
        }
        r.root = *expr;
        if (peek().kind != TokenKind::End) return std::unexpected(std::format("多余的内容 (偏移 {})", peek().pos));
        return r;
    }

    // 专门处理 "x = expr"
    std::expected<ParseResult, std::string> parse_assignment() {
        if (peek().kind == TokenKind::Identifier && peek(1).kind == TokenKind::Assign) {
            std::string name = peek().text;
            advance(); advance();
            auto rhs = parse_expression();
            if (!rhs) return std::unexpected(rhs.error());
            if (peek().kind != TokenKind::End) return std::unexpected(std::format("多余的内容 (偏移 {})", peek().pos));
            ParseResult r;
            r.assigned_name = name;
            r.root = *rhs;
            return r;
        }
        return std::unexpected(std::string("不是赋值语句"));
    }

    std::expected<NodePtr, std::string> parse_all() {
        auto e = parse_expression();
        if (!e) return std::unexpected(e.error());
        if (peek().kind != TokenKind::End) return std::unexpected(std::format("多余的内容 (偏移 {})", peek().pos));
        return *e;
    }

private:
    std::vector<Token> t_;
    std::size_t i_ = 0;

    const Token& peek(std::size_t ahead = 0) const {
        static Token end{TokenKind::End, "", 0, 0};
        std::size_t idx = std::min(i_ + ahead, t_.size() - 1);
        return t_[idx];
    }
    const Token& advance() { return t_[i_ < t_.size() - 1 ? i_++ : i_]; }
    bool match(TokenKind k) { if (peek().kind == k) { advance(); return true; } return false; }

    // 表达式：加减
    std::expected<NodePtr, std::string> parse_expression() {
        auto lhs = parse_term();
        if (!lhs) return std::unexpected(lhs.error());
        while (peek().kind == TokenKind::Plus || peek().kind == TokenKind::Minus) {
            char op = peek().kind == TokenKind::Plus ? '+' : '-';
            advance();
            auto rhs = parse_term();
            if (!rhs) return std::unexpected(rhs.error());
            auto node = std::make_shared<Node>();
            node->kind = Node::Kind::Binary;
            node->op = op;
            node->lhs = *lhs;
            node->rhs = *rhs;
            lhs = node;
        }
        return lhs;
    }

    // 项：乘除模
    std::expected<NodePtr, std::string> parse_term() {
        auto lhs = parse_unary();
        if (!lhs) return std::unexpected(lhs.error());
        while (peek().kind == TokenKind::Star || peek().kind == TokenKind::Slash || peek().kind == TokenKind::Percent) {
            char op = peek().kind == TokenKind::Star ? '*' : (peek().kind == TokenKind::Slash ? '/' : '%');
            advance();
            auto rhs = parse_unary();
            if (!rhs) return std::unexpected(rhs.error());
            auto node = std::make_shared<Node>();
            node->kind = Node::Kind::Binary;
            node->op = op;
            node->lhs = *lhs;
            node->rhs = *rhs;
            lhs = node;
        }
        return lhs;
    }

    // 一元负号
    std::expected<NodePtr, std::string> parse_unary() {
        if (match(TokenKind::Minus)) {
            auto operand = parse_unary();
            if (!operand) return std::unexpected(operand.error());
            auto node = std::make_shared<Node>();
            node->kind = Node::Kind::Negate;
            node->lhs = *operand;
            return node;
        }
        return parse_primary();
    }

    std::expected<NodePtr, std::string> parse_primary() {
        const Token& tok = peek();
        if (tok.kind == TokenKind::Number) {
            auto node = std::make_shared<Node>();
            node->kind = Node::Kind::Number;
            node->value = tok.number;
            advance();
            return node;
        }
        if (tok.kind == TokenKind::Identifier) {
            auto node = std::make_shared<Node>();
            node->kind = Node::Kind::Variable;
            node->name = tok.text;
            advance();
            return node;
        }
        if (match(TokenKind::LParen)) {
            auto inner = parse_expression();
            if (!inner) return std::unexpected(inner.error());
            if (!match(TokenKind::RParen)) return std::unexpected(std::format("缺少 ) (偏移 {})", peek().pos));
            return inner;
        }
        return std::unexpected(std::format("意外的 token '{}' (偏移 {})", tok.text, tok.pos));
    }
};

}
